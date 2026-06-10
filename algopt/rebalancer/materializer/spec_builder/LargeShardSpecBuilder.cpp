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

#include "algopt/rebalancer/materializer/spec_builder/LargeShardSpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <algorithm>
#include <limits>

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

namespace {
constexpr int BUCKET_COUNT = 10;
} // namespace

LargeShardSpecBuilder::LargeShardSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::LargeShardSpec spec)
    : SpecBuilder(universe),
      spec_(std::move(spec)),
      scopeId_(universe_->getScopeId(*spec_.scope())),
      dimensionId_(universe_->getDimensionId(*spec_.dimension())),
      unassignedScopeItemId_(universe_->getScopeItemId(
          scopeId_,
          *spec_.unassignedScopeItemName())) {
  const auto& objectDimension =
      universe_->getObjects().getDimension(dimensionId_);
  if (objectDimension.size() > 1) {
    throw std::runtime_error(
        "LargeShardSpec is not currently supported when size of given dimension > 1");
  }

  collectImmovableShardsAndNonAcceptingScopeItems();
}

std::string LargeShardSpecBuilder::description() const {
  return fmt::format(
      "Accommodate a large shard w.r.t. dimension '{}'", *spec_.dimension());
}

SpecParameters LargeShardSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .dimension = *spec_.dimension(),
      .filterAllowListSize = spec_.filter()->itemsWhitelist()
          ? static_cast<int>(spec_.filter()->itemsWhitelist()->size())
          : 0,
      .filterBlockListSize = spec_.filter()->itemsBlacklist()
          ? static_cast<int>(spec_.filter()->itemsBlacklist()->size())
          : 0};
}

folly::coro::Task<std::vector<ConstraintInfo>>
LargeShardSpecBuilder::constraints(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("LargeShardSpec is not supported as a constraint");
}

folly::coro::Task<ExprPtr> LargeShardSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto candidateLargeShardOpt =
      findLargestMovableShardIn(unassignedScopeItemId_);
  if (!candidateLargeShardOpt.has_value()) {
    XLOG(WARN) << fmt::format(
        "Not adding LargeShardSpec goal w.r.t. dimension '{}' since no candidate large shard found",
        *spec_.dimension());
    co_return const_expr(0, universe_);
  }
  auto [candidateLargeShardId, candidateLargeShardSize] =
      candidateLargeShardOpt.value();

  // assignable scopeItems are ones that satisfy the filter, are not the same as
  // 'unassignedScopeItem', and are not part of a 'nonAcceptingSpec'
  const ScopeItemFilterWrapper filter(*universe_, *spec_.filter(), scopeId_);
  auto assignableScopeItemIds = filter.getScopeItemIds();
  std::erase_if(assignableScopeItemIds, [&](const ScopeItemId id) {
    return (id == unassignedScopeItemId_) ||
        nonAcceptingScopeItemIds_.contains(id);
  });

  auto candidateScopeItemOpt = co_await findCandidateScopeItemToDrain(
      expressionBuilder, assignableScopeItemIds, candidateLargeShardSize);

  if (!candidateScopeItemOpt.has_value()) {
    XLOG(WARN) << fmt::format(
        "For LargeShardSpec goal w.r.t. dimension '{}', adding constant goal expr since no candidate scopeItem found to drain",
        *spec_.dimension());
    co_return const_expr(candidateLargeShardSize, universe_);
  }

  /*
  Add a goal to drain as much as required to fit the large shard if it is in
  the unassignedScopeItem. In other words, we want to incentivize the value of
  'candidateScopeItemUtil' to be such that there is enough space for
  largeShard to move in.
  */
  auto candidateScopeItemId = candidateScopeItemOpt.value();
  auto candidateScopeItemCapacity = universe_->getScope(scopeId_)
                                        .getDimension(dimensionId_)
                                        .getValue(candidateScopeItemId);
  auto candidateScopeItemUtil = co_await expressionBuilder.getAbsoluteUtil(
      UtilMetric::AFTER, dimensionId_, scopeId_, candidateScopeItemId);
  auto currAvailableCapacity =
      max({const_expr(0, universe_),
           candidateScopeItemCapacity - candidateScopeItemUtil},
          universe_);

  // we only require extra space in the candidateScopeItem if the
  // candidateLargeShard remains in the unassignedScopeItem
  auto isCandidateLargeShardUnassigned = expressionBuilder.isAssigned(
      scopeId_, unassignedScopeItemId_, candidateLargeShardId);
  auto spaceForCandidateLargeShard =
      isCandidateLargeShardUnassigned * candidateLargeShardSize;

  co_return max(
      {const_expr(0, universe_),
       spaceForCandidateLargeShard - currAvailableCapacity},
      universe_);
}

