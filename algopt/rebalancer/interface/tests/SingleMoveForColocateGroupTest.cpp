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
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

namespace facebook::rebalancer::interface::tests {
class SingleMoveForColocateGroupTest : public ::testing::TestWithParam<int> {
 protected:
  std::unique_ptr<ProblemSolver> solver_;
  std::string OBJECT_NAME = "task";
  std::string CONTAINER_NAME = "host";
  std::string PARTITION_NAME = "group";
  std::string SCOPE_NAME = "region";
  std::string DIMENSION_NAME = "task_count";
  std::string SPECIAL_CONTAINER = "dummy";

  /** Instance description:
      - Initial:
        - dummy (unassigned): task0~task99
        - host0: task100~task119
        - host1: task120~task149
        - host2: empty
        - host3: empty
      - Capacity:
        - {capacity1} tasks per host in region0
        - {capacity2} tasks per host in region1
      - Groups:
        - group0: task0~task99
        - group1: task100~task119
        - group2: task120~task149
      - Scope:
        - region0: host0, host1, host2
        - region1: host3
      - Goal: assign all tasks to the same region.
    */
  void setUp(double capacity1, double capacity2) {
    entities::Map<std::string, std::vector<std::string>> initialAssignment = {
        {SPECIAL_CONTAINER, {}},
        {"host0", {}},
        {"host1", {}},
        {"host2", {}},
        {"host3", {}}};
    entities::Map<std::string, std::vector<std::string>> groupToTasks = {
        {"group0", {}}, {"group1", {}}, {"group2", {}}};
    for (const auto taskId : folly::irange(100)) {
      initialAssignment[SPECIAL_CONTAINER].push_back(
          fmt::format("task{}", taskId));
      groupToTasks["group0"].push_back(fmt::format("task{}", taskId));
    }
    for (const auto taskId : folly::irange(20)) {
      initialAssignment["host0"].push_back(fmt::format("task{}", taskId + 100));
      groupToTasks["group1"].push_back(fmt::format("task{}", taskId + 100));
    }
    for (const auto taskId : folly::irange(30)) {
      initialAssignment["host1"].push_back(fmt::format("task{}", taskId + 120));
      groupToTasks["group2"].push_back(fmt::format("task{}", taskId + 120));
    }

    const entities::Map<std::string, std::vector<std::string>> regionToHosts = {
        {"region0", {"host0", "host1", "host2"}},
        {"region1", {"host3"}},
    };
    const entities::Map<std::string, double> containerToMaxObjectCount = {
        {"host0", capacity1},
        {"host1", capacity1},
        {"host2", capacity1},
        {"host3", capacity2},
    };

    solver_ = initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver_->setObjectName(OBJECT_NAME);
    solver_->setContainerName(CONTAINER_NAME);
    solver_->setAssignment(initialAssignment);
    solver_->addPartition(PARTITION_NAME, groupToTasks);
    solver_->addScope(SCOPE_NAME, regionToHosts);
    solver_->addContainerDimension(DIMENSION_NAME, containerToMaxObjectCount);

    CapacitySpec capacitySpec;
    capacitySpec.scope() = CONTAINER_NAME;
    capacitySpec.dimension() = DIMENSION_NAME;
    capacitySpec.filter()->itemsBlacklist() = {SPECIAL_CONTAINER};
    solver_->addConstraint(capacitySpec);
  }
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    SingleMoveForColocateGroupTest,
    testThreadCounts());

TEST_P(SingleMoveForColocateGroupTest, MultiStageSolverWithSingleMove) {
  // In this scenario we have enough capacity in region0, so no swap needed.
  setUp(50, 50);

  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "ToFreeSpec";
  toFreeSpec.containers() = {SPECIAL_CONTAINER};
  toFreeSpec.dimension() = DIMENSION_NAME;
  solver_->addGoal(toFreeSpec);
  solver_->addGoalBoundary();
  ColocateGroupsSpec colocateGroupsSpec;
  colocateGroupsSpec.name() = "ColocateGroupsSpec";
  colocateGroupsSpec.scope() = std::move(SCOPE_NAME);
  colocateGroupsSpec.partitionName() = std::move(PARTITION_NAME);
  solver_->addGoal(colocateGroupsSpec);
  solver_->addGoalBoundary();

  LocalSearchStageSpec localSearchStageSpec1;
  localSearchStageSpec1.begin() = 0;
  localSearchStageSpec1.end() = 1;
  localSearchStageSpec1.solverSpec()->moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  LocalSearchStageSpec localSearchStageSpec2;
  localSearchStageSpec2.begin() = 1;
  localSearchStageSpec2.end() = 2;
  localSearchStageSpec2.solverSpec()->moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  LocalSearchStageSolverSpec localSearchStageSolverSpec;
  localSearchStageSolverSpec.stageSpecs() = {
      localSearchStageSpec1, localSearchStageSpec2};
  localSearchStageSolverSpec.exploreMovesFromContainersNotInObjective() = true;

  solver_->addSolver(localSearchStageSolverSpec);

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  // Count how many tasks from each group are in each host.
  std::map<std::string, int> numTasksPerHost;
  for (auto& [task, host] : *solution.assignment()) {
    ++numTasksPerHost[host];
  }
  EXPECT_EQ(numTasksPerHost["host0"], 50);
  EXPECT_EQ(numTasksPerHost["host1"], 50);
  EXPECT_EQ(numTasksPerHost["host2"], 50);
  EXPECT_EQ(numTasksPerHost["host3"], 0);
}

TEST_P(SingleMoveForColocateGroupTest, MultiStageSolverWithSingleAndSwapMove) {
  // In this scenario we don't have enough capacity in region0, so need to swap
  // tasks to region1.
  setUp(35, 50);

  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "ToFreeSpec";
  toFreeSpec.containers() = {SPECIAL_CONTAINER};
  toFreeSpec.dimension() = DIMENSION_NAME;
  solver_->addGoal(toFreeSpec);
  solver_->addGoalBoundary();
  ColocateGroupsSpec colocateGroupsSpec;
  colocateGroupsSpec.name() = "ColocateGroupsSpec";
  colocateGroupsSpec.scope() = std::move(SCOPE_NAME);
  colocateGroupsSpec.partitionName() = std::move(PARTITION_NAME);
  // Give different weights to incentivize a swap move.
  colocateGroupsSpec.scopeItemWeights() = {{"region0", 100}, {"region1", 1}};
  solver_->addGoal(colocateGroupsSpec);
  solver_->addGoalBoundary();

  LocalSearchStageSpec localSearchStageSpec1;
  localSearchStageSpec1.begin() = 0;
  localSearchStageSpec1.end() = 1;
  localSearchStageSpec1.solverSpec()->moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  LocalSearchStageSpec localSearchStageSpec2;
  localSearchStageSpec2.begin() = 1;
  localSearchStageSpec2.end() = 2;
  localSearchStageSpec2.solverSpec()->moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearchStageSpec2.solverSpec()->moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SwapMoveTypeSpec()));

  LocalSearchStageSolverSpec localSearchStageSolverSpec;
  localSearchStageSolverSpec.stageSpecs() = {
      localSearchStageSpec1, localSearchStageSpec2};
  localSearchStageSolverSpec.exploreMovesFromContainersNotInObjective() = true;

  solver_->addSolver(localSearchStageSolverSpec);

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  // Count how many tasks from each group are in each host.
  std::map<std::string, int> numTasksPerHost;
  for (auto& [task, host] : *solution.assignment()) {
    ++numTasksPerHost[host];
  }
  // 100 unassigned tasks were moved to region0, and 50 tasks were moved from
  // region0 to region1.
  EXPECT_EQ(
      numTasksPerHost["host0"] + numTasksPerHost["host1"] +
          numTasksPerHost["host2"],
      100);
  EXPECT_EQ(numTasksPerHost["host3"], 50);
}
} // namespace facebook::rebalancer::interface::tests
