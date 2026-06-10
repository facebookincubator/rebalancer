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

#pragma once

#include "algopt/rebalancer/entities/builders/AssignmentBuilder.h"
#include "algopt/rebalancer/entities/builders/ConstraintsBuilder.h"
#include "algopt/rebalancer/entities/builders/DimensionsBuilder.h"
#include "algopt/rebalancer/entities/builders/GoalsBuilder.h"
#include "algopt/rebalancer/entities/builders/IdBuilder.h"
#include "algopt/rebalancer/entities/builders/PartitionsBuilder.h"
#include "algopt/rebalancer/entities/builders/RoutingConfigsBuilder.h"
#include "algopt/rebalancer/entities/builders/ScopesBuilder.h"
#include "algopt/rebalancer/entities/Universe.h"

#include <folly/container/F14Map.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/coro/Task.h>
#include <folly/futures/SharedPromise.h>
#include <folly/Synchronized.h>
#include <folly/SynchronizedPtr.h>

namespace facebook::rebalancer::entities {

class AsyncUniverseBuilder {
 public:
  // -------------------------------------------------------------
  // Assignment
  // -------------------------------------------------------------
  void setObjectTypeName(const std::string& objectTypeName) {
    objectTypeName_ = objectTypeName;
  }
  void setContainerTypeName(const std::string& containerTypeName) {
    containerTypeName_ = containerTypeName;
  }
  template <typename ContainerToObjects>
  void setInitialAssignment(
      const ContainerToObjects& containerNameToObjectNames)
    requires IsIterableOverPairs<
        ContainerToObjects,
        std::string,
        std::vector<std::string>>;

  std::shared_ptr<const entities::ObjectsData> getObjects() const {
    return assignmentBuilder_.getObjects();
  }
  std::shared_ptr<const entities::ContainersData> getContainers() const {
    return assignmentBuilder_.getContainers();
  }

  const std::string& getObjectTypeName() const {
    return objectTypeName_;
  }
  const std::string& getContainerTypeName() const {
    return containerTypeName_;
  }

  // -------------------------------------------------------------
  // Id creation
  // -------------------------------------------------------------
  ScopeId makeScopeId(const std::string& scopeName) {
    return scopesBuilder_.makeScopeId(scopeName);
  }
  DimensionId makeObjectDimensionId(const std::string& dimensionName) {
    return dimensionsBuilder_.makeObjectDimensionId(dimensionName);
  }
  DimensionId makeScopeDimensionId(
      const std::string& dimensionName,
      ScopeId scopeId) {
    return dimensionsBuilder_.makeScopeDimensionId(dimensionName, scopeId);
  }
  PartitionId makePartitionId(const std::string& partitionName) {
    return partitionsBuilder_.makePartitionId(partitionName);
  }
  GoalId makeGoalId(const std::string& goalName) {
    return goalsBuilder_.makeGoalId(goalName);
  }
  ConstraintId makeConstraintId(const std::string& constraintName) {
    return constraintsBuilder_.makeConstraintId(constraintName);
  }
  RoutingConfigId makeRoutingConfigId(const std::string& routingConfigName) {
    return routingConfigBuilder_.makeRoutingConfigId(routingConfigName);
  }

  // -------------------------------------------------------------
  // Id lookup
  // -------------------------------------------------------------
  ScopeId getScopeId(const std::string& scopeName) const {
    return scopesBuilder_.getScopeId(scopeName);
  }
  DimensionId getDimensionId(const std::string& dimensionName) const {
    return dimensionsBuilder_.getDimensionId(dimensionName);
  }
  PartitionId getPartitionId(const std::string& partitionName) const {
    return partitionsBuilder_.getPartitionId(partitionName);
  }
  RoutingConfigId getRoutingConfigId(
      const std::string& routingConfigName) const {
    return routingConfigBuilder_.getRoutingConfigId(routingConfigName);
  }

