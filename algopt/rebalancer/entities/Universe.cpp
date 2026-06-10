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

#include "algopt/rebalancer/entities/Universe.h"

#include "algopt/rebalancer/entities/GraphEmbeddingUtils.h"

#include <fmt/core.h>
#include <folly/container/F14Map.h>
#include <folly/container/irange.h>

#include <cassert>
#include <stdexcept>
#include <utility>

namespace facebook::rebalancer::entities {

using namespace facebook::rebalancer::interface;

Universe::Universe(UniverseConfig&& config)
    : idStore_(std::move(config.idStore)),
      objectTypeName_(std::move(config.objectTypeName)),
      containerTypeName_(std::move(config.containerTypeName)),
      objects_(std::move(config.objects)),
      containers_(std::move(config.containers)),
      similarContainerIds_(std::move(config.similarContainerIds)),
      scopes_(std::move(config.scopes)),
      partitions_(std::move(config.partitions)),
      goals_(std::move(config.goals)),
      constraints_(std::move(config.constraints)),
      stableOptimization_(config.stableOptimization),
      moveObjectsOnce_(config.moveObjectsOnce),
      descendingHotnessContainers_(
          std::move(config.descendingHotnessContainers)),
      objectOrderingDimensionId_(std::move(config.objectOrderingDimensionId)),
      routingConfigIdToRoutingConfig_(
          std::move(config.routingConfigIdToConfig)),
      idToDestinationsToExploreOptions_(
          std::move(config.idToDestinationsToExploreOptions)),
      precision_(std::move(config.precision)) {
  if (!objects_) {
    throw std::runtime_error("Expected 'objects' to be non-null");
  }
  if (!containers_) {
    throw std::runtime_error("Expected 'containers' to be non-null");
  }
}

Universe::Universe(const thrift::Universe& universe)
    : idStore_(*universe.idStore()),
      objectTypeName_(*universe.objectTypeName()),
      containerTypeName_(*universe.containerTypeName()),
      containers_(std::make_unique<Containers>(*universe.containers())),
      scopes_(*universe.scopes()),
      partitions_(*universe.partitions()),
      goals_(*universe.goals()),
      constraints_(*universe.constraints()),
      stableOptimization_(*universe.stableOptimization()),
      moveObjectsOnce_(*universe.moveObjectsOnce()),
      precision_(Precision(*universe.precisionTolerances())) {
  objects_ = std::make_unique<Objects>(*universe.objects(), &partitions_);

  if (universe.similarContainerIds()) {
    std::vector<std::vector<ContainerId>> similarContainerIds;
    similarContainerIds.reserve(universe.similarContainerIds()->size());
    for (auto& containers : *universe.similarContainerIds()) {
      std::vector<ContainerId> containerIds;
      containerIds.reserve(containers.size());
      for (auto containerId : containers) {
        containerIds.emplace_back(containerId);
      }
      similarContainerIds.push_back(std::move(containerIds));
    }
    similarContainerIds_ = std::move(similarContainerIds);
  }

  descendingHotnessContainers_.reserve(
      universe.descendingHotnessContainers()->size());
  for (auto containerId : *universe.descendingHotnessContainers()) {
    descendingHotnessContainers_.emplace_back(containerId);
  }

  if (universe.objectOrderingDimensionId()) {
    objectOrderingDimensionId_ =
        DimensionId(*universe.objectOrderingDimensionId());
  }

  for (auto& [configId, thriftRoutingConfig] :
       *universe.routingConfigIdToRoutingConfig()) {
    routingConfigIdToRoutingConfig_.emplace(
        RoutingConfigId(configId),
        std::make_shared<RoutingConfig>(thriftRoutingConfig));
  }

  for (auto& [destinationsToExploreOptionsName, destinationsToExplore] :
       *universe.idToDestinationToExploreOptions()) {
    idToDestinationsToExploreOptions_.emplace(
        destinationsToExploreOptionsName,
        interface::DestinationsToExploreOptions(destinationsToExplore));
  }
}

ObjectId Universe::getObjectId(const std::string& objectName) const {
  return idStore_.getObjectId(objectName);
}

ContainerId Universe::getContainerId(const std::string& containerName) const {
  return idStore_.getContainerId(containerName);
}

ScopeId Universe::getScopeId(const std::string& scopeName) const {
  return idStore_.getScopeId(scopeName);
}

ScopeItemId Universe::getScopeItemId(
    ScopeId scopeId,
    const std::string& scopeItemName) const {
  return idStore_.getScopeItemId(scopeId, scopeItemName);
}

PartitionId Universe::getPartitionId(const std::string& partitionName) const {
  return idStore_.getPartitionId(partitionName);
}

GroupId Universe::getGroupId(
    PartitionId partitionId,
    const std::string& groupName) const {
  return idStore_.getGroupId(partitionId, groupName);
}

DimensionId Universe::getDimensionId(const std::string& dimensionName) const {
  return idStore_.getDimensionId(dimensionName);
}

GoalId Universe::getGoalId(const std::string& goalName) const {
  return idStore_.getGoalId(goalName);
}

ConstraintId Universe::getConstraintId(
    const std::string& constraintName) const {
  return idStore_.getConstraintId(constraintName);
}

RoutingConfigId Universe::getRoutingConfigId(
    const std::string& configName) const {
  return idStore_.getRoutingConfigId(configName);
}

const interface::DestinationsToExploreOptions&
Universe::getDestinationsToExploreOptions(
    const std::string& destinationsToExploreOptionsName) const {
  return idToDestinationsToExploreOptions_.at(destinationsToExploreOptionsName);
}

const std::string& Universe::getObjectTypeName() const {
  return objectTypeName_;
}

const std::string& Universe::getContainerTypeName() const {
  return containerTypeName_;
}

const Containers& Universe::getContainers() const {
  return *containers_;
}

const Objects& Universe::getObjects() const {
  return *objects_;
}

const Scope& Universe::getScope(ScopeId scopeId) const {
  return scopes_.getScope(scopeId);
}

const Partition& Universe::getPartition(PartitionId partitionId) const {
  return partitions_.getPartition(partitionId);
}

const Goals& Universe::getGoals() const {
  return goals_;
}

const Constraints& Universe::getConstraints() const {
  return constraints_;
}

const std::optional<DimensionId>& Universe::getObjectOrderingDimensionId()
    const {
  return objectOrderingDimensionId_;
}

bool Universe::getStableOptimization() const {
  return stableOptimization_;
}

bool Universe::getMoveObjectsOnce() const {
  return moveObjectsOnce_;
}

const std::vector<ContainerId>& Universe::getDescendingHotnessContainers()
    const {
  return descendingHotnessContainers_;
}

const Precision& Universe::getPrecision() const {
  return precision_;
}

bool Universe::existsPartition(const std::string& partitionName) const {
  return idStore_.existsPartition(partitionName);
}

const std::optional<std::vector<std::vector<ContainerId>>>&
Universe::getSimilarContainers() const {
  return similarContainerIds_;
}

const entities::RoutingConfig& Universe::getRoutingConfig(
    entities::RoutingConfigId routingConfigId) const {
  auto infoPtr =
      folly::get_ptr(routingConfigIdToRoutingConfig_, routingConfigId);
  if (!infoPtr) {
    throw std::runtime_error(
        fmt::format(
            "Routing config '{}' not found", getEntityName(routingConfigId)));
  }

  return **infoPtr;
}

thrift::Universe Universe::toThrift() const {
  thrift::Universe universe;
  universe.idStore() = idStore_.toThrift();
  universe.objectTypeName() = objectTypeName_;
  universe.containerTypeName() = containerTypeName_;
  universe.objects() = objects_->toThrift();
  universe.containers() = containers_->toThrift();

  if (similarContainerIds_) {
    universe.similarContainerIds().emplace();
    assert(universe.similarContainerIds());
    universe.similarContainerIds().value().reserve(
        similarContainerIds_->size());
    for (const auto& containerIds : *similarContainerIds_) {
      universe.similarContainerIds().value().emplace_back(
          entities::asIntsVec(containerIds));
    }
  }

  universe.scopes() = scopes_.toThrift();
  universe.partitions() = partitions_.toThrift();
  universe.goals() = goals_.toThrift();
  universe.constraints() = constraints_.toThrift();
  universe.stableOptimization() = stableOptimization_;
  universe.moveObjectsOnce() = moveObjectsOnce_;
  universe.descendingHotnessContainers() =
      asIntsVec(descendingHotnessContainers_);

  if (objectOrderingDimensionId_) {
    universe.objectOrderingDimensionId() = objectOrderingDimensionId_->asInt();
  }

  for (auto& [configId, routingConfig] : routingConfigIdToRoutingConfig_) {
    universe.routingConfigIdToRoutingConfig()->emplace(
        configId.asInt(), routingConfig->toThrift());
  }

  for (auto& [destinationsToExploreOptionsName, destinationsToExploreOptions] :
       idToDestinationsToExploreOptions_) {
    universe.idToDestinationToExploreOptions()->emplace(
        destinationsToExploreOptionsName, destinationsToExploreOptions);
  }

  universe.precisionTolerances() = precision_.getTolerances();

  return universe;
}

namespace {

std::vector<DimensionId> collectSortedDimensionIds(const Objects& objects) {
  std::vector<DimensionId> dimIds;
  for (const auto dimId : objects.getDimensionIds()) {
    dimIds.push_back(dimId);
  }
  std::sort(dimIds.begin(), dimIds.end());
  return dimIds;
}

void buildVertexIndex(
    const std::vector<thrift::GraphVertex>& vertices,
    int64_t offset,
    folly::F14FastMap<int64_t, int64_t>& index) {
  index.reserve(vertices.size());
  for (const auto i : folly::irange(vertices.size())) {
    index[*vertices[i].entityId()] = offset + static_cast<int64_t>(i);
  }
}

void padEmbeddings(
    std::vector<thrift::GraphVertex>& vertices,
    std::size_t targetSize) {
  for (auto& v : vertices) {
    v.embedding()->resize(targetSize, 0.0);
  }
}

// Object IDs are dense and contiguous from 0 (see Objects.h), so
// objectId.asInt() is the vertex index directly — no index map needed.
void appendAssignmentEdges(
    const Containers& containers,
    const folly::F14FastMap<int64_t, int64_t>& containerVertexIndex,
    std::vector<thrift::GraphEdge>& edges) {
  for (const auto& [containerId, objectIds] :
       containers.getInitialAssignment()) {
    const auto cIt = containerVertexIndex.find(containerId.asInt());
    if (cIt == containerVertexIndex.end()) {
      throw std::runtime_error(
          fmt::format(
              "Container {} missing from vertex index", containerId.asInt()));
    }
    for (const auto objectId : objectIds) {
      thrift::GraphEdge edge;
      edge.sourceIndex() = static_cast<int64_t>(objectId.asInt());
      edge.targetIndex() = cIt->second;
      edges.push_back(std::move(edge));
    }
  }
}

void appendScopeMembershipEdges(
    const Scope& scope,
    const folly::F14FastMap<int64_t, int64_t>& containerVertexIndex,
    const folly::F14FastMap<int64_t, int64_t>& scopeItemVertexIndex,
    std::vector<thrift::GraphEdge>& edges) {
  for (const auto scopeItemId : scope.getScopeItemIds()) {
    const auto sIt = scopeItemVertexIndex.find(scopeItemId.asInt());
    if (sIt == scopeItemVertexIndex.end()) {
      throw std::runtime_error(
          fmt::format(
              "ScopeItem {} missing from vertex index", scopeItemId.asInt()));
    }
    for (const auto containerId : scope.getContainerIds(scopeItemId)) {
      const auto cIt = containerVertexIndex.find(containerId.asInt());
      if (cIt == containerVertexIndex.end()) {
        throw std::runtime_error(
            fmt::format(
                "Container {} missing from vertex index", containerId.asInt()));
      }
      thrift::GraphEdge edge;
      edge.sourceIndex() = cIt->second;
      edge.targetIndex() = sIt->second;
      edges.push_back(std::move(edge));
    }
  }
}

void appendPartitionMembershipEdges(
    const Partition& partition,
    const folly::F14FastMap<int64_t, int64_t>& groupVertexIndex,
    std::vector<thrift::GraphEdge>& edges) {
  for (const auto& [objectId, groupIds] : partition.getObjectIdToGroupIds()) {
    for (const auto groupId : groupIds) {
      const auto gIt = groupVertexIndex.find(groupId.asInt());
      if (gIt == groupVertexIndex.end()) {
        throw std::runtime_error(
            fmt::format("Group {} missing from vertex index", groupId.asInt()));
      }
      thrift::GraphEdge edge;
      // Object IDs are dense from 0, same invariant as appendAssignmentEdges.
      edge.sourceIndex() = static_cast<int64_t>(objectId.asInt());
      edge.targetIndex() = gIt->second;
      edges.push_back(std::move(edge));
    }
  }
}

} // namespace

thrift::GraphState Universe::buildGraph() const {
  thrift::GraphState graph;

  const auto dimIds = collectSortedDimensionIds(getObjects());
  const auto embeddingDim = kNumVertexTypes + dimIds.size();

  graph.dimensionNames()->reserve(dimIds.size());
  for (const auto dimId : dimIds) {
    graph.dimensionNames()->push_back(getEntityName(dimId));
  }
  graph.embeddingDimension() = static_cast<int32_t>(embeddingDim);

  // --- encode vertices ---
  auto objectVertices = getObjects().toEmbedding(dimIds);
  const int64_t objectCount = static_cast<int64_t>(objectVertices.size());

  auto containerVertices = getContainers().toEmbedding();
  padEmbeddings(containerVertices, embeddingDim);
  folly::F14FastMap<int64_t, int64_t> containerVertexIndex;
  buildVertexIndex(containerVertices, objectCount, containerVertexIndex);
  const int64_t containerCount = static_cast<int64_t>(containerVertices.size());

  std::vector<thrift::GraphVertex> scopeItemVertices;
  folly::F14FastMap<int64_t, int64_t> scopeItemVertexIndex;
  for (const auto scopeId : getScopeIds()) {
    auto scopeVerts = getScope(scopeId).toEmbedding(dimIds);
    for (auto& v : scopeVerts) {
      const int64_t idx = objectCount + containerCount +
          static_cast<int64_t>(scopeItemVertices.size());
      scopeItemVertexIndex[*v.entityId()] = idx;
      scopeItemVertices.push_back(std::move(v));
    }
  }
  const int64_t scopeItemCount = static_cast<int64_t>(scopeItemVertices.size());

  std::vector<thrift::GraphVertex> groupVertices;
  folly::F14FastMap<int64_t, int64_t> groupVertexIndex;
  for (const auto partitionId : getPartitionIds()) {
    auto groupVerts = getPartition(partitionId).toEmbedding();
    padEmbeddings(groupVerts, embeddingDim);
    for (auto& v : groupVerts) {
      const int64_t idx = objectCount + containerCount + scopeItemCount +
          static_cast<int64_t>(groupVertices.size());
      groupVertexIndex[*v.entityId()] = idx;
      groupVertices.push_back(std::move(v));
    }
  }

  // --- assemble vertices ---
  auto& allVertices = *graph.vertices();
  allVertices.reserve(
      objectVertices.size() + containerVertices.size() +
      scopeItemVertices.size() + groupVertices.size());
  for (auto& v : objectVertices) {
    allVertices.push_back(std::move(v));
  }
  for (auto& v : containerVertices) {
    allVertices.push_back(std::move(v));
  }
  for (auto& v : scopeItemVertices) {
    allVertices.push_back(std::move(v));
  }
  for (auto& v : groupVertices) {
    allVertices.push_back(std::move(v));
  }

  // --- build edges ---
  // Compute exact edge count from source data.
  size_t edgeCount = objectVertices.size(); // one assignment edge per object
  for (const auto scopeId : getScopeIds()) {
    const auto& scope = getScope(scopeId);
    for (const auto scopeItemId : scope.getScopeItemIds()) {
      edgeCount += scope.getContainerIds(scopeItemId).size();
    }
  }
  for (const auto partitionId : getPartitionIds()) {
    for (const auto& [objectId, groupIds] :
         getPartition(partitionId).getObjectIdToGroupIds()) {
      edgeCount += groupIds.size();
    }
  }
  auto& edges = *graph.edges();
  edges.reserve(edgeCount);
  appendAssignmentEdges(getContainers(), containerVertexIndex, edges);
  for (const auto scopeId : getScopeIds()) {
    appendScopeMembershipEdges(
        getScope(scopeId), containerVertexIndex, scopeItemVertexIndex, edges);
  }
  for (const auto partitionId : getPartitionIds()) {
    appendPartitionMembershipEdges(
        getPartition(partitionId), groupVertexIndex, edges);
  }

  return graph;
}

} // namespace facebook::rebalancer::entities