folly::coro::Task<std::optional<ScopeItemId>>
LargeShardSpecBuilder::findCandidateScopeItemToDrain(
    ExpressionBuilder& expressionBuilder,
    const std::vector<ScopeItemId>& assignableScopeItemIds,
    double candidateLargeShardSize) const {
  /*
  We ideally want to find a scopeItem that is "easy to drain" so that
  we can move the largeShard into this scopeItem. The "easiness to drain"
  metric, henceforth referred as "DrainMetric", is captured by a heuristic, and
  the core assumption is that it is easier to drain a scopeItem with several
  small shards than a scopeItem which has a relatively larger shard. More
  elaborately, we are looking to find scopeItems where size of the largest shard
  that needs to be removed is small and where we only need to move a few shards
  to make space for the candidateLargeShard.

  For a scopeItem, the "DrainMetric" is captured by first computing the
  "RawDrainMetric" tuple (maxShardRemovalSizeRequired, nShardsToRemove), and
  then converting it to the tuple (shardRemovalSizeBucketIndex,
  nShardsToRemove). To define the terms above, we use the following notation:

    Let O be the set of movable shards in scopeItem C arranged in non-decreasing
    order of their dimension value. Let S be the shard of the smallest dimension
    value in O such that by removing all the shards until (and including) S from
    O, there will be enough space to fit the candidateLargeShard in C. Then,

      1) maxShardRemovalSizeRequired is the dimension value of S
      2) nShardsToRemove is the position of S in O (i.e., 1 + its index in O)
      3) shardRemovalSizeBucketIndex is a value between 0 and BUCKET_COUNT that
      is used to categorize scopeItems, and that essentially denotes that
      scopeItems mapped to the same bucketIndex have similar
      maxShardRemovalSizeRequired value associated with them. (This is done to
      ensure that we are not too reliant on maxShardRemovalSizeRequired.)
  */
  const auto scopeItemIdToDrainMetric =
      co_await computeDrainMetricForScopeItems(
          expressionBuilder, assignableScopeItemIds, candidateLargeShardSize);

  if (scopeItemIdToDrainMetric.size() == 0) {
    // no candidateScopeItem found
    co_return std::nullopt;
  }

  // Find the scopeItem with the best DrainMetric
  auto compareFunc = [&](const std::pair<ScopeItemId, DrainMetric>& c1,
                         const std::pair<ScopeItemId, DrainMetric>& c2) {
    auto& [c1Id, c1DrainMetric] = c1;
    auto& [c2Id, c2DrainMetric] = c2;

    if (c1DrainMetric == c2DrainMetric) {
      // if two scopeItems are equivalent in terms of their DrainMetrics, then
      // break ties deterministically based on their names
      return universe_->getEntityName(c1Id) < universe_->getEntityName(c2Id);
    }
    return c1DrainMetric < c2DrainMetric;
  };

  auto [candidateScopeItemId, _] = *std::min_element(
      scopeItemIdToDrainMetric.begin(),
      scopeItemIdToDrainMetric.end(),
      compareFunc);

  co_return candidateScopeItemId;
}

