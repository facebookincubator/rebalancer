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

#include "algopt/rebalancer/entities/builders/AsyncUniverseBuilder.h"

#include <folly/logging/xlog.h>

namespace facebook::rebalancer::entities {

namespace {
template <typename NameToIdMap>
std::vector<std::string> buildNameVector(const NameToIdMap& nameToId) {
  std::vector<std::string> names(nameToId.size());
  for (const auto& [name, id] : nameToId) {
    names[id.asIndex()] = name;
  }
  return names;
}

template <typename NestedMap>
std::vector<std::string> buildNameVectorFromNested(const NestedMap& nestedMap) {
  size_t totalSize = 0;
  for (const auto& [_, innerMapPtr] : nestedMap) {
    totalSize += innerMapPtr->size();
  }
  std::vector<std::string> names(totalSize);
  for (const auto& [_, innerMapPtr] : nestedMap) {
    for (const auto& [name, id] : *innerMapPtr) {
      names[id.asIndex()] = name;
    }
  }
  return names;
}

Scopes buildScopes(
    Map<ScopeId, std::shared_ptr<const Map<ScopeItemId, Set<ContainerId>>>>&&
        scopeIdToScopeItems,
    Map<ScopeId, Map<DimensionId, std::shared_ptr<const ScopeDimension>>>&&
        scopeIdToDimensions) {
  Map<ScopeId, std::unique_ptr<Scope>> scopeIdToScope;
  for (auto&& [scopeId, scopeItems] : scopeIdToScopeItems) {
    auto dimPtr = folly::get_ptr(scopeIdToDimensions, scopeId);
    auto scope = dimPtr
        ? std::make_unique<Scope>(std::move(scopeItems), std::move(*dimPtr))
        : std::make_unique<Scope>(
              std::move(scopeItems),
              Map<DimensionId, std::shared_ptr<const ScopeDimension>>{});
    scopeIdToScope.emplace(scopeId, std::move(scope));
  }

  return Scopes(std::move(scopeIdToScope));
}
} // namespace

folly::coro::Task<void> AsyncUniverseBuilder::buildTrivialScope() {
  const auto& containers = assignmentBuilder_.getContainers();
  const auto& containerNameToId = containers->containerNameToId;
  Map<std::string, std::vector<std::string>> trivialScopeItems;
  trivialScopeItems.reserve(containerNameToId.size());
  for (const auto& [containerName, _] : containerNameToId) {
    trivialScopeItems.emplace(
        containerName, std::vector<std::string>{containerName});
  }

  const auto trivialScopeId = scopesBuilder_.makeScopeId(containerTypeName_);
  co_return co_await addScope(trivialScopeId, std::move(trivialScopeItems));
}

folly::coro::Task<void> AsyncUniverseBuilder::buildObjectCountDimension(
    std::size_t totalObjects) {
  const auto objectCountDimensionId = dimensionsBuilder_.makeObjectDimensionId(
      fmt::format("{}_count", objectTypeName_));
  co_return co_await dimensionsBuilder_.add(
      objectCountDimensionId,
      ObjectDimensionData{
          .dimension =
              std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
                  totalObjects,
                  /*defaultValue=*/1.0,
                  /*expectedNonDefaultSize=*/0))});
}

