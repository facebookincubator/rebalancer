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

#include "algopt/rebalancer/materializer/spec_builder/DiversifyWithinScopeItemSpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <vector>

namespace facebook::rebalancer::materializer {

DiversifyWithinScopeItemSpec::DiversifyWithinScopeItemSpec(
    std::shared_ptr<const entities::Universe> universe,
    interface::DiversifyWithinScopeItemSpec spec)
    : SpecBuilder(universe),
      spec_(std::move(spec)),
      dimensionId_(universe_->getDimensionId(*spec_.dimension())),
      scopeId_(universe_->getScopeId(*spec_.scope())),
      partitionId_(universe_->getPartitionId(*spec_.partition())),
      containerScopeId_(
          universe_->getScopeId(universe_->getContainerTypeName())),
      scope_(universe_->getScope(scopeId_)),
      containerScope_(universe_->getScope(containerScopeId_)),
      partition_(universe_->getPartition(partitionId_)),
      scopeDimension_(scope_.getDimension(dimensionId_)),
      groupToLimit_(LimitWrapper(
          universe_,
          *spec_.groupToLimit(),
          scopeId_,
          partitionId_)) {
  auto throwMsg = [&](const std::string& reason) {
    throw std::runtime_error(
        fmt::format(
            "DiversifyWithinScopeItemSpec is currently not supported with {}",
            reason));
  };

  auto& objectDimension = universe_->getObjects().getDimension(dimensionId_);
  if (objectDimension.size() > 1) {
    throwMsg("non-scalar dimensions");
  }
  if (objectDimension.hasNegativeValues()) {
    throwMsg("dimensions with negative values");
  }
}

folly::coro::Task<std::vector<ConstraintInfo>>
DiversifyWithinScopeItemSpec::constraints(
    ExpressionBuilder& /*expressionBuilder*/) const {
  throw std::runtime_error(
      "DiversifyWithinScopeItemSpec is NOT supported as a constraint");
}

folly::coro::Task<ExprPtr> DiversifyWithinScopeItemSpec::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto filteredScopeItemIds =
      ScopeItemFilterWrapper(*universe_, *spec_.scopeItemFilter(), scopeId_)
          .getScopeItemIds();
  // throw if any scopeItem or if any container in it has zero dimension value
  throwIfZeroScopeDimensionValuesExist(filteredScopeItemIds);

  auto objective = const_expr(0, universe_);
  for (auto groupId : partition_.getGroupIds()) {
    inplace_add(
        objective,
        co_await getObjectiveExpr(
            expressionBuilder, groupId, filteredScopeItemIds),
        universe_);
  }

  co_return objective;
}

folly::coro::Task<ExprPtr> DiversifyWithinScopeItemSpec::getObjectiveExpr(
    ExpressionBuilder& expressionBuilder,
    entities::GroupId groupId,
    const std::vector<entities::ScopeItemId>& scopeItemsIds) const {
  auto groupObjExpr = const_expr(0, universe_);
  for (auto scopeItemId : scopeItemsIds) {
    inplace_add(
        groupObjExpr,
        co_await getDiversificationExpr(
            expressionBuilder, groupId, scopeItemId),
        universe_);
  }

  co_return groupObjExpr;
}

folly::coro::Task<ExprPtr> DiversifyWithinScopeItemSpec::getDiversificationExpr(
    ExpressionBuilder& expressionBuilder,
    entities::GroupId groupId,
    entities::ScopeItemId scopeItemId) const {
  auto limit = (*spec_.groupToLimit()->type() == interface::LimitType::ABSOLUTE)
      ? groupToLimit_.getLimit(scopeItemId, groupId) /
          scopeDimension_.getValue(scopeItemId)
      : groupToLimit_.getLimit(scopeItemId, groupId);

  auto relativeUtil = co_await expressionBuilder.getRelativeUtil(
      UtilMetric::AFTER,
      dimensionId_,
      scopeId_,
      scopeItemId,
      partitionId_,
      groupId);

  co_return product(
      step(relativeUtil - limit, universe_),
      co_await getSpreadingFormula(expressionBuilder, groupId, scopeItemId),
      universe_);
}

folly::coro::Task<ExprPtr> DiversifyWithinScopeItemSpec::getSpreadingFormula(
    ExpressionBuilder& expressionBuilder,
    entities::GroupId groupId,
    entities::ScopeItemId scopeItemId) const {
  auto spreadingFormula = const_expr(0, universe_);
  for (auto containerId : scope_.getContainerIds(scopeItemId)) {
    auto poweredRelativeUtil = power(
        co_await expressionBuilder.getRelativeUtil(
            UtilMetric::AFTER,
            dimensionId_,
            containerScopeId_,
            *containerScope_.getScopeItemId(containerId),
            partitionId_,
            groupId),
        1.1,
        universe_);
    inplace_add(spreadingFormula, poweredRelativeUtil, universe_);
  }

  co_return spreadingFormula;
}

void DiversifyWithinScopeItemSpec::throwIfZeroScopeDimensionValuesExist(
    const std::vector<entities::ScopeItemId>& scopeItemIds) const {
  for (auto scopeItemId : scopeItemIds) {
    auto dimensionValue = scopeDimension_.getValue(scopeItemId);
    if (universe_->getPrecision().compare(dimensionValue, 0) == 0) {
      throw std::runtime_error(
          fmt::format(
              "Expected scope items to have non-zero dimension value when using DiversifyWithinScopeItemSpec,"
              "but found zero for scope item '{}' w.r.t. dimemsion '{}'",
              universe_->getEntityName(scopeItemId),
              *spec_.dimension()));
    }

    for (auto containerId : scope_.getContainerIds(scopeItemId)) {
      auto dimensionValueInContainerScope =
          containerScope_.getDimension(dimensionId_)
              .getValue(*containerScope_.getScopeItemId(containerId));
      if (universe_->getPrecision().compare(
              dimensionValueInContainerScope, 0) == 0) {
        throw std::runtime_error(
            fmt::format(
                "Expected containers to have non-zero dimension value when using DiversifyWithinScopeItemSpec,"
                "but found zero for container '{}' w.r.t. dimemsion '{}'",
                universe_->getEntityName(containerId),
                *spec_.dimension()));
      }
    }
  }
}

std::string DiversifyWithinScopeItemSpec::description() const {
  return fmt::format(
      "Diversify group within scope item using partition '{}', scope '{}', and dimension '{}'",
      *spec_.partition(),
      *spec_.scope(),
      *spec_.dimension());
}

SpecParameters DiversifyWithinScopeItemSpec::getSpecInfo() const {
  SpecParameters info;
  info.name = *spec_.name();
  info.scope = *spec_.scope();
  info.partition = *spec_.partition();
  info.dimension = *spec_.dimension();
  return info;
}

} // namespace facebook::rebalancer::materializer
