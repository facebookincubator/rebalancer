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
#include "algopt/rebalancer/entities/builders/tests/BuilderTestBase.h"

#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/portability/GTest.h>
#include <gtest/gtest.h>

#include <stdexcept>

namespace facebook::rebalancer::entities::tests {

class AsyncUniverseBuilderTest : public BuilderTestBase {
 protected:
  void SetUp() override {
    BuilderTestBase::SetUp();

    // Set object and container type names.
    universe.setObjectTypeName("task");
    universe.setContainerTypeName("host");

    // Set initial assignment.
    const Map<std::string, std::vector<std::string>> assignment{
        {"c1", {"o1", "o2"}},
        {"c2", {"o3"}},
        {"c3", {}},
    };
    universe.setInitialAssignment(assignment);
  }

  size_t objectSize() const {
    return universe.getObjects()->numObjects;
  }

  AsyncUniverseBuilder universe;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    AsyncUniverseBuilderTest,
    ::testing::Values(1, 3, 12, 60));

CO_TEST_P(AsyncUniverseBuilderTest, AddScopeItemToContainers) {
  const Map<std::string, std::vector<std::string>> regionScope{
      {"region-1", {"c1", "c2"}},
      {"region-2", {"c3"}},
  };
  const auto regionId = universe.makeScopeId("region");
  co_await universe.addScope(regionId, regionScope);

  const auto regionScopeData = co_await universe.getScope(regionId);
  EXPECT_EQ(2, regionScopeData->scopeItemNameToId->size());
  const auto region1Id = regionScopeData->getScopeItemId("region-1");
  const auto& region1Containers =
      regionScopeData->scopeItemIdToContainerIds->at(region1Id);
  EXPECT_EQ(region1Containers.size(), 2);
}

CO_TEST_P(AsyncUniverseBuilderTest, AddContainerToScopeItems) {
  const auto scopeId = universe.makeScopeId("region");
  const Map<std::string, std::string> containerToRegion{
      {"c1", "region-1"},
      {"c2", "region-1"},
      {"c3", "region-2"},
  };
  co_await universe.addScope(scopeId, containerToRegion);

  const auto scopeData = co_await universe.getScope(scopeId);

  EXPECT_EQ(2, scopeData->scopeItemNameToId->size());
  EXPECT_EQ(2, scopeData->scopeItemIdToContainerIds->size());
  const auto region1Id = scopeData->getScopeItemId("region-1");
  const auto region2Id = scopeData->getScopeItemId("region-2");
  const auto& region1Containers =
      scopeData->scopeItemIdToContainerIds->at(region1Id);
  const auto& region2Containers =
      scopeData->scopeItemIdToContainerIds->at(region2Id);
  EXPECT_EQ(2, region1Containers.size());
  EXPECT_EQ(1, region2Containers.size());
}

CO_TEST_P(AsyncUniverseBuilderTest, AddObjectDimensionBasic) {
  const auto memoryId = universe.makeObjectDimensionId("memory");
  co_await universe.addObjectDimension(
      memoryId,
      ObjectDimensionData{
          .dimension =
              std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
                  /*totalSize=*/objectSize(),
                  /*defaultValue=*/1.0,
                  /*expectedNonDefaultSize=*/0))});
  EXPECT_EQ(memoryId, universe.getDimensionId("memory"));
}

CO_TEST_P(AsyncUniverseBuilderTest, AddScopeDimensionBasic) {
  const auto scopeId = universe.makeScopeId("region");
  const auto capacityId = universe.makeScopeDimensionId("capacity", scopeId);
  co_await universe.addScopeDimension(
      capacityId,
      scopeId,
      ScopeDimensionData{
          .dimension = std::make_shared<const ScopeDimension>(
              Map<ScopeItemId, double>{}, 100.0)});
  EXPECT_EQ(capacityId, universe.getDimensionId("capacity"));
}

CO_TEST_P(AsyncUniverseBuilderTest, AddPartitionGroupToObjects) {
  const Map<std::string, std::vector<std::string>> partition{
      {"group-1", {"o1", "o2"}},
      {"group-2", {"o3"}},
  };
  const auto partitionId = universe.makePartitionId("partition");
  co_await universe.addPartition(partitionId, partition);

  const auto partitionData = co_await universe.getPartition(partitionId);
  EXPECT_EQ(2, partitionData->groupNameToId->size());
  const auto group1Id = partitionData->getGroupId("group-1");
  const auto& group1Objects = partitionData->partition->getObjectIds(group1Id);
  EXPECT_EQ(2, group1Objects.size());
}

CO_TEST_P(AsyncUniverseBuilderTest, AddPartitionObjectToGroup) {
  const auto partitionId = universe.makePartitionId("partition");
  const Map<std::string, std::string> objectToGroup{
      {"o1", "group-1"},
      {"o2", "group-1"},
      {"o3", "group-2"},
  };
  co_await universe.addPartition(partitionId, objectToGroup);

  const auto partitionData = co_await universe.getPartition(partitionId);

  EXPECT_EQ(2, partitionData->groupNameToId->size());
  EXPECT_EQ(2, partitionData->partition->getGroupToObjectIds().size());
  const auto group1Id = partitionData->getGroupId("group-1");
  const auto group2Id = partitionData->getGroupId("group-2");
  const auto& group1Objects = partitionData->partition->getObjectIds(group1Id);
  const auto& group2Objects = partitionData->partition->getObjectIds(group2Id);
  EXPECT_EQ(2, group1Objects.size());
  EXPECT_EQ(1, group2Objects.size());
}

