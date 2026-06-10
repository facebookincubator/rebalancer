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

#include "algopt/rebalancer/materializer/spec_builder/ItemsAffinitySpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

ItemsAffinitySpecBuilder::ItemsAffinitySpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::ItemsAffinitySpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<std::vector<ConstraintInfo>>
ItemsAffinitySpecBuilder::constraints(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("ItemsAffinitySpec not supported as a constraint");
}

folly::coro::Task<ExprPtr> ItemsAffinitySpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto partitionId = universe_->getPartitionId(*spec_.partitionName());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());

  auto& scopeItems1 = *spec_.scopeItemsOfType1();
  auto& scopeItems2 = *spec_.scopeItemsOfType2();
  if (scopeItems1.empty() || scopeItems2.empty()) {
    // only one set of items exist, no affinity to build
    co_return const_expr(0, universe_);
  }
  // Let x_g1 = # of parts of server group g assigned to items1
  // Let x_g2 = # of parts of server group g assigned to items2
  // We want to ensure that for each server there are as many pairs of type-1
  // parts as type-2 parts
  // We can do this by
  //  min \sum_{g \in all groups}  max(x_g1, x_g2)
  // essentially, for each group g, we try to minimize the imbalance between
  // x_g1 and x_g2
  ExprPtr objective = const_expr(0, universe_);
  for (auto groupId : universe_->getPartition(partitionId).getGroupIds()) {
    auto util1 = const_expr(0, universe_);
    auto util2 = const_expr(0, universe_);
    for (const auto& scopeItem : scopeItems1) {
      util1 += co_await expressionBuilder.getAbsoluteUtil(
          UtilMetric::AFTER,
          dimensionId,
          scopeId,
          universe_->getScopeItemId(scopeId, scopeItem),
          partitionId,
          groupId);
    }
    for (const auto& scopeItem : scopeItems2) {
      util2 += co_await expressionBuilder.getAbsoluteUtil(
          UtilMetric::AFTER,
          dimensionId,
          scopeId,
          universe_->getScopeItemId(scopeId, scopeItem),
          partitionId,
          groupId);
    }
    inplace_add(objective, max({util1, util2}, universe_), universe_);
  }
  co_return objective;
}

std::string ItemsAffinitySpecBuilder::description() const {
  return fmt::format(
      "ItemsAffinitySpec({}, {}) for scope {}",
      *spec_.partitionName(),
      *spec_.dimension(),
      *spec_.scope());
}

SpecParameters ItemsAffinitySpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .partition = *spec_.partitionName(),
      .dimension = *spec_.dimension()};
}

} // namespace facebook::rebalancer::materializer
