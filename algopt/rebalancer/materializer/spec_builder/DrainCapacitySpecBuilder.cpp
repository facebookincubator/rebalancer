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

#include "algopt/rebalancer/materializer/spec_builder/DrainCapacitySpecBuilder.h"

#include "algopt/rebalancer/solver/expressions/Operators.h"

namespace facebook::rebalancer::materializer {

DrainCapacitySpecBuilder::DrainCapacitySpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    facebook::rebalancer::interface::DrainCapacitySpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> DrainCapacitySpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  co_return getAggregatedConstraintViolation(
      co_await constraints(expressionBuilder), universe_);
}

folly::coro::Task<std::vector<ConstraintInfo>>
DrainCapacitySpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  std::vector<ConstraintInfo> result;

  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto& scope = universe_->getScope(scopeId);
  auto& scopeItemIds = scope.getScopeItemIds();

  auto dimensionId = universe_->getDimensionId(*spec_.dimension());
  auto& dimension = scope.getDimension(dimensionId);

  double totalCapacity = 0.0;
  for (auto scopeItemId : scopeItemIds) {
    totalCapacity += dimension.getValue(scopeItemId);
  }

  const double normCoeff =
      totalCapacity == 0.0 ? 1.0 : 1.0 / (totalCapacity / scopeItemIds.size());

  for (auto& [srcItemName, proportions] : *spec_.spillDistribution()) {
    auto srcItemId = universe_->getScopeItemId(scopeId, srcItemName);
    auto srcUsage = co_await expressionBuilder.getAbsoluteUtil(
        UtilMetric::AFTER, dimensionId, scopeId, srcItemId);

    for (auto& [dstItemName, proportion] : proportions) {
      auto dstItemId = universe_->getScopeItemId(scopeId, dstItemName);
      auto dstUsage = co_await expressionBuilder.getAbsoluteUtil(
          UtilMetric::AFTER, dimensionId, scopeId, dstItemId);
      double dstCapacity = dimension.getValue(dstItemId);

      // Constraint formula:
      // {dst usage} + {proportion} * {src usage} <= {dst capacity}
      auto expr = (dstUsage + proportion * srcUsage - dstCapacity) * normCoeff;
      expr->description = fmt::format(
          "usage of {} + {} * usage of {} <= {}",
          dstItemName,
          proportion,
          srcItemName,
          dstCapacity);
      result.emplace_back(expr);
    }
  }

  co_return result;
}

std::string DrainCapacitySpecBuilder::description() const {
  return fmt::format(
      "limit drain capacity ({}) on scope {}",
      *spec_.dimension(),
      *spec_.scope());
}

SpecParameters DrainCapacitySpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .dimension = *spec_.dimension()};
}

} // namespace facebook::rebalancer::materializer