CO_TEST_P(AsyncUniverseBuilderTest, AddGoal) {
  interface::GoalSpecs spec;
  spec.capacitySpec().emplace();

  const auto goalId = universe.makeGoalId("test_goal");
  co_await universe.addGoal(
      goalId,
      GoalData{.goal = std::make_unique<Goal>(std::move(spec), 1.5, 0)});
}

CO_TEST_P(AsyncUniverseBuilderTest, AddConstraint) {
  interface::ConstraintSpecs spec;
  spec.capacitySpec().emplace();

  const auto constraintId = universe.makeConstraintId("test_constraint");
  co_await universe.addConstraint(
      constraintId,
      ConstraintData{
          .constraint = std::make_unique<Constraint>(
              std::move(spec),
              interface::ConstraintPolicy::HARD,
              100.0,
              0.0,
              0)});
}

CO_TEST_P(AsyncUniverseBuilderTest, AddRoutingConfig) {
  const auto scopeId = universe.makeScopeId("region");
  const auto partitionId = universe.makePartitionId("partition");

  const auto routingConfigId =
      universe.makeRoutingConfigId("test_routing_config");

  const Map<GroupId, std::vector<RoutingRing>> groupToRoutingRings;
  co_await universe.addRoutingConfig(
      routingConfigId,
      RoutingConfigData{
          .routingConfig = std::make_shared<RoutingConfig>(
              groupToRoutingRings,
              /*latencyTable=*/nullptr,
              scopeId,
              partitionId)});

  EXPECT_EQ(
      routingConfigId, universe.getRoutingConfigId("test_routing_config"));
}

