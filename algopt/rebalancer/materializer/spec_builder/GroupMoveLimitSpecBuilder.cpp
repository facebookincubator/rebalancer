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

#include "algopt/rebalancer/materializer/spec_builder/GroupMoveLimitSpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include <algopt/rebalancer/materializer/spec_builder/SpecBuilder.h>
#include <algopt/rebalancer/materializer/utils/LimitWrapper.h>

#include <stdexcept>

namespace facebook::rebalancer::materializer {

namespace {
entities::Set<entities::ContainerId> convertToContainerIds(
    const std::vector<entities::ScopeItemId>& scopeItemIds,
    const entities::Universe& universe) {
  entities::Set<entities::ContainerId> containerIds;
  for (auto scopeItemId : scopeItemIds) {
    auto& containerName = universe.getEntityName(scopeItemId);
    containerIds.emplace(universe.getContainerId(containerName));
  }

  return containerIds;
}

} // namespace

GroupMoveLimitSpecBuilder::GroupMoveLimitSpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    interface::GroupMoveLimitSpec spec)
    : SpecBuilder(universe),
      spec_(std::move(spec)),
      groupToMoveLimits_(
          LimitWrapper::getAllGroupLimits(
              universe_,
              universe_->getPartitionId(*spec_.partitionName()),
              *spec_.limit())),
      dimensionId_(
          spec_.dimension().has_value()
              ? universe_->getDimensionId(*spec_.dimension())
              : universe_->getDimensionId(
                    fmt::format("{}_count", universe_->getObjectTypeName()))) {
  auto& objectDimension = universe_->getObjects().getDimension(dimensionId_);
  if (objectDimension.hasNegativeValues()) {
    throw std::runtime_error(
        "GroupMoveLimitSpec is not supported when the object dimension has negative values");
  }

  if (objectDimension.size() > 1) {
    throw std::runtime_error(
        "GroupMoveLimitSpec is not supported when size of given dimension > 1");
  }

  // Note: currently only the container scope is supported in GroupMoveLimitSpec
  auto scopeId = universe_->getScopeId(universe_->getContainerTypeName());
  const ScopeItemFilterWrapper sourceContainersFilter(
      *universe_, *spec_.sourceScopeItemsAffectingLimitFilter(), scopeId);
  const ScopeItemFilterWrapper destinationContainersFilter(
      *universe_, *spec_.destinationScopeItemsAffectingLimitFilter(), scopeId);

  sourceContainerIdsNotAffectingLimit_ = convertToContainerIds(
      sourceContainersFilter.getExcludedScopeItemIds(), *universe_);
  destinationContainerIdsNotAffectingLimit_ = convertToContainerIds(
      destinationContainersFilter.getExcludedScopeItemIds(), *universe_);
}

folly::coro::Task<ExprPtr> GroupMoveLimitSpecBuilder::goalCoro(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("GroupMoveLimitSpec not supported as goal");
}

folly::coro::Task<std::vector<ConstraintInfo>>
GroupMoveLimitSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  co_return std::vector<ConstraintInfo>{
      ConstraintInfo(expressionBuilder.getObjectPartitionMoveLimit(
          groupToMoveLimits_,
          universe_->getPartitionId(*spec_.partitionName()),
          dimensionId_,
          sourceContainerIdsNotAffectingLimit_,
          destinationContainerIdsNotAffectingLimit_))};
}

entities::Set<entities::ObjectId> GroupMoveLimitSpecBuilder::fixedObjects()
    const {
  // if some source or destination does not affect the limit, then we cannot
  // classify objects as 'fixed' based on whether they have a zero move limit
  if (sourceContainerIdsNotAffectingLimit_.size() > 0 ||
      destinationContainerIdsNotAffectingLimit_.size() > 0) {
    return {};
  }

  auto partitionId = universe_->getPartitionId(*spec_.partitionName());
  auto& partition = universe_->getPartition(partitionId);
  entities::Set<entities::ObjectId> fixedObjects;
  for (auto& [groupId, moveLimit] : groupToMoveLimits_) {
    if (universe_->getPrecision().compare(moveLimit, 0.0) == 0) {
      auto& objectsInGroup = partition.getObjectIds(groupId);
      fixedObjects.insert(objectsInGroup.begin(), objectsInGroup.end());
    }
  }
  return fixedObjects;
}

std::string GroupMoveLimitSpecBuilder::description() const {
  return fmt::format(
      "Number of moving objects of partition {} w.r.t. dimension {} must be within limits",
      *spec_.partitionName(),
      universe_->getEntityName(dimensionId_));
}

SpecParameters GroupMoveLimitSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .partition = *spec_.partitionName(),
      .dimension = universe_->getEntityName(dimensionId_),
      .limitType = apache::thrift::util::enumNameSafe(*spec_.limit()->type())};
}

} // namespace facebook::rebalancer::materializer
