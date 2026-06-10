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

#include "algopt/rebalancer/interface/tests/utils.h"
#include <algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h>

#include <folly/container/irange.h>
#include <folly/String.h>
#include <gtest/gtest.h>

#define ASSERT_OBJECT_IN_ANY_CONTAINER(object, containers, assignment) \
  assertObjectInAnyContainer(__FILE__, __LINE__, object, containers, assignment)

void assertObjectInAnyContainer(
    const std::string&
        fileName /*File name from which this function was called*/,
    int lineNumber /*Line number from which this function was called*/,
    const std::string& object,
    const std::vector<std::string>& containers,
    const folly::F14FastMap<std::string, std::string>& assignment) {
  auto& finalContainer = assignment.at(object);
  ASSERT_TRUE(
      std::find(containers.begin(), containers.end(), finalContainer) !=
      containers.end())
      << fmt::format(
             "assertion failed on line {} in file {}\nobject {} is in container {} which is not in the list of expected containers [{}]",
             lineNumber,
             fileName,
             object,
             finalContainer,
             folly::join(", ", containers));
}

namespace facebook::rebalancer::interface::tests {

class ColocateGroupsMoveTypeTest : public ::testing::TestWithParam<int> {
 protected:
  std::vector<std::string> getAllObjects() {
    std::vector<std::string> allObjects;
    for (const auto& [_, objects] : groupToObjects_) {
      allObjects.insert(allObjects.end(), objects.begin(), objects.end());
    }
    return allObjects;
  }

  std::vector<std::string> getAllNonDummyContainers() {
    std::vector<std::string> allNonDummyContainers;
    for (const auto& [_, containers] : regionToContainers_) {
      allNonDummyContainers.insert(
          allNonDummyContainers.end(), containers.begin(), containers.end());
    }
    return allNonDummyContainers;
  }

  static interface::ColocateGroupsMoveTypeRelatedGroupsInfo
  makeRelatedGroupsInfo(
      const folly::F14FastSet<std::string>& relatedGroupNames,
      const std::optional<folly::F14FastSet<std::string>>&
          destinationScopeItems = std::nullopt) {
    interface::ColocateGroupsMoveTypeRelatedGroupsInfo relatedGroupsInfo;
    relatedGroupsInfo.relatedGroups() = relatedGroupNames;
    relatedGroupsInfo.destinationScopeItems().from_optional(
        destinationScopeItems);

    return relatedGroupsInfo;
  }

  std::vector<std::string> addLeaderFollowerDimsAndReturnNames() {
    // create dimensions, e.g., "l_f1", "l_f2", "l_f3"...
    // for each model in modelToTenants_, the leaders (the first entry for
    // the corresponding list of sub-models) have value 1. Follower-i for that
    // model has value -1  w.r.t. dimension "l_f{i}"
    auto getLFiDimValues = [this](size_t i) {
      entities::Map<std::string, double> objectToLFi;
      for (const auto& [model, tenants] : modelToTenants_) {
        if ((tenants.size() <= i)) {
          // add i-th dimension w.r.t. a model only if the model has greater at
          // least i followers (note that the first one is leader)
          continue;
        }

        auto& leaderName = tenants[0];
        for (auto& object : groupToObjects_.at(leaderName)) {
          objectToLFi[object] = 1.0;
        }

        if (tenants.size() > i) {
          auto tenantName = tenants[i];
          auto& objectsInTenant = groupToObjects_.at(tenantName);
          for (auto& object : objectsInTenant) {
            objectToLFi[object] = -1.0;
          }
        }
      }
      return objectToLFi;
    };

    std::vector<std::string> dimNames;
    size_t maxNumFollowers = 0;
    for (auto& [_, tenants] : modelToTenants_) {
      // the first one is always the leader, so use tenants.size()- 1
      maxNumFollowers = std::max(maxNumFollowers, tenants.size() - 1);
    }

    for (const auto i : folly::irange(1, maxNumFollowers + 1)) {
      auto dimName = fmt::format("l_f{}", i);
      solver_->addObjectDimension(dimName, getLFiDimValues(i), 0.0);
      dimNames.emplace_back(std::move(dimName));
    }

    return dimNames;
  }