CO_TEST_P(AsyncUniverseBuilderTest, Build) {
  // Add multiple scopes
  const auto regionScopeId = universe.makeScopeId("region");
  const Map<std::string, std::vector<std::string>> regionScope{
      {"us-east", {"c1", "c2"}},
      {"us-west", {"c3"}},
  };
  co_await universe.addScope(regionScopeId, regionScope);

  const auto rackScopeId = universe.makeScopeId("rack");
  const Map<std::string, std::string> containerToRack{
      {"c1", "rack-1"},
      {"c2", "rack-2"},
      {"c3", "rack-3"},
  };
  co_await universe.addScope(rackScopeId, containerToRack);

  // Add object dimensions
  const auto memoryDimId = universe.makeObjectDimensionId("memory");
  co_await universe.addObjectDimension(
      memoryDimId,
      ObjectDimensionData{
          .dimension =
              std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
                  /*totalSize=*/objectSize(),
                  /*defaultValue=*/10.0,
                  /*expectedNonDefaultSize=*/0))});

  const auto cpuDimId = universe.makeObjectDimensionId("cpu");
  co_await universe.addObjectDimension(
      cpuDimId,
      ObjectDimensionData{
          .dimension =
              std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
                  /*totalSize=*/objectSize(),
                  /*defaultValue=*/2.0,
                  /*expectedNonDefaultSize=*/0))});

  // Add scope dimensions for region scope
  const auto capacityDimId =
      universe.makeScopeDimensionId("capacity", regionScopeId);
  co_await universe.addScopeDimension(
      capacityDimId,
      regionScopeId,
      ScopeDimensionData{
          .dimension = std::make_shared<const ScopeDimension>(
              Map<ScopeItemId, double>{}, /*defaultValue=*/100.0)});

  // Add partitions using both input formats
  const auto partition1Id = universe.makePartitionId("job_partition");
  const Map<std::string, std::vector<std::string>> groupToObjects{
      {"job-A", {"o1"}},
      {"job-B", {"o2", "o3"}},
  };
  co_await universe.addPartition(partition1Id, groupToObjects);

  const auto partition2Id = universe.makePartitionId("priority_partition");
  const Map<std::string, std::string> objectToGroup{
      {"o1", "high"},
      {"o2", "low"},
      {"o3", "high"},
  };
  co_await universe.addPartition(partition2Id, objectToGroup);

  // Add goals with different weights and tuple indices
  const auto goal1Id = universe.makeGoalId("balance_goal");
  co_await universe.addGoal(
      goal1Id, GoalData{.goal = makeGoal(1.5, /*tupleIndex=*/0)});

  const auto goal2Id = universe.makeGoalId("minimize_moves_goal");
  co_await universe.addGoal(
      goal2Id, GoalData{.goal = makeGoal(0.5, /*tupleIndex=*/1)});

  // Add constraints
  const auto constraint1Id = universe.makeConstraintId("capacity_constraint");
  co_await universe.addConstraint(
      constraint1Id,
      ConstraintData{
          .constraint = makeConstraint(
              /*invalidCost=*/100.0, /*invalidState=*/0.0, /*tupleIndex=*/0)});

  const auto constraint2Id = universe.makeConstraintId("affinity_constraint");
  co_await universe.addConstraint(
      constraint2Id,
      ConstraintData{
          .constraint = makeConstraint(
              /*invalidCost=*/50.0, /*invalidState=*/1.0, /*tupleIndex=*/0)});

  // Add routing config
  const auto routingConfigId = universe.makeRoutingConfigId("routing_config");
  const Map<GroupId, std::vector<RoutingRing>> groupToRoutingRings;
  co_await universe.addRoutingConfig(
      routingConfigId,
      RoutingConfigData{
          .routingConfig = std::make_shared<RoutingConfig>(
              groupToRoutingRings,
              /*latencyTable=*/nullptr,
              regionScopeId,
              partition1Id)});

  // Set optional configurations
  const auto c1Id = universe.getContainers()->getId("c1");
  const auto c2Id = universe.getContainers()->getId("c2");
  const auto c3Id = universe.getContainers()->getId("c3");

  universe.setSimilarContainers({{c1Id, c2Id}, {c3Id}});
  universe.setMoveObjectsOnce(true);
  universe.setStableOptimization(true);
  universe.setDescendingHotnessContainers({c1Id, c2Id, c3Id});
  universe.setObjectOrderingDimensionId(memoryDimId);

  // Build the universe
  const auto builtUniverse = co_await universe.build();

  // Verify object/container type names
  EXPECT_EQ("task", builtUniverse->getObjectTypeName());
  EXPECT_EQ("host", builtUniverse->getContainerTypeName());

  // Verify objects and containers
  EXPECT_EQ(3, builtUniverse->getNumObjects());
  EXPECT_EQ(3, builtUniverse->getContainers().getContainerIds().size());

  // Verify scopes (trivial + region + rack = 3 scopes)
  const auto scopeIds = builtUniverse->getScopeIds();
  EXPECT_EQ(3, scopeIds.size());

  const auto builtRegionScopeId = builtUniverse->getScopeId("region");
  EXPECT_EQ(regionScopeId, builtRegionScopeId);
  const auto& regionScopeBuilt = builtUniverse->getScope(builtRegionScopeId);
  EXPECT_EQ(2, regionScopeBuilt.getScopeItemIds().size());

  const auto builtRackScopeId = builtUniverse->getScopeId("rack");
  EXPECT_EQ(rackScopeId, builtRackScopeId);
  const auto& rackScopeBuilt = builtUniverse->getScope(builtRackScopeId);
  EXPECT_EQ(3, rackScopeBuilt.getScopeItemIds().size());

  // Verify dimensions
  EXPECT_EQ(memoryDimId, builtUniverse->getDimensionId("memory"));
  EXPECT_EQ(cpuDimId, builtUniverse->getDimensionId("cpu"));
  EXPECT_EQ(capacityDimId, builtUniverse->getDimensionId("capacity"));

  // Verify object dimensions are accessible
  const auto& objects = builtUniverse->getObjects();
  EXPECT_EQ(10.0, objects.getDimension(memoryDimId).only().getDefaultValue());
  EXPECT_EQ(2.0, objects.getDimension(cpuDimId).only().getDefaultValue());

  // Verify scope dimensions are attached to correct scope
  const auto& regionScopeWithDims = builtUniverse->getScope(regionScopeId);
  EXPECT_EQ(
      100.0, regionScopeWithDims.getDimension(capacityDimId).getDefaultValue());

  // Verify partitions
  const auto partitionIds = builtUniverse->getPartitionIds();
  EXPECT_EQ(2, partitionIds.size());

  const auto builtPartition1Id = builtUniverse->getPartitionId("job_partition");
  EXPECT_EQ(partition1Id, builtPartition1Id);
  const auto& jobPartition = builtUniverse->getPartition(builtPartition1Id);
  EXPECT_EQ(2, jobPartition.getGroupIds().size());

  const auto builtPartition2Id =
      builtUniverse->getPartitionId("priority_partition");
  EXPECT_EQ(partition2Id, builtPartition2Id);
  const auto& priorityPartition =
      builtUniverse->getPartition(builtPartition2Id);
  EXPECT_EQ(2, priorityPartition.getGroupIds().size());

  // Verify goals
  const auto& goals = builtUniverse->getGoals();
  EXPECT_EQ(2, goals.getGoalIds().size());

  const auto builtGoal1Id = builtUniverse->getGoalId("balance_goal");
  EXPECT_EQ(goal1Id, builtGoal1Id);
  EXPECT_DOUBLE_EQ(1.5, goals.getGoal(builtGoal1Id).getWeight());
  EXPECT_EQ(0, goals.getGoal(builtGoal1Id).getTupleIndex());

  const auto builtGoal2Id = builtUniverse->getGoalId("minimize_moves_goal");
  EXPECT_EQ(goal2Id, builtGoal2Id);
  EXPECT_DOUBLE_EQ(0.5, goals.getGoal(builtGoal2Id).getWeight());
  EXPECT_EQ(1, goals.getGoal(builtGoal2Id).getTupleIndex());

  // Verify constraints
  const auto& constraints = builtUniverse->getConstraints();
  EXPECT_EQ(2, constraints.getConstraintIds().size());

  const auto builtConstraint1Id =
      builtUniverse->getConstraintId("capacity_constraint");
  EXPECT_EQ(constraint1Id, builtConstraint1Id);
  EXPECT_DOUBLE_EQ(
      100.0, constraints.getConstraint(builtConstraint1Id).getInvalidCost());

  const auto builtConstraint2Id =
      builtUniverse->getConstraintId("affinity_constraint");
  EXPECT_EQ(constraint2Id, builtConstraint2Id);
  EXPECT_DOUBLE_EQ(
      50.0, constraints.getConstraint(builtConstraint2Id).getInvalidCost());

  // Verify routing config
  const auto builtRoutingConfigId =
      builtUniverse->getRoutingConfigId("routing_config");
  EXPECT_EQ(routingConfigId, builtRoutingConfigId);
  const auto& routingConfig =
      builtUniverse->getRoutingConfig(builtRoutingConfigId);
  EXPECT_EQ(regionScopeId, routingConfig.getScopeId());
  EXPECT_EQ(partition1Id, routingConfig.getPartitionId());

  // Verify optional configurations
  EXPECT_TRUE(builtUniverse->getSimilarContainers().has_value());
  EXPECT_EQ(2, builtUniverse->getSimilarContainers()->size());
  EXPECT_TRUE(builtUniverse->getMoveObjectsOnce());
  EXPECT_TRUE(builtUniverse->getStableOptimization());
  EXPECT_EQ(3, builtUniverse->getDescendingHotnessContainers().size());
  EXPECT_TRUE(builtUniverse->getObjectOrderingDimensionId().has_value());
  EXPECT_EQ(memoryDimId, builtUniverse->getObjectOrderingDimensionId().value());

  // Verify entity names are correctly set
  // Object names
  const auto o1Id = builtUniverse->getObjectId("o1");
  const auto o2Id = builtUniverse->getObjectId("o2");
  const auto o3Id = builtUniverse->getObjectId("o3");
  EXPECT_EQ("o1", builtUniverse->getEntityName(o1Id));
  EXPECT_EQ("o2", builtUniverse->getEntityName(o2Id));
  EXPECT_EQ("o3", builtUniverse->getEntityName(o3Id));

  // Container names
  const auto builtC1Id = builtUniverse->getContainerId("c1");
  const auto builtC2Id = builtUniverse->getContainerId("c2");
  const auto builtC3Id = builtUniverse->getContainerId("c3");
  EXPECT_EQ("c1", builtUniverse->getEntityName(builtC1Id));
  EXPECT_EQ("c2", builtUniverse->getEntityName(builtC2Id));
  EXPECT_EQ("c3", builtUniverse->getEntityName(builtC3Id));

  // Scope names
  EXPECT_EQ("region", builtUniverse->getEntityName(regionScopeId));
  EXPECT_EQ("rack", builtUniverse->getEntityName(rackScopeId));
  const auto trivialScopeId = builtUniverse->getScopeId("host");
  EXPECT_EQ("host", builtUniverse->getEntityName(trivialScopeId));

  // Scope item names for region scope
  const auto usEastId = builtUniverse->getScopeItemId(regionScopeId, "us-east");
  const auto usWestId = builtUniverse->getScopeItemId(regionScopeId, "us-west");
  EXPECT_EQ("us-east", builtUniverse->getEntityName(usEastId));
  EXPECT_EQ("us-west", builtUniverse->getEntityName(usWestId));

  // Scope item names for rack scope
  const auto rack1Id = builtUniverse->getScopeItemId(rackScopeId, "rack-1");
  const auto rack2Id = builtUniverse->getScopeItemId(rackScopeId, "rack-2");
  const auto rack3Id = builtUniverse->getScopeItemId(rackScopeId, "rack-3");
  EXPECT_EQ("rack-1", builtUniverse->getEntityName(rack1Id));
  EXPECT_EQ("rack-2", builtUniverse->getEntityName(rack2Id));
  EXPECT_EQ("rack-3", builtUniverse->getEntityName(rack3Id));

  // Dimension names
  EXPECT_EQ("memory", builtUniverse->getEntityName(memoryDimId));
  EXPECT_EQ("cpu", builtUniverse->getEntityName(cpuDimId));
  EXPECT_EQ("capacity", builtUniverse->getEntityName(capacityDimId));
  const auto taskCountDimId = builtUniverse->getDimensionId("task_count");
  EXPECT_EQ("task_count", builtUniverse->getEntityName(taskCountDimId));

  // Partition names
  EXPECT_EQ("job_partition", builtUniverse->getEntityName(partition1Id));
  EXPECT_EQ("priority_partition", builtUniverse->getEntityName(partition2Id));

  // Group names within job_partition
  const auto jobAGroupId = builtUniverse->getGroupId(partition1Id, "job-A");
  const auto jobBGroupId = builtUniverse->getGroupId(partition1Id, "job-B");
  EXPECT_EQ("job-A", builtUniverse->getEntityName(jobAGroupId));
  EXPECT_EQ("job-B", builtUniverse->getEntityName(jobBGroupId));

  // Group names within priority_partition
  const auto highGroupId = builtUniverse->getGroupId(partition2Id, "high");
  const auto lowGroupId = builtUniverse->getGroupId(partition2Id, "low");
  EXPECT_EQ("high", builtUniverse->getEntityName(highGroupId));
  EXPECT_EQ("low", builtUniverse->getEntityName(lowGroupId));

  // Goal names
  EXPECT_EQ("balance_goal", builtUniverse->getEntityName(goal1Id));
  EXPECT_EQ("minimize_moves_goal", builtUniverse->getEntityName(goal2Id));

  // Constraint names
  EXPECT_EQ("capacity_constraint", builtUniverse->getEntityName(constraint1Id));
  EXPECT_EQ("affinity_constraint", builtUniverse->getEntityName(constraint2Id));

  // Routing config name - verify exact name
  EXPECT_EQ("routing_config", builtUniverse->getEntityName(routingConfigId));
}