  // -------------------------------------------------------------
  // Scopes
  // -------------------------------------------------------------
  template <typename ScopeItemToContainers>
  folly::coro::Task<void> addScope(
      ScopeId scopeId,
      ScopeItemToContainers scopeItemToContainers)
    requires IsIterableOverPairs<
        ScopeItemToContainers,
        std::string,
        std::vector<std::string>>
  {
    co_return co_await scopesBuilder_.add(
        scopeId, std::move(scopeItemToContainers), getContainers());
  }

  template <typename ContainerToScopeItem>
  folly::coro::Task<void> addScope(
      ScopeId scopeId,
      ContainerToScopeItem containerToScopeItem)
    requires IsIterableOverPairs<ContainerToScopeItem, std::string, std::string>
  {
    co_return co_await scopesBuilder_.add(
        scopeId, std::move(containerToScopeItem), getContainers());
  }

  folly::coro::Task<std::shared_ptr<const ScopeData>> getScope(
      ScopeId id) const {
    co_return co_await scopesBuilder_.get(id);
  }

  // -------------------------------------------------------------
  // Dimensions
  // -------------------------------------------------------------
  folly::coro::Task<void> addObjectDimension(
      DimensionId dimensionId,
      ObjectDimensionData dimension) {
    co_return co_await dimensionsBuilder_.add(
        dimensionId, std::move(dimension));
  }

  folly::coro::Task<void> addScopeDimension(
      DimensionId dimensionId,
      ScopeId scopeId,
      ScopeDimensionData dimension) {
    co_return co_await dimensionsBuilder_.add(
        dimensionId, scopeId, std::move(dimension));
  }

  // -------------------------------------------------------------
  // Partitions
  // -------------------------------------------------------------
  template <typename GroupToObjects>
  folly::coro::Task<void> addPartition(
      PartitionId partitionId,
      GroupToObjects groupNameToObjectNames)
    requires IsIterableOverPairs<
        GroupToObjects,
        std::string,
        std::vector<std::string>>
  {
    co_return co_await partitionsBuilder_.add(
        partitionId, std::move(groupNameToObjectNames), getObjects());
  }

  template <typename ObjectToGroup>
  folly::coro::Task<void> addPartition(
      PartitionId partitionId,
      ObjectToGroup objectNameToGroupName)
    requires IsIterableOverPairs<ObjectToGroup, std::string, std::string>
  {
    co_return co_await partitionsBuilder_.add(
        partitionId, std::move(objectNameToGroupName), getObjects());
  }

  folly::coro::Task<std::shared_ptr<const PartitionData>> getPartition(
      PartitionId partitionId) const {
    co_return co_await partitionsBuilder_.get(partitionId);
  }

  // -------------------------------------------------------------
  // Goals and Constraints
  // -------------------------------------------------------------
  folly::coro::Task<void> addGoal(GoalId goalId, GoalData goalData) {
    co_return co_await goalsBuilder_.add(goalId, std::move(goalData));
  }

  folly::coro::Task<void> addConstraint(
      ConstraintId constraintId,
      ConstraintData constraintData) {
    co_return co_await constraintsBuilder_.add(
        constraintId, std::move(constraintData));
  }

  // -------------------------------------------------------------
  // RoutingConfigs
  // -------------------------------------------------------------
  folly::coro::Task<void> addRoutingConfig(
      RoutingConfigId routingConfigId,
      RoutingConfigData routingConfigData) {
    co_return co_await routingConfigBuilder_.add(
        routingConfigId, std::move(routingConfigData));
  }
  folly::coro::Task<std::shared_ptr<const RoutingConfigData>> getRoutingConfig(
      RoutingConfigId routingConfigId) const {
    co_return co_await routingConfigBuilder_.get(routingConfigId);
  }

  // -------------------------------------------------------------
  // Other configs
  // -------------------------------------------------------------
  void setSimilarContainers(
      std::vector<std::vector<ContainerId>> similarContainers) {
    similarContainerIds_ = std::move(similarContainers);
  }

