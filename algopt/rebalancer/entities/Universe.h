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

#include "algopt/rebalancer/entities/Constraints.h"
#include "algopt/rebalancer/entities/Containers.h"
#include "algopt/rebalancer/entities/Goals.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/Objects.h"
#include "algopt/rebalancer/entities/Partitions.h"
#include "algopt/rebalancer/entities/RoutingConfig.h"
#include "algopt/rebalancer/entities/Scopes.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/GraphState_types.h"
#include "algopt/rebalancer/solver/utils/Precision.h"

#include <memory>
#include <string>
#include <vector>

namespace facebook::rebalancer::entities {

struct UniverseConfig {
  IdStore idStore;
  std::string objectTypeName;
  std::string containerTypeName;
  std::unique_ptr<Objects> objects;
  std::unique_ptr<Containers> containers;
  Scopes scopes;
  Partitions partitions;
  Goals goals;
  Constraints constraints;
  Map<RoutingConfigId, std::shared_ptr<RoutingConfig>> routingConfigIdToConfig;
  std::optional<std::vector<std::vector<ContainerId>>> similarContainerIds;
  bool moveObjectsOnce;
  bool stableOptimization;
  std::vector<ContainerId> descendingHotnessContainers;
  std::optional<DimensionId> objectOrderingDimensionId;
  Map<std::string, interface::DestinationsToExploreOptions>
      idToDestinationsToExploreOptions;
  algopt::common::thrift::PrecisionTolerances precision;
};

class Universe {
 public:
  Universe() = default;

  explicit Universe(UniverseConfig&& config);

  explicit Universe(const thrift::Universe& universe);

  ~Universe() = default;

  // Delete the copy and assignment constructors to prevent accidental copies.
  Universe(const Universe&) = delete;
  Universe& operator=(const Universe&) = delete;
  Universe(Universe&&) = delete;
  Universe& operator=(Universe&&) = delete;

  // Getters.
  template <typename T>
  const std::string& getEntityName(T entityId) const {
    return idStore_.getEntityName(entityId);
  }

  ObjectId getObjectId(const std::string& objectName) const;
  ContainerId getContainerId(const std::string& containerName) const;
  ScopeId getScopeId(const std::string& scopeName) const;
  ScopeItemId getScopeItemId(ScopeId scopeId, const std::string& scopeItemName)
      const;
  PartitionId getPartitionId(const std::string& partitionName) const;
  GroupId getGroupId(PartitionId partitionId, const std::string& groupName)
      const;
  DimensionId getDimensionId(const std::string& dimensionName) const;
  GoalId getGoalId(const std::string& goalName) const;
  ConstraintId getConstraintId(const std::string& constraintName) const;
  RoutingConfigId getRoutingConfigId(
      const std::string& routingConfigName) const;

  const std::string& getObjectTypeName() const;
  const std::string& getContainerTypeName() const;

  const Containers& getContainers() const;
  const Objects& getObjects() const;

  EntityIdType getNumObjects() const {
    return objects_->getNumObjects();
  }

  auto getScopeIds() const {
    return scopes_.getScopeIds();
  }

  const Scope& getScope(ScopeId scopeId) const;

  auto getPartitionIds() const {
    return partitions_.getPartitionIds();
  }

  const Partition& getPartition(PartitionId partitionId) const;
  const Goals& getGoals() const;
  const Constraints& getConstraints() const;
  const std::optional<DimensionId>& getObjectOrderingDimensionId() const;
  bool getStableOptimization() const;
  bool getMoveObjectsOnce() const;

  const std::vector<ContainerId>& getDescendingHotnessContainers() const;

  const entities::RoutingConfig& getRoutingConfig(
      entities::RoutingConfigId routingConfigId) const;

  const interface::DestinationsToExploreOptions&
  getDestinationsToExploreOptions(
      const std::string& destinationsToExploreOptionsName) const;

  const Precision& getPrecision() const;

  const std::optional<std::vector<std::vector<ContainerId>>>&
  getSimilarContainers() const;

  bool existsPartition(const std::string& partitionName) const;

  thrift::Universe toThrift() const;

  // Create a graph embedding of the universe for use in GNNs.
  // This is an experimental capability to enhance heuristic decisions.
  thrift::GraphState buildGraph() const;

 private:
  IdStore idStore_;
  std::string objectTypeName_;
  std::string containerTypeName_;
  std::unique_ptr<Objects> objects_;
  std::unique_ptr<Containers> containers_;
  std::optional<std::vector<std::vector<ContainerId>>> similarContainerIds_;
  Scopes scopes_;
  Partitions partitions_;
  Goals goals_;
  Constraints constraints_;
  bool stableOptimization_ = false;
  bool moveObjectsOnce_ = false;
  std::vector<ContainerId> descendingHotnessContainers_;
  std::optional<DimensionId> objectOrderingDimensionId_ = std::nullopt;
  Map<RoutingConfigId, std::shared_ptr<RoutingConfig>>
      routingConfigIdToRoutingConfig_;
  Map<std::string, interface::DestinationsToExploreOptions>
      idToDestinationsToExploreOptions_;
  Precision precision_ =
      Precision(algopt::common::thrift::PrecisionTolerances());
};

} // namespace facebook::rebalancer::entities