CO_TEST_P(AsyncUniverseBuilderTest, ConcurrentAddThenBuild) {
  // Create a larger universe for concurrency testing
  AsyncUniverseBuilder largeUniverse;
  largeUniverse.setObjectTypeName("task");
  largeUniverse.setContainerTypeName("host");

  constexpr size_t kNumObjects = 10'000;
  constexpr size_t kNumContainers = 500;
  const auto assignment = generateInitialAssignment(
      ProblemConfig{
          .numObjects = kNumObjects, .numContainers = kNumContainers});
  largeUniverse.setInitialAssignment(assignment);

  const auto containers = largeUniverse.getContainers();
  const auto objects = largeUniverse.getObjects();

  // Prepare IDs for entities to be added concurrently
  constexpr int kNumScopes = 50;
  constexpr int kNumScopeItems = 25;
  constexpr int kNumObjectDimensions = 100;
  constexpr int kNumScopeDimensions = 50;
  constexpr int kNumPartitions = 30;
  constexpr int kNumGroups = 20;
  constexpr int kNumGoals = 100;
  constexpr int kNumConstraints = 100;
  constexpr int kNumRoutingConfigs = 20;

  std::vector<ScopeId> scopeIds;
  scopeIds.reserve(kNumScopes);
  for (const auto i : folly::irange(kNumScopes)) {
    scopeIds.push_back(largeUniverse.makeScopeId(fmt::format("scope_{}", i)));
  }

  std::vector<PartitionId> partitionIds;
  partitionIds.reserve(kNumPartitions);
  for (const auto i : folly::irange(kNumPartitions)) {
    partitionIds.push_back(
        largeUniverse.makePartitionId(fmt::format("partition_{}", i)));
  }

  std::vector<GoalId> goalIds;
  goalIds.reserve(kNumGoals);
  for (const auto i : folly::irange(kNumGoals)) {
    goalIds.push_back(largeUniverse.makeGoalId(fmt::format("goal_{}", i)));
  }

  std::vector<ConstraintId> constraintIds;
  constraintIds.reserve(kNumConstraints);
  for (const auto i : folly::irange(kNumConstraints)) {
    constraintIds.push_back(
        largeUniverse.makeConstraintId(fmt::format("constraint_{}", i)));
  }

  std::vector<RoutingConfigId> routingConfigIds;
  routingConfigIds.reserve(kNumRoutingConfigs);
  for (const auto i : folly::irange(kNumRoutingConfigs)) {
    routingConfigIds.push_back(
        largeUniverse.makeRoutingConfigId(fmt::format("routing_{}", i)));
  }

  // Generate test data
  const auto scopeItemToContainers =
      generateScopeItemNameToContainerNames(kNumScopeItems, *containers);
  const auto groupToObjects =
      generateGroupNameToObjectNames(kNumGroups, *objects);

  // Build tasks for concurrent execution
  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve(
      kNumScopes + kNumObjectDimensions + kNumScopeDimensions + kNumPartitions +
      kNumGoals + kNumConstraints + kNumRoutingConfigs);

  // Add all scope coros
  for (const auto i : folly::irange(kNumScopes)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&largeUniverse,
             scopeId = scopeIds[i],
             &scopeItemToContainers]() -> folly::coro::Task<void> {
              executeRandomDelay();
              co_await largeUniverse.addScope(scopeId, scopeItemToContainers);
            }));
  }

  // Add all object dimension coros
  for (const auto i : folly::irange(kNumObjectDimensions)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&largeUniverse, i, this]() -> folly::coro::Task<void> {
              const auto dimId = largeUniverse.makeObjectDimensionId(
                  fmt::format("obj_dim_{}", i));
              executeRandomDelay();
              co_await largeUniverse.addObjectDimension(
                  dimId,
                  ObjectDimensionData{
                      .dimension = std::make_shared<const ObjectDimension>(
                          ObjectIdToDoubleMap(
                              /*totalSize=*/objectSize(),
                              /*defaultValue=*/static_cast<double>(i),
                              /*expectedNonDefaultSize=*/0))});
            }));
  }

  // Add all scope dimension coros
  for (const auto i : folly::irange(kNumScopeDimensions)) {
    const auto scopeId = scopeIds[i % kNumScopes];
    const auto scopeDimId = largeUniverse.makeScopeDimensionId(
        fmt::format("scope_dim_{}", i), scopeId);
    tasks.push_back(
        folly::coro::co_invoke(
            [&largeUniverse, dimId = scopeDimId, scopeId, i]()
                -> folly::coro::Task<void> {
              executeRandomDelay();
              co_await largeUniverse.addScopeDimension(
                  dimId,
                  scopeId,
                  ScopeDimensionData{
                      .dimension = std::make_shared<const ScopeDimension>(
                          Map<ScopeItemId, double>{},
                          static_cast<double>(i) * 10)});
            }));
  }

  // Add all partitions coros
  for (const auto i : folly::irange(kNumPartitions)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&largeUniverse,
             partitionId = partitionIds[i],
             &groupToObjects]() -> folly::coro::Task<void> {
              executeRandomDelay();
              co_await largeUniverse.addPartition(partitionId, groupToObjects);
            }));
  }

  // Add all goal coros
  for (const auto i : folly::irange(kNumGoals)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&largeUniverse,
             goalId = goalIds[i],
             i]() -> folly::coro::Task<void> {
              executeRandomDelay();
              const double weight = static_cast<double>(i) / 100.0;
              const int tupleIndex = i % 3;
              co_await largeUniverse.addGoal(
                  goalId, GoalData{.goal = makeGoal(weight, tupleIndex)});
            }));
  }

  // Add constraint coros
  for (const auto i : folly::irange(kNumConstraints)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&largeUniverse,
             constraintId = constraintIds[i],
             i]() -> folly::coro::Task<void> {
              executeRandomDelay();
              const double invalidCost = static_cast<double>(i) * 10.0;
              const double invalidState = static_cast<double>(i) / 1000.0;
              const int tupleIndex = i % 3;
              co_await largeUniverse.addConstraint(
                  constraintId,
                  ConstraintData{
                      .constraint = makeConstraint(
                          invalidCost, invalidState, tupleIndex)});
            }));
  }

  // Add routing config coros
  for (const auto i : folly::irange(kNumRoutingConfigs)) {
    const auto scopeId = scopeIds[i % kNumScopes];
    const auto partitionId = partitionIds[i % kNumPartitions];
    tasks.push_back(
        folly::coro::co_invoke(
            [&largeUniverse,
             routingConfigId = routingConfigIds[i],
             scopeId,
             partitionId]() -> folly::coro::Task<void> {
              executeRandomDelay();
              const Map<GroupId, std::vector<RoutingRing>> groupToRoutingRings;
              co_await largeUniverse.addRoutingConfig(
                  routingConfigId,
                  RoutingConfigData{
                      .routingConfig = std::make_shared<RoutingConfig>(
                          groupToRoutingRings,
                          /*latencyTable=*/nullptr,
                          scopeId,
                          partitionId)});
            }));
  }

  // Execute all tasks concurrently
  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  // Build the universe
  const auto builtUniverse = co_await largeUniverse.build();

  // Verify all entities are correctly aggregated
  EXPECT_EQ("task", builtUniverse->getObjectTypeName());
  EXPECT_EQ("host", builtUniverse->getContainerTypeName());
  EXPECT_EQ(kNumObjects, builtUniverse->getNumObjects());
  EXPECT_EQ(
      kNumContainers, builtUniverse->getContainers().getContainerIds().size());

  // Verify scopes (kNumScopes + trivial scope)
  EXPECT_EQ(kNumScopes + 1, builtUniverse->getScopeIds().size());
  for (const auto i : folly::irange(kNumScopes)) {
    const auto scopeName = fmt::format("scope_{}", i);
    const auto scopeId = builtUniverse->getScopeId(scopeName);
    EXPECT_EQ(scopeIds[i], scopeId);
    const auto& scope = builtUniverse->getScope(scopeId);
    EXPECT_EQ(kNumScopeItems, scope.getScopeItemIds().size());
  }

  // Verify dimensions (kNumObjectDimensions + kNumScopeDimensions + task_count)
  const auto& objectsBuilt = builtUniverse->getObjects();
  for (const auto i : folly::irange(kNumObjectDimensions)) {
    const auto dimName = fmt::format("obj_dim_{}", i);
    const auto dimId = builtUniverse->getDimensionId(dimName);
    EXPECT_DOUBLE_EQ(
        static_cast<double>(i),
        objectsBuilt.getDimension(dimId).only().getDefaultValue());
  }

  // Verify partitions
  EXPECT_EQ(kNumPartitions, builtUniverse->getPartitionIds().size());
  for (const auto i : folly::irange(kNumPartitions)) {
    const auto partitionName = fmt::format("partition_{}", i);
    const auto partitionId = builtUniverse->getPartitionId(partitionName);
    EXPECT_EQ(partitionIds[i], partitionId);
    const auto& partition = builtUniverse->getPartition(partitionId);
    EXPECT_EQ(kNumGroups, partition.getGroupIds().size());
  }

  // Verify goals
  const auto& goals = builtUniverse->getGoals();
  EXPECT_EQ(kNumGoals, goals.getGoalIds().size());
  for (const auto i : folly::irange(kNumGoals)) {
    const auto goalName = fmt::format("goal_{}", i);
    const auto goalId = builtUniverse->getGoalId(goalName);
    EXPECT_EQ(goalIds[i], goalId);
    const double expectedWeight = static_cast<double>(i) / 100.0;
    EXPECT_DOUBLE_EQ(expectedWeight, goals.getGoal(goalId).getWeight());
    EXPECT_EQ(i % 3, goals.getGoal(goalId).getTupleIndex());
  }

  // Verify constraints
  const auto& constraints = builtUniverse->getConstraints();
  EXPECT_EQ(kNumConstraints, constraints.getConstraintIds().size());
  for (const auto i : folly::irange(kNumConstraints)) {
    const auto constraintName = fmt::format("constraint_{}", i);
    const auto constraintId = builtUniverse->getConstraintId(constraintName);
    EXPECT_EQ(constraintIds[i], constraintId);
    const double expectedInvalidCost = static_cast<double>(i) * 10.0;
    EXPECT_DOUBLE_EQ(
        expectedInvalidCost,
        constraints.getConstraint(constraintId).getInvalidCost());
  }

  // Verify routing configs
  for (const auto i : folly::irange(kNumRoutingConfigs)) {
    const auto routingName = fmt::format("routing_{}", i);
    const auto routingConfigId = builtUniverse->getRoutingConfigId(routingName);
    EXPECT_EQ(routingConfigIds[i], routingConfigId);
    const auto& routingConfig =
        builtUniverse->getRoutingConfig(routingConfigId);
    EXPECT_EQ(scopeIds[i % kNumScopes], routingConfig.getScopeId());
    EXPECT_EQ(partitionIds[i % kNumPartitions], routingConfig.getPartitionId());
  }
}