  void setMoveObjectsOnce(bool moveObjectsOnce) {
    moveObjectsOnce_ = moveObjectsOnce;
  }

  void setStableOptimization(bool stableOptimization) {
    stableOptimization_ = stableOptimization;
  }

  void setDescendingHotnessContainers(
      std::vector<ContainerId> descendingHotnessContainers) {
    descendingHotnessContainers_ = std::move(descendingHotnessContainers);
  }

  void setObjectOrderingDimensionId(DimensionId objectOrderingDimensionId) {
    objectOrderingDimensionId_.emplace(objectOrderingDimensionId);
  }

  void addDestinationsToExploreOptions(
      const std::string& name,
      interface::DestinationsToExploreOptions destinationsToExploreOptions) {
    idToDestinationsToExploreOptions_.wlock()->emplace(
        name, std::move(destinationsToExploreOptions));
  }

  void setPrecision(
      algopt::common::thrift::PrecisionTolerances precisionTolerances) {
    precisionTolerances_ = std::move(precisionTolerances);
  }

  // -------------------------------------------------------------
  // Build
  // -------------------------------------------------------------
  folly::coro::Task<std::shared_ptr<const Universe>> build();

  void printSummary() const;
  std::string getSummary() const;

 private:
  template <typename ContainerToObjects>
  folly::coro::Task<void> setInitialAssignmentImpl(
      const ContainerToObjects& containerNameToObjectNames);

  folly::coro::Task<void> buildTrivialScope();

  folly::coro::Task<void> buildObjectCountDimension(std::size_t totalObjects);

  folly::coro::Task<std::string> buildSummaryString() const;

 private:
  IdBuilder idBuilder_;
  std::string objectTypeName_;
  std::string containerTypeName_;
  AssignmentBuilder assignmentBuilder_{idBuilder_};
  ScopesBuilder scopesBuilder_{idBuilder_};
  DimensionsBuilder dimensionsBuilder_{idBuilder_};
  PartitionsBuilder partitionsBuilder_{idBuilder_};
  GoalsBuilder goalsBuilder_{idBuilder_};
  ConstraintsBuilder constraintsBuilder_{idBuilder_};
  RoutingConfigsBuilder routingConfigBuilder_{idBuilder_};

  std::optional<std::vector<std::vector<ContainerId>>> similarContainerIds_;
  bool moveObjectsOnce_ = false;
  bool stableOptimization_ = false;
  std::vector<ContainerId> descendingHotnessContainers_;
  std::optional<DimensionId> objectOrderingDimensionId_;
  folly::Synchronized<Map<std::string, interface::DestinationsToExploreOptions>>
      idToDestinationsToExploreOptions_;
  algopt::common::thrift::PrecisionTolerances precisionTolerances_;
};

// implementations
template <typename ContainerToObjects>
void AsyncUniverseBuilder::setInitialAssignment(
    const ContainerToObjects& containerNameToObjectNames)
  requires IsIterableOverPairs<
      ContainerToObjects,
      std::string,
      std::vector<std::string>>
{
  folly::coro::blockingWait(
      setInitialAssignmentImpl(containerNameToObjectNames));
}

template <typename ContainerToObjects>
folly::coro::Task<void> AsyncUniverseBuilder::setInitialAssignmentImpl(
    const ContainerToObjects& containerNameToObjectNames) {
  const bool isFirstAssignment = !assignmentBuilder_.wasPreviouslySet();
  assignmentBuilder_.set(containerNameToObjectNames);
  if (isFirstAssignment) {
    const auto totalObjects = assignmentBuilder_.getObjects()->numObjects;
    co_await folly::coro::collectAll(
        buildTrivialScope(), buildObjectCountDimension(totalObjects));
  }
  co_return;
}

} // namespace facebook::rebalancer::entities
