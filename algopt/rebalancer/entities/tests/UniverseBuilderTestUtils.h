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

#include "algopt/rebalancer/entities/builders/AsyncUniverseBuilder.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/Universe.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>

#include <memory>
#include <string>
#include <vector>

namespace facebook::rebalancer::entities::tests {

class UniverseBuilderTestUtils {
  constexpr static const char* kObjectName = "object";
  constexpr static const char* kContainerName = "container";

 public:
  UniverseBuilderTestUtils(
      const std::string& objectName = kObjectName,
      const std::string& containerName = kContainerName);

  virtual ~UniverseBuilderTestUtils() = default;

  ObjectId object(int i) const;
  ContainerId container(int i) const;
  ObjectId object(const std::string& name) const;
  ContainerId container(const std::string& name) const;
  ObjectId objectId(const std::string& name) const {
    return object(name);
  }
  ContainerId containerId(const std::string& name) const {
    return container(name);
  }

  ScopeId scopeId(const std::string& name) const;
  ScopeItemId scopeItemId(ScopeId scopeId, const std::string& name) const;
  ScopeItemId scopeItemId(
      const std::string& scopeName,
      const std::string& scopeItemName) const;
  PartitionId partitionId(const std::string& name) const;
  DimensionId dimensionId(const std::string& name) const;
  GroupId groupId(PartitionId partition, const std::string& name) const;
  GroupId groupId(
      const std::string& partitionName,
      const std::string& groupName) const;
  RoutingConfigId routingConfigId(const std::string& name) const;

  std::shared_ptr<const Universe> buildUniverse();
  std::shared_ptr<const Universe> getUniversePtr() const;
  const entities::Universe& getUniverse() const;

  template <
      typename ContainerToObjects = Map<std::string, std::vector<std::string>>>
  void setInitialAssignment(const ContainerToObjects& containerToObjects)
    requires IsIterableOverPairs<
        ContainerToObjects,
        std::string,
        std::vector<std::string>>
  {
    universeBuilder_.setInitialAssignment(containerToObjects);
  }

  folly::coro::Task<void> addPartition(
      const std::string& partitionName,
      const Map<std::string, std::vector<std::string>>& groupToObjects);

  folly::coro::Task<void> addScopeDimension(
      const std::string& dimensionName,
      ScopeId scopeId,
      const Map<std::string, double>& scopeItemNameToValue,
      double defaultValue);

  folly::coro::Task<void> addRoutingConfig(
      const std::string& name,
      RoutingConfigData routingConfigData);

  folly::coro::Task<void> addScope(
      const std::string& scopeName,
      const Map<std::string, std::vector<std::string>>& scopeItemToContainers);

  folly::coro::Task<void> addObjectDimension(
      const std::string& dimensionName,
      ObjectDimensionData objectDimensionData);

  folly::coro::Task<void> addObjectDimension(
      const std::string& dimensionName,
      const Map<std::string, double>& objectNameToValue,
      double defaultValue);

  folly::coro::Task<void> addObjectDimension(
      const std::string& dimensionName,
      const Map<ObjectId, double>& objectToValue,
      double defaultValue);

  folly::coro::Task<void> addObjectDimension(
      const std::string& dimensionName,
      const std::vector<Map<ObjectId, double>>& values,
      const std::vector<double>& defaultValues);

  folly::coro::Task<void> addDynamicObjectDimension(
      const std::string& dimensionName,
      ScopeId scopeId,
      const Map<std::string, Map<std::string, double>>&
          scopeItemNameToObjectToValue,
      double defaultValue);

  folly::coro::Task<void> addDynamicObjectDimension(
      const std::string& dimensionName,
      ScopeId scopeId,
      const Map<std::string, std::shared_ptr<const Map<ObjectId, double>>>&
          scopeItemNameToObjectValues,
      double defaultValue);

  void addDestinationsToExploreOptions(
      const std::string& name,
      interface::DestinationsToExploreOptions options);

  folly::coro::Task<void> addGoal(
      const std::string& name,
      interface::GoalSpecs spec,
      double weight,
      int tupleIndex);

  folly::coro::Task<void> addConstraint(
      const std::string& name,
      interface::ConstraintSpecs spec,
      interface::ConstraintPolicy policy,
      double invalidCost,
      double invalidState,
      int tupleIndex);

  std::size_t getNumObjects() const {
    return universeBuilder_.getObjects()->numObjects;
  }

 protected:
  AsyncUniverseBuilder universeBuilder_;
  std::shared_ptr<const Universe> universe_;
};

} // namespace facebook::rebalancer::entities::tests