CO_TEST_P(AsyncUniverseBuilderTest, AddGoalAfterBuildingUniverse) {
  const auto goal1Id = universe.makeGoalId("goal_1");
  co_await universe.addGoal(
      goal1Id, GoalData{.goal = makeGoal(1.0, /*tupleIndex=*/0)});

  const auto builtUniverse1 = co_await universe.build();
  EXPECT_EQ(1, builtUniverse1->getGoals().getGoalIds().size());

  const auto goal2Id = universe.makeGoalId("goal_2");
  co_await universe.addGoal(
      goal2Id, GoalData{.goal = makeGoal(2.0, /*tupleIndex=*/1)});

  const auto builtUniverse2 = co_await universe.build();
  EXPECT_EQ(1, builtUniverse1->getGoals().getGoalIds().size());
  EXPECT_EQ(2, builtUniverse2->getGoals().getGoalIds().size());
}

CO_TEST_P(AsyncUniverseBuilderTest, AddConstraintAfterBuildingUniverse) {
  const auto constraint1Id = universe.makeConstraintId("constraint_1");
  co_await universe.addConstraint(
      constraint1Id,
      ConstraintData{
          .constraint = makeConstraint(
              /*invalidCost=*/100.0, /*invalidState=*/0.0, /*tupleIndex=*/0)});

  const auto builtUniverse1 = co_await universe.build();
  EXPECT_EQ(1, builtUniverse1->getConstraints().getConstraintIds().size());

  const auto constraint2Id = universe.makeConstraintId("constraint_2");
  co_await universe.addConstraint(
      constraint2Id,
      ConstraintData{
          .constraint = makeConstraint(
              /*invalidCost=*/50.0, /*invalidState=*/1.0, /*tupleIndex=*/1)});

  const auto builtUniverse2 = co_await universe.build();
  EXPECT_EQ(1, builtUniverse1->getConstraints().getConstraintIds().size());
  EXPECT_EQ(2, builtUniverse2->getConstraints().getConstraintIds().size());
}

