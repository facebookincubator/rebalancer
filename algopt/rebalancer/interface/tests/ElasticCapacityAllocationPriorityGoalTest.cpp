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
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <tuple>
#include <vector>

using namespace facebook::rebalancer::interface;
using facebook::algopt::isSolverUnavailable;
using facebook::algopt::solverName;
using facebook::algopt::testSolverPackages;

class ElasticCapacityAllocationPriorityGoalTest
    : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    ElasticCapacityAllocationPriorityGoalTest,
    testThreadCounts());

class ElasticCapacityAllocationPriorityGoalOptimalTest
    : public ::testing::TestWithParam<std::tuple<int, OptimalSolverPackage>> {
 protected:
  void SetUp() override {
    const auto [threads, solver] = GetParam();
    if (isSolverUnavailable(solver)) {
      GTEST_SKIP() << solverName(solver) << " solver not available";
    }
  }
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    ElasticCapacityAllocationPriorityGoalOptimalTest,
    ::testing::Combine(testThreadCounts(), testSolverPackages()));

static CapacitySpec buildCapacitySpec(
    const std::string& name,
    const std::string& dimensionName,
    const std::map<std::string, double>& itemToValue,
    const std::string& scopeName,
    CapacitySpecBound bound) {
  CapacitySpec spec;
  spec.name() = name;

  spec.scope() = scopeName;
  spec.dimension() = dimensionName;
  spec.bound() = bound;

  Limit limit;
  limit.scopeItemLimits() = itemToValue;
  spec.limit() = limit;

  Filter filter;
  filter.itemsWhitelist() = {};
  for (const auto& [container, _] : itemToValue) {
    filter.itemsWhitelist()->emplace_back(container);
  }
  spec.filter() = filter;

  return spec;
}

static void addCapacitySpec(
    std::unique_ptr<ProblemSolver>& solver,
    const std::string& name,
    bool isConstraint,
    const std::map<std::string, double>& itemToValue,
    CapacitySpecBound bound,
    double weight = 1,
    std::optional<int> tuplePos = std::nullopt) {
  auto spec =
      buildCapacitySpec(name, "server_count", itemToValue, "container", bound);
  if (isConstraint) {
    solver->addConstraint(spec);
  } else {
    solver->addGoal(spec, weight, tuplePos);
  }
}

static std::unique_ptr<ProblemSolver> setupProblem(
    int executorThreadCount,
    bool enableGoalTuplePos = false) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = executorThreadCount});
  solver->setObjectName("server");
  solver->setContainerName("container");

  std::vector<std::string> servers = {
      "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10"};

  // lowest priority reservation has all servers initially
  solver->setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"r1:Tier_0:0", {}},
          {"r2:Tier_1:0", {}},
          {"r3:Tier_2:0", {}},
          {"r4:Tier_3:0", servers},
          {"unassigned_container", {}},
      });

  // add elastic problem scopes (reservation, tier, tier_priority)
  const std::map<std::string, std::string> containerToReservationScope = {
      {"r1:Tier_0:0", "r1"},
      {"r2:Tier_1:0", "r2"},
      {"r3:Tier_2:0", "r3"},
      {"r4:Tier_3:0", "r4"},
      {"unassigned_container", "unassigned_scope"}};

  const std::map<std::string, std::vector<std::string>> tierScopeToContainers =
      {
          {"tTier_0", {"r1:Tier_0:0"}},
          {"tTier_1", {"r2:Tier_1:0"}},
          {"tTier_2", {"r3:Tier_2:0"}},
          {"tTier_3", {"r4:Tier_3:0"}},
          {"unassigned_scope", {"unassigned_container"}},
      };

  const std::map<std::string, std::vector<std::string>>
      tierPriorityScopeToContainers = {
          {"pTier_0PRIORITY_0", {"r1:Tier_0:0"}},
          {"pTier_1PRIORITY_1", {"r2:Tier_1:0"}},
          {"pTier_2PRIORITY_2", {"r3:Tier_2:0"}},
          {"pTier_3PRIORITY_3", {"r4:Tier_3:0"}},
      };

  solver->addScope("reservation", containerToReservationScope);
  solver->addScope("tier", tierScopeToContainers);
  solver->addScope("tier_priority", tierPriorityScopeToContainers);

  // constraints
  // requested server count by each reservation not exceeded
  const std::map<std::string, double> requestedAmountPerContainer = {
      {"r1:Tier_0:0", 5},
      {"r2:Tier_1:0", 3},
      {"r3:Tier_2:0", 2},
      {"r4:Tier_3:0", 11},
  };
  addCapacitySpec(
      solver,
      "constraint: upper limit capacity",
      true,
      requestedAmountPerContainer,
      CapacitySpecBound::MAX);

  // goals
  // for each priority, containers in it want max servers
  std::map<std::string, int> priorityToWeight = {
      {"PRIORITY_0", 60},
      {"PRIORITY_1", 50},
      {"PRIORITY_2", 40},
      {"PRIORITY_3", 20},
  };

  addCapacitySpec(
      solver,
      "Priority PRIORITY_0 based allocation",
      false,
      {{"r1:Tier_0:0", servers.size()}},
      CapacitySpecBound::MIN,
      priorityToWeight["PRIORITY_0"],
      enableGoalTuplePos ? 1 : 0);

  addCapacitySpec(
      solver,
      "Priority PRIORITY_1 based allocation",
      false,
      {{"r2:Tier_1:0", servers.size()}},
      CapacitySpecBound::MIN,
      priorityToWeight["PRIORITY_1"],
      enableGoalTuplePos ? 2 : 0);

  addCapacitySpec(
      solver,
      "Priority PRIORITY_2 based allocation",
      false,
      {{"r3:Tier_2:0", servers.size()}},
      CapacitySpecBound::MIN,
      priorityToWeight["PRIORITY_2"],
      enableGoalTuplePos ? 3 : 0);

  addCapacitySpec(
      solver,
      "Priority PRIORITY_3 based allocation",
      false,
      {{"r4:Tier_3:0", servers.size()}},
      CapacitySpecBound::MIN,
      priorityToWeight["PRIORITY_3"],
      enableGoalTuplePos ? 4 : 0);

  return solver;
}