  void addLeaderFollowerConstraints() {
    // enforce the constraint that for each model in modelToTenants_ which has
    // more than one tenant associated with it, the number of objects of the
    // "leader" (first tenant) is equal to number of objects of each of
    // followers
    auto leaderFollowerDimNames = addLeaderFollowerDimsAndReturnNames();
    for (auto& dimName : leaderFollowerDimNames) {
      GroupCountSpec groupCountSpec;
      groupCountSpec.name() =
          fmt::format("leader follower EXACT constraint {}", dimName);
      groupCountSpec.scope() = COLOCATION_SCOPE_NAME;
      groupCountSpec.partitionName() = MODEL_PARTITION_NAME;
      groupCountSpec.dimension() = dimName;
      groupCountSpec.limit()->globalLimit() = 0;
      groupCountSpec.bound() = GroupCountSpecBound::EXACT;

      solver_->addConstraint(groupCountSpec);
    }
  }

  void addConstraintToAllocateAllObjects() {
    // empty dummy container
    ToFreeSpec toFreeSpec;
    toFreeSpec.name() = "free dummy container";
    toFreeSpec.containers() = {DUMMY_CONTAINER_NAME};
    solver_->addConstraint(std::move(toFreeSpec));
  }

  void addCapacityConstraints() {
    CapacitySpec capacitySpec;
    capacitySpec.name() = "capacity constraint";
    capacitySpec.scope() = CONTAINER_NAME;
    capacitySpec.dimension() = fmt::format("{}_count", OBJECT_NAME);
    capacitySpec.definition() = CapacitySpecDefinition::AFTER;
    capacitySpec.bound() = CapacitySpecBound::MAX;
    capacitySpec.limit()->type() = LimitType::RELATIVE;
    capacitySpec.limit()->globalLimit() = 1.0;
    capacitySpec.filter()->itemsBlacklist() = {DUMMY_CONTAINER_NAME};

    solver_->addConstraint(std::move(capacitySpec));
  }

  void setUpProblem(
      const entities::Map<std::string, std::vector<std::string>>&
          initialAssignment) {
    solver_ = initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver_->setObjectName(OBJECT_NAME);
    solver_->setContainerName(CONTAINER_NAME);

    solver_->setAssignment(initialAssignment);

    // add "tenants" partition
    solver_->addPartition(TENANT_PARTITION_NAME, groupToObjects_);

    // add model partition
    entities::Map<std::string, std::vector<std::string>> modelToObjects;
    for (const auto& [model, tenants] : modelToTenants_) {
      std::vector<std::string> objects;
      for (auto& tenant : tenants) {
        auto& tenantObjects = groupToObjects_.at(tenant);
        objects.insert(
            objects.end(), tenantObjects.begin(), tenantObjects.end());
      }

      modelToObjects[model] = std::move(objects);
    }
    solver_->addPartition(MODEL_PARTITION_NAME, modelToObjects);

    // add "region" scope
    solver_->addScope(COLOCATION_SCOPE_NAME, regionToContainers_);

    // add "object_count" dimension for containers
    const entities::Map<std::string, double> containerToMaxObjectCount = {
        {"r1_c1", 1},
        {"r1_c2", 1},
        {"r1_c3", 1},
        {"r1_c4", 3},
        {"r1_c5", 3},
        {"r1_c6", 3},
        //
        {"r2_c1", 1},
        {"r2_c2", 1},
        {"r2_c3", 1},
        {"r2_c4", 0},
        //
        {"r3_c1", 3},
        {"r3_c2", 2},
        {"r3_c3", 6},
    };
    solver_->addContainerDimension(
        fmt::format("{}_count", OBJECT_NAME), containerToMaxObjectCount);

    solver_->enableMoveStats();
  }