CO_TEST_P(AsyncUniverseBuilderTest, AddRoutingConfigAfterBuildingUniverse) {
  const auto scopeId = universe.makeScopeId("region");
  const auto partitionId = universe.makePartitionId("partition");

  // Scope and partition must have data set before build(), otherwise
  // build() hangs waiting on unfulfilled promises in the AsyncValueMap.
  const Map<std::string, std::vector<std::string>> regionScope{
      {"region-1", {"c1", "c2"}},
      {"region-2", {"c3"}},
  };
  co_await universe.addScope(scopeId, regionScope);

  const Map<std::string, std::vector<std::string>> partition{
      {"group-1", {"o1", "o2"}},
      {"group-2", {"o3"}},
  };
  co_await universe.addPartition(partitionId, partition);

  const auto routingConfig1Id =
      universe.makeRoutingConfigId("routing_config_1");
  const Map<GroupId, std::vector<RoutingRing>> groupToRoutingRings;
  co_await universe.addRoutingConfig(
      routingConfig1Id,
      RoutingConfigData{
          .routingConfig = std::make_shared<RoutingConfig>(
              groupToRoutingRings,
              /*latencyTable=*/nullptr,
              scopeId,
              partitionId)});

  const auto builtUniverse1 = co_await universe.build();
  EXPECT_EQ(
      routingConfig1Id, builtUniverse1->getRoutingConfigId("routing_config_1"));

  const auto routingConfig2Id =
      universe.makeRoutingConfigId("routing_config_2");
  co_await universe.addRoutingConfig(
      routingConfig2Id,
      RoutingConfigData{
          .routingConfig = std::make_shared<RoutingConfig>(
              groupToRoutingRings,
              /*latencyTable=*/nullptr,
              scopeId,
              partitionId)});

  const auto builtUniverse2 = co_await universe.build();
  EXPECT_EQ(
      routingConfig1Id, builtUniverse1->getRoutingConfigId("routing_config_1"));
  EXPECT_THROW(
      builtUniverse1->getRoutingConfigId("routing_config_2"),
      std::out_of_range);
  EXPECT_EQ(
      routingConfig1Id, builtUniverse2->getRoutingConfigId("routing_config_1"));
  EXPECT_EQ(
      routingConfig2Id, builtUniverse2->getRoutingConfigId("routing_config_2"));
}