folly::coro::Task<Map<ScopeItemId, LargeShardSpecBuilder::DrainMetric>>
LargeShardSpecBuilder::computeDrainMetricForScopeItems(
    ExpressionBuilder& expressionBuilder,
    const std::vector<ScopeItemId>& assignableScopeItemIds,
    double candidateLargeShardSize) const {
  auto minMaxSizeShard = std::numeric_limits<double>::max();
  auto maxSizeShard = std::numeric_limits<double>::lowest();
  Map<ScopeItemId, std::pair<double, int>> scopeItemIdToRawDrainMetric;
  for (auto scopeItemId : assignableScopeItemIds) {
    auto scopeItemCapacity = universe_->getScope(scopeId_)
                                 .getDimension(dimensionId_)
                                 .getValue(scopeItemId);
    if (scopeItemCapacity < candidateLargeShardSize) {
      // this scopeItem cannot be a candidateScopeItem as it does not have
      // enough space to fit the candidateLargeShard
      continue;
    }

    // 1. Get dimension values for objects in scopeItem in non-decreasing order
    const auto objectDimValues =
        getSortedDimValuesOfInitialObjectsIn(scopeItemId);

    // 2: Find the "rawDrainMetric", i.e., the tuple
    // (maxShardRemovalSizeRequired, nShardsToRemove), for the scopeItem.
    auto rawDrainMetricOpt = co_await computeRawDrainMetric(
        expressionBuilder,
        objectDimValues,
        candidateLargeShardSize,
        scopeItemCapacity,
        scopeItemId);

    if (!rawDrainMetricOpt.has_value()) {
      // If 'rawDrainMetricOpt' has no value, then this implies that there are
      // not enough removable shards in this scopeItem. Therefore, we ignore
      // this scopeItem.
      continue;
    }

    auto [maxSizeShardRemovedFromScopeItem, _] = rawDrainMetricOpt.value();
    minMaxSizeShard =
        std::min(minMaxSizeShard, maxSizeShardRemovedFromScopeItem);
    maxSizeShard = std::max(maxSizeShard, maxSizeShardRemovedFromScopeItem);

    scopeItemIdToRawDrainMetric.emplace(scopeItemId, rawDrainMetricOpt.value());
  }

  // 3. Since we don't want to depend too precisely on the maxShardRemovalSize
  // while picking a candidateScopeItem, we partition all of them into
  // buckets, and use that to compute the DrainMetric, i.e., the tuple
  // (shardRemovalSizeBucketIndex, nShardsToRemove)
  auto bucketSize = (maxSizeShard - minMaxSizeShard) / BUCKET_COUNT;
  Map<ScopeItemId, DrainMetric> scopeItemIdToDrainMetric;
  for (const auto& [scopeItemId, rawDrainMetric] :
       scopeItemIdToRawDrainMetric) {
    const auto& [maxShardRemovalSize, nShardsToRemove] = rawDrainMetric;
    auto bucketIndex = bucketSize != 0
        ? std::ceil((maxShardRemovalSize - minMaxSizeShard) / bucketSize)
        : 0;
    scopeItemIdToDrainMetric.emplace(
        scopeItemId, std::make_pair(bucketIndex, nShardsToRemove));
  }

  co_return scopeItemIdToDrainMetric;
}

folly::coro::Task<std::optional<LargeShardSpecBuilder::RawDrainMetric>>
LargeShardSpecBuilder::computeRawDrainMetric(
    ExpressionBuilder& expressionBuilder,
    const std::vector<ObjectIdValue>& objectDimValues,
    double candidateLargeShardSize,
    double scopeItemCapacity,
    ScopeItemId scopeItemId) const {
  auto initialUtilExpr = co_await expressionBuilder.getAbsoluteUtil(
      UtilMetric::INITIAL, dimensionId_, scopeId_, scopeItemId);
  auto initialUtil = expressionBuilder.getInitialValue(*initialUtilExpr);
  auto availableCapacity = std::max(0.0, scopeItemCapacity - initialUtil);
  auto amountOfDrainRequired =
      std::max(0.0, candidateLargeShardSize - availableCapacity);

  if (universe_->getPrecision().compare(amountOfDrainRequired, 0.0) == 0) {
    // no shards need to be removed
    co_return std::make_pair(0.0, 0);
  }

  double sumToCurrentVal = 0.0;
  double maxShardRemovalSize = 0.0;
  int nShardsToRemove = 0;

  for (auto& [objectId, dimValue] : objectDimValues) {
    if (immovableShards_.contains(objectId)) {
      // removing this shard is in conflict with an avoidMovingSpec
      // constraint, so ignore this shard
      continue;
    }
    sumToCurrentVal += dimValue;
    nShardsToRemove++;

    if (sumToCurrentVal >= amountOfDrainRequired) {
      maxShardRemovalSize = dimValue;
      break;
    }
  }

  if (sumToCurrentVal < amountOfDrainRequired) {
    // there are not enough movable shards in this scopeItem to accommodate the
    // candidateLargeShard
    co_return std::nullopt;
  }

  co_return std::make_pair(maxShardRemovalSize, nShardsToRemove);
}