  std::unique_ptr<ProblemSolver> solver_;
  entities::Map<std::string, std::vector<std::string>> groupToObjects_ = {
      {"m1_l1", {"m1_l1_0", "m1_l1_1", "m1_l1_2"}},
      {"m1_f1", {"m1_f1_0", "m1_f1_1", "m1_f1_2"}},
      {"m1_f2", {"m1_f2_0", "m1_f2_1", "m1_f2_2"}},
      {"m1_f3", {"m1_f3_0", "m1_f3_1", "m1_f3_2"}},
      {"m2_l1", {"m2_l1_0"}},
      {"m2_f1", {"m2_f1_0"}},
      {"m3", {"m3_0", "m3_1", "m3_2"}},
      {"m4", {"m4_0", "m4_1", "m4_2"}},
  };
  entities::Map<std::string, std::vector<std::string>> regionToContainers_ = {
      {"region1", {"r1_c1", "r1_c2", "r1_c3", "r1_c4", "r1_c5", "r1_c6"}},
      {"region2", {"r2_c1", "r2_c2", "r2_c3", "r2_c4"}},
      {"region3", {"r3_c1", "r3_c2", "r3_c3"}},
  };

  entities::Map<std::string, std::vector<std::string>> modelToTenants_ = {
      {"m1", {"m1_l1", "m1_f1", "m1_f2", "m1_f3"}},
      {"m2", {"m2_l1", "m2_f1"}},
      {"m3", {"m3"}},
      {"m4", {"m4"}},
  };