CO_TEST_P(AsyncUniverseBuilderTest, AddPartitionAfterBuildingUniverse) {
  const auto partition1Id = universe.makePartitionId("partition_1");
  const Map<std::string, std::vector<std::string>> partition1{
      {"group-1", {"o1", "o2"}},
      {"group-2", {"o3"}},
  };
  co_await universe.addPartition(partition1Id, partition1);

  const auto builtUniverse1 = co_await universe.build();
  EXPECT_EQ(1, builtUniverse1->getPartitionIds().size());

  const auto partition2Id = universe.makePartitionId("partition_2");
  const Map<std::string, std::vector<std::string>> partition2{
      {"group-3", {"o1"}},
      {"group-4", {"o2", "o3"}},
  };
  co_await universe.addPartition(partition2Id, partition2);

  const auto builtUniverse2 = co_await universe.build();
  EXPECT_EQ(1, builtUniverse1->getPartitionIds().size());
  EXPECT_EQ(2, builtUniverse2->getPartitionIds().size());
}

CO_TEST_P(AsyncUniverseBuilderTest, AddScopeAfterBuildingUniverse) {
  const auto scope1Id = universe.makeScopeId("scope_1");
  const Map<std::string, std::vector<std::string>> scope1{
      {"region-1", {"c1", "c2"}},
      {"region-2", {"c3"}},
  };
  co_await universe.addScope(scope1Id, scope1);

  const auto builtUniverse1 = co_await universe.build();
  EXPECT_EQ(2, builtUniverse1->getScopeIds().size());
  EXPECT_EQ(scope1Id, builtUniverse1->getScopeId("scope_1"));

  const auto scope2Id = universe.makeScopeId("scope_2");
  const Map<std::string, std::vector<std::string>> scope2{
      {"rack-1", {"c1"}},
      {"rack-2", {"c2", "c3"}},
  };
  co_await universe.addScope(scope2Id, scope2);

  const auto builtUniverse2 = co_await universe.build();
  EXPECT_EQ(2, builtUniverse1->getScopeIds().size());
  EXPECT_EQ(scope1Id, builtUniverse1->getScopeId("scope_1"));
  EXPECT_EQ(3, builtUniverse2->getScopeIds().size());
  EXPECT_EQ(scope1Id, builtUniverse2->getScopeId("scope_1"));
  EXPECT_EQ(scope2Id, builtUniverse2->getScopeId("scope_2"));
}

