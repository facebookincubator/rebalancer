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

#include "algopt/rebalancer/materializer/spec_builder/MoveGroupSpecBuilder.h"

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/materializer/utils/ExpressionBuilder.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <cmath>
#include <optional>
#include <stdexcept>

using namespace facebook::rebalancer::entities;
using namespace facebook::rebalancer::interface;

namespace facebook::rebalancer::materializer {

MoveGroupSpecBuilder::MoveGroupSpecBuilder(
    std::shared_ptr<const Universe> universe,
    MoveGroupSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> MoveGroupSpecBuilder::goalCoro(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("MoveGroupSpec not supported as a goal");
}

folly::coro::Task<std::vector<ConstraintInfo>>
MoveGroupSpecBuilder::constraints(ExpressionBuilder& expressionBuilder) const {
  auto scopeId = universe_->getScopeId(universe_->getContainerTypeName());
  auto& scope = universe_->getScope(scopeId);
  auto& scopeItemIds = scope.getScopeItemIds();

  auto partitionId = universe_->getPartitionId(*spec_.partitionName());
  auto& partition = universe_->getPartition(partitionId);
  auto& groupIds = partition.getGroupIds();

  auto countDimensionId = universe_->getDimensionId(
      fmt::format("{}_count", universe_->getObjectTypeName()));

  std::vector<ConstraintInfo> result;

  for (auto scopeItemId : scopeItemIds) {
    for (auto groupId : groupIds) {
      const int groupSize = partition.getObjectIds(groupId).size();
      if (groupSize < 2) {
        continue;
      }

      auto util = co_await expressionBuilder.getAbsoluteUtil(
          UtilMetric::AFTER,
          countDimensionId,
          scopeId,
          scopeItemId,
          partitionId,
          groupId);

      // The scope item must contain either none of the objects in this group,
      // or all of them. If util falls within [1, groupSize - 1], then the
      // constraint is violated.
      auto expr = rectangle(util, 0.5, groupSize - 0.5, universe_);
      result.emplace_back(expr);
    }
  }

  co_return result;
}

std::string MoveGroupSpecBuilder::description() const {
  return fmt::format(
      "Each group of partition {} is contained in a single container",
      *spec_.partitionName());
}

SpecParameters MoveGroupSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(), .partition = *spec_.partitionName()};
}

} // namespace facebook::rebalancer::materializer