void LargeShardSpecBuilder::collectImmovableShardsAndNonAcceptingScopeItems() {
  // Collect the objects that are part of a movesInProgressSpec and scopeItems
  // that are part of a nonAcceptingSpec. Shards that are part of the former
  // spec are allowed to move even if they are mentioned in an avoidMovingSpec.
  // ScopeItems that are part of the latter spec cannot be a
  // candidateScopeItem.
  Set<ObjectId> objectsWithMovesInProgress;
  for (auto constraintId : universe_->getConstraints().getConstraintIds()) {
    const auto& spec =
        universe_->getConstraints().getConstraint(constraintId).getSpec();
    if (spec.getType() ==
        interface::ConstraintSpecs::Type::movesInProgressSpec) {
      for (const auto& move : *spec.movesInProgressSpec()->moves()) {
        auto objectId = universe_->getObjectId(*move.objName());
        objectsWithMovesInProgress.insert(objectId);
      }
    } else if (
        spec.getType() == interface::ConstraintSpecs::Type::nonAcceptingSpec) {
      auto& scope = *spec.nonAcceptingSpec()->scope();
      // We only consider scopeItems that are part of the same scope in
      // nonAcceptingSpec as *spec_.scope()
      if (scope == spec_.scope()) {
        for (auto& scopeItemName : *spec.nonAcceptingSpec()->items()) {
          nonAcceptingScopeItemIds_.insert(
              universe_->getScopeItemId(scopeId_, scopeItemName));
        }
      }
    }
  }

  // Going through constraints again to look for avoidMovingSpecs and collect
  // the objects that are part of such a spec. These objects are not allowed to
  // move unless they are mentioned in movesInProgressSpec.
  for (auto constraintId : universe_->getConstraints().getConstraintIds()) {
    const auto& spec =
        universe_->getConstraints().getConstraint(constraintId).getSpec();
    if (spec.getType() == interface::ConstraintSpecs::Type::avoidMovingSpec) {
      for (const auto& object : *spec.avoidMovingSpec()->objects()) {
        auto objectId = universe_->getObjectId(object);
        if (!objectsWithMovesInProgress.contains(objectId)) {
          immovableShards_.insert(objectId);
        }
      }
    }
  }
}

std::optional<LargeShardSpecBuilder::ObjectIdValue>
LargeShardSpecBuilder::findLargestMovableShardIn(
    ScopeItemId scopeItemId) const {
  std::optional<ObjectIdValue> objectIdValue = std::nullopt;
  for (auto containerId :
       universe_->getScope(scopeId_).getContainerIds(scopeItemId)) {
    auto& initialObjectIds =
        universe_->getContainers().getInitialObjectIds(containerId);

    for (auto objectId : initialObjectIds) {
      if (immovableShards_.contains(objectId)) {
        // do not consider immovable shards
        continue;
      }
      auto objectValue = getObjectDimValue(objectId, scopeItemId);

      // update objectIdValue if necessary
      if (!objectIdValue.has_value() ||
          (objectIdValue.has_value() && objectIdValue->second < objectValue)) {
        objectIdValue = std::make_pair(objectId, objectValue);
      }
    }
  }

  return objectIdValue;
}

std::vector<LargeShardSpecBuilder::ObjectIdValue>
LargeShardSpecBuilder::getSortedDimValuesOfInitialObjectsIn(
    ScopeItemId scopeItemId) const {
  std::vector<ObjectIdValue> objectDimValues;
  for (auto containerId :
       universe_->getScope(scopeId_).getContainerIds(scopeItemId)) {
    for (auto objectId :
         universe_->getContainers().getInitialObjectIds(containerId)) {
      objectDimValues.emplace_back(
          objectId, getObjectDimValue(objectId, scopeItemId));
    }
  }

  std::sort(
      objectDimValues.begin(),
      objectDimValues.end(),
      [](const ObjectIdValue& p1, const ObjectIdValue& p2) {
        return p1.second < p2.second;
      });

  return objectDimValues;
}

double LargeShardSpecBuilder::getObjectDimValue(
    ObjectId objectId,
    ScopeItemId scopeItemId) const {
  auto& scalarDimension =
      universe_->getObjects().getDimension(dimensionId_).at(0);

  auto objectValue = scalarDimension.isDynamic()
      ? scalarDimension.getValue(objectId, scopeItemId)
      : scalarDimension.getValue(objectId);

  return objectValue;
}

} // namespace facebook::rebalancer::materializer