CO_TEST_P(AsyncUniverseBuilderTest, AddDimensionAfterBuildingUniverse) {
  const auto dimension1Id = universe.makeObjectDimensionId("dimension_1");
  co_await universe.addObjectDimension(
      dimension1Id,
      ObjectDimensionData{
          .dimension =
              std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
                  /*totalSize=*/objectSize(),
                  /*defaultValue=*/5.0,
                  /*expectedNonDefaultSize=*/0))});

  const auto builtUniverse1 = co_await universe.build();
  EXPECT_EQ(2, builtUniverse1->getObjects().getDimensionCount());
  EXPECT_EQ(dimension1Id, builtUniverse1->getDimensionId("dimension_1"));

  const auto dimension2Id = universe.makeObjectDimensionId("dimension_2");
  co_await universe.addObjectDimension(
      dimension2Id,
      ObjectDimensionData{
          .dimension =
              std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
                  /*totalSize=*/objectSize(),
                  /*defaultValue=*/10.0,
                  /*expectedNonDefaultSize=*/0))});

  const auto builtUniverse2 = co_await universe.build();
  EXPECT_EQ(2, builtUniverse1->getObjects().getDimensionCount());
  EXPECT_EQ(dimension1Id, builtUniverse1->getDimensionId("dimension_1"));
  EXPECT_EQ(3, builtUniverse2->getObjects().getDimensionCount());
  EXPECT_EQ(dimension1Id, builtUniverse2->getDimensionId("dimension_1"));
  EXPECT_EQ(dimension2Id, builtUniverse2->getDimensionId("dimension_2"));
}

CO_TEST_P(AsyncUniverseBuilderTest, BuildAssignmentTwice) {
  const auto builtUniverse1 = co_await universe.build();
  EXPECT_EQ(3, builtUniverse1->getNumObjects());
  EXPECT_EQ(3, builtUniverse1->getContainers().getContainerIds().size());

  const auto builtUniverse2 = co_await universe.build();
  EXPECT_EQ(3, builtUniverse1->getNumObjects());
  EXPECT_EQ(3, builtUniverse1->getContainers().getContainerIds().size());
  EXPECT_EQ(3, builtUniverse2->getNumObjects());
  EXPECT_EQ(3, builtUniverse2->getContainers().getContainerIds().size());
}

} // namespace facebook::rebalancer::entities::tests
