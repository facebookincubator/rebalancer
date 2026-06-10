// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "algopt/rebalancer/materializer/spec_builder/ToFreeSpecBuilder.h"

#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <limits>
#include <stdexcept>

namespace entities = facebook::rebalancer::entities;

namespace {
struct DimensionValues {
  double sum;
  double minPositive;
};

DimensionValues getObjectDimensionSumAndMinPositiveValue(
    const entities::ObjectDimension& objectDimension) {
  auto throwMsg = [](const std::string& reason) {
    throw std::runtime_error(
        fmt::format(
            "{} are not supported when using ToFreeSpec with MINIMIZE_OCCUPIED_CONTAINERS formula",
            reason));
  };
  if (objectDimension.hasNegativeValues()) {
    throwMsg("Negative dimension values");
  }
  if (objectDimension.size() > 1 || objectDimension.isDynamic()) {
    throwMsg("Non-scalar or dynamic dimensions");
  }

  auto& objectScalarDimension = objectDimension.at(0);
  const auto minPositiveDimensionValue =
      objectScalarDimension.getMinimumPositiveValue();
  return {
      .sum = objectScalarDimension.values().sum(),
      .minPositive = minPositiveDimensionValue.value_or(
          std::numeric_limits<double>::max())};
}

} // namespace

namespace facebook::rebalancer::materializer {

ToFreeSpecBuilder::ToFreeSpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    interface::ToFreeSpec spec,
    bool continuousExpressions)
    : SpecBuilder(universe),
      spec_(std::move(spec)),
      continuousExpressions_(continuousExpressions),
      scopeId_(universe_->getScopeId(universe_->getContainerTypeName())),
      dimensionId_(
          spec_.dimension().has_value()
              ? universe_->getDimensionId(*spec_.dimension())
              : universe_->getDimensionId(
                    fmt::format("{}_count", universe_->getObjectTypeName()))),
      dimension_(universe_->getObjects().getDimension(dimensionId_)) {
  if (dimension_.size() > 1 || dimension_.at(0).isRoutingConfigBased()) {
    throw std::runtime_error(
        "ToFreeSpec is currently only supported with scalar dimensions");
  }
}

folly::coro::Task<ExprPtr> ToFreeSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  // Note that when used as a goal, ToFree just minimizes the total number of
  // objects in the containers that need to be freed (it does not explicitly
  // disincentivize new objects from moving in, unlike how it is when used as a
  // constraint)
  co_return co_await getObjectiveExpr(expressionBuilder);
}

folly::coro::Task<std::vector<ConstraintInfo>> ToFreeSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  if (spec_.formula() !=
      interface::ToFreeSpecFormula::MINIMIZE_TOTAL_UTILIZATION) {
    throw std::runtime_error(
        "Choosing a formula is not supported when using ToFreeSpec as a constraint;"
        "only the default formula ToFreeSpecFormula::MINIMIZE_TOTAL_UTILIZATION is supported");
  }

  std::vector<ConstraintInfo> exprs;
  exprs.reserve(2 * spec_.containers()->size());
  for (auto& containerName : *spec_.containers()) {
    auto scopeItemId = universe_->getScopeItemId(scopeId_, containerName);

    // afterUtil <= 0; to ensure total util of the container is at most 0 (when
    // using default dimension object_count, we want number of objects in the
    // container <= 0)
    exprs.emplace_back(
        co_await expressionBuilder.getAbsoluteUtil(
            UtilMetric::AFTER, dimensionId_, scopeId_, scopeItemId));

    // newUtil <= 0; we want to be as stable as possible here and ensure no new
    // elements are added to the container
    exprs.emplace_back(
        co_await expressionBuilder.getAbsoluteUtil(
            UtilMetric::NEW, dimensionId_, scopeId_, scopeItemId));

    const auto& scope = universe_->getScope(scopeId_);
    const auto& containers = scope.getContainerIds(scopeItemId);
    if (containers.size() == 1 &&
        !dimension_.hasZeroValuedObjects(scopeItemId)) {
      // if there are zero valued objects, then the container cannot be marked
      // as non-accepting; also, if there are multiple containers in the scope
      // item, then objects can move freely between them
      nonAcceptingContainers_.insert(containers.begin(), containers.end());
    }
  }

  co_return exprs;
}

folly::coro::Task<ExprPtr> ToFreeSpecBuilder::getObjectiveExpr(
    ExpressionBuilder& expressionBuilder) const {
  if (spec_.containers()->empty()) {
    // If there are no containers to free, return 0
    co_return const_expr(0, universe_);
  }

  switch (*spec_.formula()) {
    case interface::ToFreeSpecFormula::MINIMIZE_TOTAL_UTILIZATION:
      co_return co_await getMinimizeTotalUtilFormulaExpr(expressionBuilder);

    case interface::ToFreeSpecFormula::MINIMIZE_OCCUPIED_CONTAINERS:
      co_return continuousExpressions_
          ? co_await getMinimizeOccupiedContainersContinuousFormulaExpr(
                expressionBuilder)
          : co_await getMinimizeOccupiedContainersDiscreteFormulaExpr(
                expressionBuilder);
  }
}

