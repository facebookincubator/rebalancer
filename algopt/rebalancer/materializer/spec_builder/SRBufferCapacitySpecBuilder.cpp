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

#include "algopt/rebalancer/materializer/spec_builder/SRBufferCapacitySpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

SRBufferCapacitySpecBuilder::SRBufferCapacitySpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::SRBufferCapacitySpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> SRBufferCapacitySpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto result = const_expr(0, universe_);
  auto exprs = co_await constraints(expressionBuilder);
  for (auto& expr : exprs) {
    result += max({const_expr(0, universe_), expr.constraintExpr}, universe_);
  }
  co_return result;
}

folly::coro::Task<std::vector<ConstraintInfo>>
SRBufferCapacitySpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());
  auto partitionId = universe_->getPartitionId(*spec_.partitionName());
  const auto& partition = universe_->getPartition(partitionId);
  auto defaultMatchingError = *spec_.matchingError();
  auto lowerBoundMatchingErrors = *spec_.lowerBoundMatchingErrors();

  const ScopeItemFilterWrapper filter(*universe_, *spec_.filter(), scopeId);
  auto filteredScopeItemIds = filter.getScopeItemIds();
  const std::unordered_set<entities::ScopeItemId> filteredItemIds(
      filteredScopeItemIds.begin(), filteredScopeItemIds.end());

  std::vector<ConstraintInfo> result;

  if (partition.getGroupIds().empty()) {
    co_return result;
  }

  for (auto& scopeItem : *spec_.scopeItemPairs()) {
    auto mainItemId =
        universe_->getScopeItemId(scopeId, *scopeItem.scopeItem1());
    auto bufferItemId =
        universe_->getScopeItemId(scopeId, *scopeItem.scopeItem2());

    if (!filteredItemIds.contains(mainItemId)) {
      continue;
    }

    std::vector<ExprPtr> partitionExprs;
    const int numFailureScenarios = partition.getGroupIds().size();
    partitionExprs.reserve(numFailureScenarios);
    for (auto groupId : partition.getGroupIds()) {
      partitionExprs.push_back(
          co_await expressionBuilder.getAbsoluteUtil(
              UtilMetric::AFTER,
              dimensionId,
              scopeId,
              mainItemId,
              partitionId,
              groupId) +
          co_await expressionBuilder.getAbsoluteUtil(
              UtilMetric::AFTER,
              dimensionId,
              scopeId,
              bufferItemId,
              partitionId,
              groupId));
    }

    auto theta = folly::get_default(
        lowerBoundMatchingErrors,
        *scopeItem.scopeItem1(),
        defaultMatchingError);

    auto bufferAllocated = co_await expressionBuilder.getAbsoluteUtil(
        UtilMetric::AFTER, dimensionId, scopeId, bufferItemId);

    auto bufferDeficitExpr = const_expr(0, universe_);

    // Lowerbound requirement:
    // bufferAllocated >= bufferRequired - theta
    for (const auto& bufferRequiredForThisPartition : partitionExprs) {
      bufferDeficitExpr +=
          max(0,
              bufferRequiredForThisPartition - bufferAllocated - theta,
              universe_);
    }
    result.emplace_back(bufferDeficitExpr);
  }
  co_return result;
}

std::string SRBufferCapacitySpecBuilder::description() const {
  return fmt::format("SR Buffer capacity for {}", *spec_.scope());
}

SpecParameters SRBufferCapacitySpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .partition = *spec_.partitionName(),
      .dimension = *spec_.dimension(),
      .filterAllowListSize = spec_.filter()->itemsWhitelist()
          ? static_cast<int>(spec_.filter()->itemsWhitelist()->size())
          : 0,
      .filterBlockListSize = spec_.filter()->itemsBlacklist()
          ? static_cast<int>(spec_.filter()->itemsBlacklist()->size())
          : 0};
}

} // namespace facebook::rebalancer::materializer
