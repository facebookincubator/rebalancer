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

#include "algopt/rebalancer/materializer/spec_builder/WorkingSetSpecBuilder.h"

#include "algopt/rebalancer/solver/expressions/Operators.h"

using facebook::rebalancer::interface::WorkingSetMetric;

namespace facebook::rebalancer::materializer {

WorkingSetSpecBuilder::WorkingSetSpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    facebook::rebalancer::interface::WorkingSetSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> WorkingSetSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto& scopeItemIds = universe_->getScope(scopeId).getScopeItemIds();
  auto metric = *spec_.metric();

  std::vector<std::pair<double, entities::Map<entities::ObjectId, double>>>
      workingUnits;
  for (auto& workingUnit : *spec_.workingUnits()) {
    const double weight = *workingUnit.weight();
    entities::Map<entities::ObjectId, double> objects;
    for (auto& objectName : *workingUnit.endpoints()) {
      auto objectId = universe_->getObjectId(objectName);
      objects.emplace(objectId, 1);
    }
    workingUnits.emplace_back(weight, std::move(objects));
  }

  auto result = const_expr(0, universe_);

  for (auto scopeItemId : scopeItemIds) {
    auto workingSetSize = const_expr(0, universe_);
    for (auto& [weight, objects] : workingUnits) {
      auto objectCount = const_expr(0, universe_);
      for (auto& [objectId, objectWeight] : objects) {
        objectCount += objectWeight *
            expressionBuilder.isAssigned(scopeId, scopeItemId, objectId);
      }
      workingSetSize += weight * step(objectCount, universe_);
    }
    workingSetSize->description = fmt::format(
        "Working set size of {} {}",
        universe_->getEntityName(scopeId),
        universe_->getEntityName(scopeItemId));

    if (metric == WorkingSetMetric::AVG) {
      auto util = co_await expressionBuilder.getAbsoluteUtil(
          UtilMetric::AFTER, dimensionId, scopeId, scopeItemId);
      result += product(util, workingSetSize, universe_);
    } else if (metric == WorkingSetMetric::MAX) {
      inplace_max(result, workingSetSize, universe_);
    } else {
      throw std::runtime_error("unknown working set metric");
    }
  }

  co_return result;
}

folly::coro::Task<std::vector<ConstraintInfo>>
WorkingSetSpecBuilder::constraints(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("WorkingSetSpec not supported as a constraint");
}

std::string WorkingSetSpecBuilder::description() const {
  if (*spec_.metric() == WorkingSetMetric::AVG) {
    return fmt::format(
        "Minimize avg working set size of {}s weighted by {}",
        *spec_.scope(),
        *spec_.dimension());
  }
  if (*spec_.metric() == WorkingSetMetric::MAX) {
    return fmt::format("Minimize max working set size of {}s", *spec_.scope());
  }
  throw std::runtime_error("unknown working set metric");
}

SpecParameters WorkingSetSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .dimension = *spec_.dimension(),
      .definition = apache::thrift::util::enumNameSafe(*spec_.metric()),
      .size = static_cast<int>(spec_.workingUnits()->size())};
}

} // namespace facebook::rebalancer::materializer