  std::string COLOCATION_SCOPE_NAME = "region";
  std::string DUMMY_CONTAINER_NAME = "dummy";
  std::string TENANT_PARTITION_NAME = "tenant";
  std::string MODEL_PARTITION_NAME = "model";
  std::string CONTAINER_NAME = "container";
  std::string OBJECT_NAME = "object";
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    ColocateGroupsMoveTypeTest,
    testThreadCounts());

TEST_P(ColocateGroupsMoveTypeTest, AllocateFromDummyContainer) {
  // initially all objects are in the dummy container
  entities::Map<std::string, std::vector<std::string>> initialAssignment;
  initialAssignment[DUMMY_CONTAINER_NAME] = getAllObjects();
  for (auto& container : getAllNonDummyContainers()) {
    initialAssignment[container] = {};
  }

  setUpProblem(initialAssignment);

  // add constraints and goals
  addConstraintToAllocateAllObjects();
  addLeaderFollowerConstraints();
  addCapacityConstraints();

  // add move type spec
  ColocateGroupsMoveTypeSpec colocateMoveTypeSpec;
  colocateMoveTypeSpec.partitionName() = TENANT_PARTITION_NAME;
  colocateMoveTypeSpec.colocationScopeName() = COLOCATION_SCOPE_NAME;
  colocateMoveTypeSpec.relatedGroupsList() = {
      makeRelatedGroupsInfo(
          {"m1_l1", "m1_f1", "m1_f2", "m1_f3"},
          folly::F14FastSet<std::string>{
              "region1"}), // only try moves to region1 for these tenants
      makeRelatedGroupsInfo({"m2_l1", "m2_f1"}),
  };

  folly::F14FastMap<
      std::string,
      folly::F14FastMap<std::string, folly::F14FastSet<std::string>>>
      colocationScopeItemToGroupToContainers;
  colocationScopeItemToGroupToContainers["region1"]["m1_l1"] = {
      "r1_c1", "r1_c2", "r1_c3"};
  auto m1FollowerDestinations =
      folly::F14FastSet<std::string>{"r1_c4", "r1_c5", "r1_c6"};
  colocationScopeItemToGroupToContainers["region1"]["m1_f1"] =
      m1FollowerDestinations;
  colocationScopeItemToGroupToContainers["region1"]["m1_f2"] =
      m1FollowerDestinations;
  colocationScopeItemToGroupToContainers["region1"]["m1_f3"] =
      m1FollowerDestinations;

  colocationScopeItemToGroupToContainers["region2"]["m2_l1"] = {"r2_c1"};
  colocationScopeItemToGroupToContainers["region2"]["m2_f1"] = {"r2_c2"};

  colocationScopeItemToGroupToContainers["region1"]["m2_l1"] =
      {}; // cannot be moved to any container in region1
  colocationScopeItemToGroupToContainers["region3"]["m2_l1"] =
      {}; // cannot be moved to any container in region3

  colocateMoveTypeSpec.colocationScopeItemToGroupToContainers() =
      std::move(colocationScopeItemToGroupToContainers);

  // add solver spec
  interface::LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList() = {
      solver_->makeMoveTypeSpec(std::move(colocateMoveTypeSpec))};

  solver_->addSolver(solverSpec);

  // solve
  auto solution = solver_->solve();
  auto& assignment = *solution.assignment();

  // assert that there are broken constraints initially, and also finally since
  // "m3" and "m4" objects cannot be allocated with just
  // ColocateGroupsMoveType
  ASSERT_EQ(*solution.initialConstraint()->brokenCount(), 1);
  ASSERT_EQ(*solution.finalConstraint()->brokenCount(), 1);

  // expect all objects in "m1_l1", "m1_f1", "m1_f2", "m1_f3" to be moved to
  // "region1". In particular: "m1_l1" should be in one of {"r1_c1",
  // "r1_c2", "r1_c3"}
  const std::vector<std::string> potentialDestinations1 = {
      "r1_c1", "r1_c2", "r1_c3"};
  for (auto& object : groupToObjects_.at("m1_l1")) {
    ASSERT_OBJECT_IN_ANY_CONTAINER(object, potentialDestinations1, assignment);
  }

  // objects in "m1_f1", "m1_f2", "m1_f3" should be in {"r1_c4","r1_c5",
  // "r1_c6"}
  const std::vector<std::string> potentialDestinations2 = {
      "r1_c4", "r1_c5", "r1_c6"};
  for (auto group : {"m1_f1", "m1_f2", "m1_f3"}) {
    for (auto& object : groupToObjects_.at(group)) {
      ASSERT_OBJECT_IN_ANY_CONTAINER(
          object, potentialDestinations2, assignment);
    }
  }

  // we expect objects in "m2_l1" and "m2_f1" to be moved to "region2" and
  // in particular to some destination in {"r2_c1", "r2_c2", "r2_c3"}. ("r2_c4"
  // has zero capacity)
  const std::vector<std::string> potentialDestinations3 = {
      "r2_c1", "r2_c2", "r2_c3"};
  for (auto group : {"m2_l1", "m2_f1"}) {
    for (auto& object : groupToObjects_.at(group)) {
      ASSERT_OBJECT_IN_ANY_CONTAINER(
          object, potentialDestinations3, assignment);
    }
  }

  // We are only using ColcoateGroupsMoveTypeSpec, so we do not expect objects
  // w.r.t "m3" and "m4" to move since they are not part of relatedGroupsSet
  // in colocateMoveType
  const std::vector<std::string> potentialDestinations4 = {
      DUMMY_CONTAINER_NAME};
  for (auto group : {"m3", "m4"}) {
    for (auto& object : groupToObjects_.at(group)) {
      ASSERT_OBJECT_IN_ANY_CONTAINER(
          object, potentialDestinations4, assignment);
    }
  }
}

TEST_P(ColocateGroupsMoveTypeTest, MoveAcrossColocationScopeItems) {
  const entities::Map<std::string, std::vector<std::string>> initialAssignment =
      {
          {"r1_c1", {"m1_l1_0", "m3_0"}}, // capacityViolation
          {"r1_c2", {"m1_l1_1", "m3_1"}}, // capacityViolation
          {"r1_c3", {"m1_l1_2"}},
          {"r1_c4",
           {"m1_f1_0", "m1_f2_0", "m1_f3_0", "m3_2"}}, // capacityViolation
          {"r1_c5", {"m1_f1_1", "m1_f2_1", "m1_f3_1"}},
          {"r1_c6", {"m1_f1_2", "m1_f2_2", "m1_f3_2"}},
          {"r2_c1", {"m2_l1_0"}},
          {"r2_c2", {"m2_f1_0"}},
          {"r2_c3", {}},
          {"r2_c4", {}},
          {"r3_c1", {"m4_0"}},
          {"r3_c2", {"m4_1", "m4_2"}},
          {"r3_c3", {}},
          {DUMMY_CONTAINER_NAME, {}},
      };

  setUpProblem(initialAssignment);

  // add constraints and goals
  addLeaderFollowerConstraints();
  addCapacityConstraints();
  {
    // avoid moving "m1_l1_2", "m3_0", "m3_1", "m3_2"
    AvoidMovingSpec avoidMovingSpec;
    avoidMovingSpec.name() = "avoid moving m3 objects";
    avoidMovingSpec.objects() = {"m1_l1_2", "m3_0", "m3_1", "m3_2"};
    solver_->addConstraint(avoidMovingSpec);
  }

  // add move type spec
  ColocateGroupsMoveTypeSpec colocateMoveTypeSpec;
  colocateMoveTypeSpec.partitionName() = TENANT_PARTITION_NAME;
  colocateMoveTypeSpec.colocationScopeName() = COLOCATION_SCOPE_NAME;
  colocateMoveTypeSpec.relatedGroupsList() = {
      makeRelatedGroupsInfo({"m1_l1", "m1_f1", "m1_f2", "m1_f3"}),
      makeRelatedGroupsInfo({"m2_l1", "m2_f1"}),
  };

  // add solver spec
  interface::LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList() = {
      solver_->makeMoveTypeSpec(std::move(colocateMoveTypeSpec))};

  solver_->addSolver(solverSpec);

  // solve
  auto solution = solver_->solve();

  auto& assignment = *solution.assignment();
  std::map<std::string, int> containerToNumObjects;
  for (auto& [_, container] : assignment) {
    containerToNumObjects[container]++;
  }

  // assert that there are broken constraints initially, but none finally
  ASSERT_EQ(*solution.initialConstraint()->brokenCount(), 1);
  ASSERT_EQ(*solution.finalConstraint()->brokenCount(), 0);

  // expect violations for "r1_c1", "r1_c2", "r1_c4" to be fixed
  ASSERT_LE(containerToNumObjects["r1_c1"], 1);
  ASSERT_LE(containerToNumObjects["r1_c2"], 1);
  ASSERT_LE(containerToNumObjects["r1_c4"], 3);

  // expect all objects in {"m3_0", "m3_1", "m3_2", "m1_l1_2"} to stay in
  // region1
  ASSERT_EQ(assignment["m3_0"], "r1_c1");
  ASSERT_EQ(assignment["m3_1"], "r1_c2");
  ASSERT_EQ(assignment["m3_2"], "r1_c4");
  ASSERT_EQ(assignment["m1_l1_2"], "r1_c3");

  // expect "m1_l1_0", "m1_l1_1" to be moved to region3 since that is the only
  // way to fix the capacity violation
  const std::vector<std::string> region3Destinations = {
      "r3_c1", "r3_c2", "r3_c3"};
  for (auto& object : {"m1_l1_0", "m1_l1_1"}) {
    ASSERT_OBJECT_IN_ANY_CONTAINER(object, region3Destinations, assignment);
  }
}

} // namespace facebook::rebalancer::interface::tests