TEST_P(ElasticCapacityAllocationPriorityGoalOptimalTest, OptimalSolverTest) {
  const auto [threads, solverPkg] = GetParam();
  auto solver = setupProblem(threads);

  OptimalSolverSpec solverSpec;
  solverSpec.solverPackage() = solverPkg;
  solver->addSolver(solverSpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  std::map<std::string, int> containerToServerCount;
  for (auto& [_server, container] : assignment) {
    containerToServerCount[container]++;
  }

  // servers go to higher priority reservations first
  EXPECT_EQ(containerToServerCount["r1:Tier_0:0"], 5);
  EXPECT_EQ(containerToServerCount["r2:Tier_1:0"], 3);
  EXPECT_EQ(containerToServerCount["r3:Tier_2:0"], 2);
  EXPECT_EQ(containerToServerCount["r4:Tier_3:0"], 1);
}

TEST_P(ElasticCapacityAllocationPriorityGoalTest, LocalSearchSolverTest) {
  auto solver = setupProblem(GetParam(), true /*enableGoalTuplePos*/);

  auto createStage = [](int goalIdx,
                        std::string scopeName,
                        std::vector<std::string> scopeItemsNames) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage {}", goalIdx);
    stage.begin() = goalIdx;
    stage.end() = goalIdx + 1;
    stage.solverSpec()->exploreMovesFromContainersNotInObjective() = false;
    ScopeItemList scopeItemList;
    scopeItemList.scopeName() = scopeName;
    scopeItemList.scopeItems() = scopeItemsNames;

    SingleFixedSourceMoveTypeSpec singleFixedSourceMoveTypeSpec;
    singleFixedSourceMoveTypeSpec.scopeItemList() = scopeItemList;
    singleFixedSourceMoveTypeSpec.stopEarlyAtScopeItemThatImprovesObjective() =
        true;
    stage.solverSpec()->moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(singleFixedSourceMoveTypeSpec));
    return stage;
  };
  LocalSearchStageSolverSpec localSearchStageSolverSpec;

  localSearchStageSolverSpec.stageSpecs() = {
      createStage(
          1,
          "tier_priority",
          {"pTier_3PRIORITY_3",
           "pTier_2PRIORITY_2",
           "pTier_1PRIORITY_1",
           "pTier_0PRIORITY_0"}),
      createStage(
          2,
          "tier_priority",
          {"pTier_3PRIORITY_3", "pTier_2PRIORITY_2", "pTier_1PRIORITY_1"}),
      createStage(
          3, "tier_priority", {"pTier_3PRIORITY_3", "pTier_2PRIORITY_2"}),
      createStage(4, "tier_priority", {"pTier_3PRIORITY_3"}),
  };
  solver->addSolver(localSearchStageSolverSpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  std::map<std::string, int> containerToServerCount;
  for (auto& [_server, container] : assignment) {
    containerToServerCount[container]++;
  }

  // servers go to higher priority reservations first
  EXPECT_EQ(containerToServerCount["r1:Tier_0:0"], 5);
  EXPECT_EQ(containerToServerCount["r2:Tier_1:0"], 3);
  EXPECT_EQ(containerToServerCount["r3:Tier_2:0"], 2);
  EXPECT_EQ(containerToServerCount["r4:Tier_3:0"], 1);
}