folly::coro::Task<ExprPtr> ToFreeSpecBuilder::getMinimizeTotalUtilFormulaExpr(
    ExpressionBuilder& expressionBuilder) const {
  ExprPtr objective = const_expr(0, universe_);
  for (auto& containerName : *spec_.containers()) {
    auto scopeItemId = universe_->getScopeItemId(scopeId_, containerName);
    inplace_add(
        objective,
        co_await expressionBuilder.getAbsoluteUtil(
            UtilMetric::AFTER, dimensionId_, scopeId_, scopeItemId),
        universe_);
  }

  co_return objective;
}

folly::coro::Task<ExprPtr>
ToFreeSpecBuilder::getMinimizeOccupiedContainersContinuousFormulaExpr(
    ExpressionBuilder& expressionBuilder) const {
  auto& objectDimension = universe_->getObjects().getDimension(dimensionId_);
  auto [objectDimensionSum, minPositiveDimensionValue] =
      getObjectDimensionSumAndMinPositiveValue(objectDimension);
  if (universe_->getPrecision().compare(objectDimensionSum, 0) == 0) {
    co_return const_expr(0, universe_);
  }

  /*
  objective function: minimize the following, where afterUtil() is a single
  lookup over all containers that are to be freed.

    (1.1/minPositiveDimensionValue) * afterUtil(containerToFree)
                  -
  1/objectDimensionSum^2 * sum_{c \in containerToFree} (absoluteUtil(c))^2)

  NOTE:
  a) 0 <= 1/objectDimensionSum^2 * sum_{c \in containerToFree}
  (absoluteUtil(c))^2) <= 1.0, given that all dimension values are non-negative
  (i.e., >= 0) and objectDimensionSuum > 0

  b) (1.1/minPositiveDimensionValue) * afterUtil(containerToFree) decreases by
  at least 1.1 any time an object with non-zero dimension value moves out of a
  container in containerToFree. Therefore, there is always an incentive to move
  an object with non-zero dimension value out.

  c) This objective function assumes that local search is implemented in a
  certain way. In particular,
    (i) in the object_lookup expression that computes
    afterUtil(containerToFree), all the container are considered to be equally
    "hot"

    (ii) as a result of (i), the hottest container c will the one that
    has absoluteUtil(c) closest to zero; this is because -(absoluteUtil(c))^2)
  expression associated with such a container c will have the largest potential
  */
  ExprPtr sumOverUtilSquared = const_expr(0, universe_);
  std::vector<entities::ScopeItemId> scopeItemIdsToFree;
  for (auto& containerName : *spec_.containers()) {
    auto scopeItemId = universe_->getScopeItemId(scopeId_, containerName);
    scopeItemIdsToFree.emplace_back(scopeItemId);
    auto absoluteUtil = co_await expressionBuilder.getAbsoluteUtil(
        UtilMetric::AFTER, dimensionId_, scopeId_, scopeItemId);
    inplace_add(
        sumOverUtilSquared, power(absoluteUtil, 2, universe_), universe_);
  }

  // create a single lookup expr over all scopeItemIdsToFree
  const ExprPtr totalAfterUtil = co_await expressionBuilder.getAbsoluteUtil(
      UtilMetric::AFTER, dimensionId_, scopeId_, scopeItemIdsToFree);

  const double coeff1 = 1.1 / minPositiveDimensionValue;
  const double coeff2 = 1.0 / (objectDimensionSum * objectDimensionSum);
  co_return coeff1* totalAfterUtil - coeff2* sumOverUtilSquared;
}

folly::coro::Task<ExprPtr>
ToFreeSpecBuilder::getMinimizeOccupiedContainersDiscreteFormulaExpr(
    ExpressionBuilder& expressionBuilder) const {
  // When continuous expressions are not required, we can directly minimize the
  // number of occupied containers
  ExprPtr objective = const_expr(0, universe_);
  for (auto& containerName : *spec_.containers()) {
    auto scopeItemId = universe_->getScopeItemId(scopeId_, containerName);
    inplace_add(
        objective,
        step(
            co_await expressionBuilder.getAbsoluteUtil(
                UtilMetric::AFTER, dimensionId_, scopeId_, scopeItemId),
            universe_),
        universe_);
  }

  co_return objective;
}

std::string ToFreeSpecBuilder::description() const {
  return fmt::format(
      "To free {} containers; '{}' dimension; {} formula",
      spec_.containers()->size(),
      universe_->getEntityName(dimensionId_),
      apache::thrift::util::enumNameSafe(*spec_.formula()));
}

SpecParameters ToFreeSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .size = static_cast<int>(spec_.containers()->size())};
}

} // namespace facebook::rebalancer::materializer