folly::coro::Task<std::shared_ptr<const entities::Universe>>
AsyncUniverseBuilder::build() {
  auto
      [assignment,
       scopes,
       dimensions,
       partitions,
       goals,
       constraints,
       routingConfigs] =
          co_await folly::coro::collectAll(
              assignmentBuilder_.build(),
              scopesBuilder_.build(),
              dimensionsBuilder_.build(),
              partitionsBuilder_.build(),
              goalsBuilder_.build(),
              constraintsBuilder_.build(),
              routingConfigBuilder_.build());

  auto objects = std::make_unique<Objects>(
      assignment.numObjects,
      std::move(dimensions.dimensionIdToObjectDimension));

  auto containers = std::make_unique<Containers>(
      std::move(assignment.containerIdToObjectIds));

  auto scopesObj = buildScopes(
      std::move(scopes.scopeIdToScopeItemIdToContainerIds),
      std::move(dimensions.scopeIdToDimensionIdToDimension));

  auto idStore = IdStoreConfig{
      .objectNames = buildNameVector(assignment.objectNameToId),
      .containerNames = buildNameVector(assignment.containerNameToId),
      .scopeNames = buildNameVector(scopes.scopeNameToId),
      .scopeItemNames =
          buildNameVectorFromNested(scopes.scopeIdToScopeItemNameToId),
      .partitionNames = buildNameVector(partitions.partitionNameToId),
      .groupNames =
          buildNameVectorFromNested(partitions.partitionIdToGroupNameToId),
      .dimensionNames = buildNameVector(dimensions.dimensionNameToId),
      .goalNames = buildNameVector(goals.goalNameToId),
      .constraintNames = buildNameVector(constraints.constraintNameToId),
      .routingConfigNames =
          buildNameVector(routingConfigs.routingConfigNameToId),
      .objectNameToId = std::move(assignment.objectNameToId),
      .containerNameToId = std::move(assignment.containerNameToId),
      .scopeNameToId = std::move(scopes.scopeNameToId),
      .scopeIdToScopeItemNameToId =
          std::move(scopes.scopeIdToScopeItemNameToId),
      .dimensionNameToId = std::move(dimensions.dimensionNameToId),
      .partitionNameToId = std::move(partitions.partitionNameToId),
      .partitionIdToGroupNameToId =
          std::move(partitions.partitionIdToGroupNameToId),
      .goalNameToId = std::move(goals.goalNameToId),
      .constraintNameToId = std::move(constraints.constraintNameToId),
      .routingConfigNameToId = std::move(routingConfigs.routingConfigNameToId),
  };

  co_return std::make_shared<const entities::Universe>(UniverseConfig{
      .idStore = IdStore(std::move(idStore)),
      .objectTypeName = objectTypeName_,
      .containerTypeName = containerTypeName_,
      .objects = std::move(objects),
      .containers = std::move(containers),
      .scopes = std::move(scopesObj),
      .partitions = Partitions(std::move(partitions.partitionIdToPartition)),
      .goals = Goals(std::move(goals.goalIdToGoal)),
      .constraints =
          Constraints(std::move(constraints.constraintIdToConstraint)),
      .routingConfigIdToConfig =
          std::move(routingConfigs.routingConfigIdToConfig),
      .similarContainerIds = similarContainerIds_,
      .moveObjectsOnce = moveObjectsOnce_,
      .stableOptimization = stableOptimization_,
      .descendingHotnessContainers = descendingHotnessContainers_,
      .objectOrderingDimensionId = objectOrderingDimensionId_,
      .idToDestinationsToExploreOptions =
          *idToDestinationsToExploreOptions_.rlock(),
      .precision = precisionTolerances_,
  });
}

void AsyncUniverseBuilder::printSummary() const {
  XLOG(INFO) << getSummary();
}

std::string AsyncUniverseBuilder::getSummary() const {
  return folly::coro::blockingWait(buildSummaryString());
}

folly::coro::Task<std::string> AsyncUniverseBuilder::buildSummaryString()
    const {
  auto
      [assignment,
       dimensions,
       scopes,
       partitions,
       goals,
       constraints,
       routingConfig] =
          co_await folly::coro::collectAll(
              assignmentBuilder_.summarize(),
              dimensionsBuilder_.summarize(),
              scopesBuilder_.summarize(),
              partitionsBuilder_.summarize(),
              goalsBuilder_.summarize(),
              constraintsBuilder_.summarize(),
              routingConfigBuilder_.summarize());

  auto summary = fmt::format(
      "Objects: {}, Containers: {}\n{}{}{}{}{}{}{}",
      objectTypeName_,
      containerTypeName_,
      assignment,
      dimensions,
      scopes,
      partitions,
      goals,
      constraints,
      routingConfig);

  // removing trailing newline
  if (!summary.empty() && summary.back() == '\n') {
    summary.pop_back();
  }

  co_return summary;
}

} // namespace facebook::rebalancer::entities
