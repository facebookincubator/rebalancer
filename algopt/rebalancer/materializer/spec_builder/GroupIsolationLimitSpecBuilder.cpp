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

#include "algopt/rebalancer/materializer/spec_builder/GroupIsolationLimitSpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/materializer/utils/LimitWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include <algopt/rebalancer/materializer/spec_builder/SpecBuilder.h>

namespace facebook::rebalancer::materializer {

GroupIsolationLimitSpecBuilder::GroupIsolationLimitSpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    interface::GroupIsolationLimitSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<std::vector<ConstraintInfo>>
GroupIsolationLimitSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  auto partitionId = universe_->getPartitionId(*spec_.partitionName());
  auto scopeId = universe_->getScopeId(*spec_.scope());
  std::vector<ConstraintInfo> exprs;

  const LimitWrapper limitWrapper(
      universe_, *spec_.limit(), scopeId, partitionId);
  const ScopeItemFilterWrapper filter(*universe_, *spec_.filter(), scopeId);

  auto objectCountDimension = universe_->getDimensionId(
      fmt::format("{}_count", universe_->getObjectTypeName()));

  auto groupLimits = limitWrapper.getAllGroupLimitsIndptOfScopeItem();
  auto objectPartition = expressionBuilder.getObjectPartition(
      groupLimits, objectCountDimension, partitionId, false);

  for (auto scopeItemId : filter.getScopeItemIds()) {
    auto groupOverrides = limitWrapper.getGroupsOverride(scopeItemId);
    auto expr = expressionBuilder.getObjectPartitionLookup(
        UtilMetric::AFTER,
        scopeId,
        scopeItemId,
        objectPartition,
        groupOverrides,
        false,
        *spec_.groupsAllowed(),
        false);
    exprs.emplace_back(expr);
  }
  co_return exprs;
}

folly::coro::Task<ExprPtr> GroupIsolationLimitSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  co_return getAggregatedConstraintViolation(
      co_await constraints(expressionBuilder), universe_);
}

/**
 Detailed spec description:
 ===========================
 Input: filtered scopeItems S,  partition G, numAllowedGroups k, groupLimits L
 This spec ensures that for every scopeItem in S, no more than k groups in G
 exceed their corresponding limits in L.

 For example, with zero limit for each group and k=1, this spec ensures that all
 groups are isolated, that is, every scopeItem can have objects of at most one
 group.
*/
std::string GroupIsolationLimitSpecBuilder::description() const {
  return fmt::format(
      "Items of scope {} can have at most {} limit violating group(s) of partition {}",
      *spec_.scope(),
      *spec_.groupsAllowed(),
      *spec_.partitionName());
}

SpecParameters GroupIsolationLimitSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .partition = *spec_.partitionName(),
      .limitType = apache::thrift::util::enumNameSafe(*spec_.limit()->type()),
      .filterAllowListSize = spec_.filter()->itemsWhitelist()
          ? static_cast<int>(spec_.filter()->itemsWhitelist()->size())
          : 0,
      .filterBlockListSize = spec_.filter()->itemsBlacklist()
          ? static_cast<int>(spec_.filter()->itemsBlacklist()->size())
          : 0};
}

} // namespace facebook::rebalancer::materializer
