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

#include "algopt/rebalancer/materializer/spec_builder/GroupAssignmentAffinitiesSpecBuilder.h"

#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

GroupAssignmentAffinitiesSpecBuilder::GroupAssignmentAffinitiesSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::GroupAssignmentAffinitiesSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> GroupAssignmentAffinitiesSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto partitionId = universe_->getPartitionId(*spec_.partition());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());

  auto& partition = universe_->getPartition(partitionId);
  auto& dimension = universe_->getObjects().getDimension(dimensionId).only();

  Map<GroupId, double> groupSize;
  for (auto groupId : partition.getGroupIds()) {
    double size = 0;
    for (auto objectId : partition.getObjectIds(groupId)) {
      size += dimension.getValue(objectId);
    }
    groupSize.emplace(groupId, size);
  }

  auto result = const_expr(0, universe_);
  for (auto& affinity : *spec_.affinities()) {
    auto groupId = universe_->getGroupId(partitionId, *affinity.group());
    auto scopeItemId =
        universe_->getScopeItemId(scopeId, *affinity.scopeItem());
    const double targetDimensionValue = *affinity.targetDimensionValue();
    const double weight = *affinity.affinity();

    auto util = co_await expressionBuilder.getAbsoluteUtil(
        UtilMetric::AFTER,
        dimensionId,
        scopeId,
        scopeItemId,
        partitionId,
        groupId);

    auto assignedFraction = util / groupSize.at(groupId);
    const double targetFraction = targetDimensionValue / groupSize.at(groupId);

    result +=
        max({const_expr(0, universe_),
             weight * (targetFraction - assignedFraction)},
            universe_);
  }
  co_return result;
}

folly::coro::Task<std::vector<ConstraintInfo>>
GroupAssignmentAffinitiesSpecBuilder::constraints(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("not supported as a constraint");
}

std::string GroupAssignmentAffinitiesSpecBuilder::description() const {
  return fmt::format(
      "Group assignment affinities of {} to {} on {}",
      *spec_.partition(),
      *spec_.scope(),
      *spec_.dimension());
}

SpecParameters GroupAssignmentAffinitiesSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .partition = *spec_.partition(),
      .dimension = *spec_.dimension(),
      .size = static_cast<int>(spec_.affinities()->size())};
}

} // namespace facebook::rebalancer::materializer
