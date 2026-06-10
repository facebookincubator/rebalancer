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

#include <folly/container/irange.h>
#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;

class MultiStageConfigTest : public ::testing::TestWithParam<int> {
 public:
  static std::unique_ptr<ProblemSolver> basicSetup(int threadCount) {
    auto solver =
        initializeTestProblemSolver({.executorThreadCount = threadCount});
    solver->setObjectName("task");
    solver->setContainerName("host");

    std::vector<std::pair<std::string, std::vector<std::string>>> assignment;
    // 10 containers, each container has 10 tasks
    for (const auto i : folly::irange(10)) {
      auto host = fmt::format("h{}", i);
      std::vector<std::string> tasks;
      tasks.reserve(10);
      for (const auto j : folly::irange(10)) {
        tasks.push_back(fmt::format("h{}_t{}", i, j));
      }
      assignment.emplace_back(host, tasks);
    }

    solver->setAssignment(assignment);

    // for each host i (0 <= i < 8), we have a capacity spec that limits the
    // number of tasks on host i to 0. Each of these capacity specs is in a new
    // stage
    for (const auto i : folly::irange(8)) {
      CapacitySpec capacitySpec;
      capacitySpec.scope() = "host";
      capacitySpec.dimension() = "task_count";
      capacitySpec.bound() = CapacitySpecBound::MAX;
      capacitySpec.limit()->type() = LimitType::RELATIVE;
      capacitySpec.limit()->globalLimit() = 0;
      capacitySpec.filter()->itemsWhitelist() = {fmt::format("h{}", i)};
      capacitySpec.zeroAllowed() = true;
      solver->addGoal(capacitySpec);
      solver->addGoalBoundary();
    }
    return solver;
  }

  static std::unique_ptr<ProblemSolver> basicSetupForTimeLimit(
      int threadCount) {
    // each stage should take a really long time to solve, will need to update
    // this if we optimize the solver
    auto solver =
        initializeTestProblemSolver({.executorThreadCount = threadCount});
    solver->setObjectName("task");
    solver->setContainerName("host");

    std::vector<std::pair<std::string, std::vector<std::string>>> assignment;
    for (const auto i : folly::irange(1000)) {
      auto host = fmt::format("h{}", i);
      std::vector<std::string> tasks(60);
      for (const auto j : folly::irange(60)) {
        tasks[j] = fmt::format("h{}_t{}", i, j);
      }
      assignment.emplace_back(host, tasks);
    }

    solver->setAssignment(assignment);

    std::map<std::string, double> taskMemory;
    for (const auto i : folly::irange(1000)) {
      for (const auto j : folly::irange(60)) {
        taskMemory[fmt::format("h{}_t{}", i, j)] = j;
      }
    }

    solver->addObjectDimension("memory", taskMemory);

    // for each host i (0 <= i < 8), we have a capacity spec that limits the
    // number of tasks on host i to 0. Each of these capacity specs is in a new
    // stage.
    // Without a time limit, each capacity spec will take around 1.01~ seconds
    for (const auto i : folly::irange(8)) {
      CapacitySpec capacitySpec;
      capacitySpec.scope() = "host";
      capacitySpec.dimension() = "task_count";
      capacitySpec.bound() = CapacitySpecBound::MAX;
      capacitySpec.limit()->type() = LimitType::RELATIVE;
      capacitySpec.limit()->globalLimit() = 0;
      capacitySpec.filter()->itemsWhitelist() = {fmt::format("h{}", i)};
      capacitySpec.zeroAllowed() = true;
      solver->addGoal(capacitySpec);
      solver->addGoalBoundary();
    }
    return solver;
  }

  static int getMoveStats(AssignmentSolution& solution, int stageId) {
    return solution.solverSummaries()
        ->at(0)
        .stagesSummaries()
        ->at(stageId)
        .moveStats()
        ->numMoves()
        .value();
  }

  static double getTimeStats(AssignmentSolution& solution, int stageId) {
    return solution.solverSummaries()
        ->at(0)
        .stagesSummaries()
        ->at(stageId)
        .duration()
        .value();
  }
};

INSTANTIATE_TEST_CASE_P(ThreadCounts, MultiStageConfigTest, testThreadCounts());

TEST_P(MultiStageConfigTest, Basic) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  std::vector<std::pair<std::string, std::vector<std::string>>> assignment;

  for (const auto i : folly::irange(10)) {
    auto task1 = fmt::format("t{}", i);
    auto task2 = fmt::format("s{}", i);
    auto host = fmt::format("h{}", i);
    assignment.emplace_back(host, std::vector<std::string>{task1, task2});
  }

  solver->setAssignment(assignment);

  for (const auto i : folly::irange(8)) {
    CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "task_count";
    capacitySpec.bound() = CapacitySpecBound::MAX;
    capacitySpec.limit()->type() = LimitType::RELATIVE;
    capacitySpec.limit()->globalLimit() = 0;
    capacitySpec.filter()->itemsWhitelist() = {fmt::format("h{}", i)};
    capacitySpec.zeroAllowed() = true;
    solver->addGoal(capacitySpec);
    solver->addGoalBoundary();
  }

  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {1, 3, 5, 7};
  multiStageConfig1.moveLimit() = 5;

  MultiStageConfig multiStageConfig2;
  multiStageConfig2.stageIds() = {2, 4, 6, 0};
  multiStageConfig2.moveLimit() = 4;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1, multiStageConfig2};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();

  // first 5 stages should have move, rest should not
  for (const auto i : folly::irange(4)) {
    EXPECT_EQ(2, getMoveStats(solution, i));
  }
  // stage 4 should not have any moves
  EXPECT_EQ(0, getMoveStats(solution, 4));

  // stage 5 should only have 1 move
  EXPECT_EQ(1, getMoveStats(solution, 5));

  for (const auto i : folly::irange(6, 8)) {
    EXPECT_EQ(0, getMoveStats(solution, i));
  }
}

TEST_P(MultiStageConfigTest, RespectStageMoveLimit) {
  auto solver = basicSetup(GetParam());

  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {0, 1, 2, 3, 4, 5, 6, 7};
  multiStageConfig1.moveLimit() = 40;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  // make stage 2 and 3 have move limit = 4 and 6 perspectively
  stageSolverSpec.stageSpecs()->at(2).solverSpec()->stopAfterMoves() = 4;
  stageSolverSpec.stageSpecs()->at(3).solverSpec()->stopAfterMoves() = 6;

  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();

  for (const auto i : folly::irange(2)) {
    EXPECT_EQ(10, getMoveStats(solution, i));
  }
  EXPECT_EQ(4, getMoveStats(solution, 2));
  EXPECT_EQ(6, getMoveStats(solution, 3));
  EXPECT_EQ(10, getMoveStats(solution, 4));
  for (const auto i : folly::irange(5, 8)) {
    EXPECT_EQ(0, getMoveStats(solution, i));
  }
}

TEST_P(MultiStageConfigTest, StagesThatDontExist) {
  auto solver = basicSetup(GetParam());

  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {1, 3, 5, 7, 9, 11, 13, 15};
  multiStageConfig1.moveLimit() = 21;

  MultiStageConfig multiStageConfig2;
  multiStageConfig2.stageIds() = {19, 2, 4, 6, 0};
  multiStageConfig2.moveLimit() = 20;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1, multiStageConfig2};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();

  // first 5 stages should have move, rest should not
  for (const auto i : folly::irange(4)) {
    EXPECT_EQ(10, getMoveStats(solution, i));
  }
  // stage 4 should not have any moves
  EXPECT_EQ(0, getMoveStats(solution, 4));

  // stage 5 should only have 1 move
  EXPECT_EQ(1, getMoveStats(solution, 5));

  for (const auto i : folly::irange(6, 8)) {
    EXPECT_EQ(0, getMoveStats(solution, i));
  }
}

TEST_P(MultiStageConfigTest, RespectGlobalMoveLimit) {
  auto solver = basicSetup(GetParam());

  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {1, 3, 5, 7};
  multiStageConfig1.moveLimit() = 16;

  MultiStageConfig multiStageConfig2;
  multiStageConfig2.stageIds() = {2, 4, 6, 0};
  multiStageConfig2.moveLimit() = 16;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1, multiStageConfig2};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  stageSolverSpec.stopAfterMoves() = 30;
  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();

  // thirty moves expected
  for (const auto i : folly::irange(2)) {
    EXPECT_EQ(10, getMoveStats(solution, i));
  }
  EXPECT_EQ(6, getMoveStats(solution, 2));
  EXPECT_EQ(4, getMoveStats(solution, 3));
  for (const auto i : folly::irange(4, 8)) {
    EXPECT_EQ(0, getMoveStats(solution, i));
  }

  std::map<std::string, int> taskCount;
  for (auto& [_, host] : *solution.assignment()) {
    ++taskCount[host];
  }

  EXPECT_EQ(0, taskCount["h0"]);
  EXPECT_EQ(0, taskCount["h1"]);
  EXPECT_EQ(4, taskCount["h2"]);
  EXPECT_EQ(6, taskCount["h3"]);
}

TEST_P(MultiStageConfigTest, RespectMultiStageLimit) {
  auto solver = basicSetup(GetParam());

  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {1, 3};
  multiStageConfig1.moveLimit() = 2;

  MultiStageConfig multiStageConfig2;
  multiStageConfig2.stageIds() = {2, 0};
  multiStageConfig2.moveLimit() = 2;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1, multiStageConfig2};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  solverSpec.stopAfterMoves() = 9;

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  stageSolverSpec.stopAfterMoves() = 30;
  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();

  EXPECT_EQ(2, getMoveStats(solution, 0));
  EXPECT_EQ(2, getMoveStats(solution, 1));
  EXPECT_EQ(0, getMoveStats(solution, 2));
  EXPECT_EQ(0, getMoveStats(solution, 3));
  EXPECT_EQ(9, getMoveStats(solution, 4));
  EXPECT_EQ(9, getMoveStats(solution, 5));
  EXPECT_EQ(8, getMoveStats(solution, 6));
  EXPECT_EQ(0, getMoveStats(solution, 7));

  std::map<std::string, int> taskCount;
  for (auto& [_, host] : *solution.assignment()) {
    ++taskCount[host];
  }

  EXPECT_EQ(8, taskCount["h0"]);
  EXPECT_EQ(8, taskCount["h1"]);
  EXPECT_EQ(10, taskCount["h2"]);
  EXPECT_EQ(10, taskCount["h3"]);
  EXPECT_EQ(1, taskCount["h4"]);
  EXPECT_EQ(1, taskCount["h5"]);
  EXPECT_EQ(2, taskCount["h6"]);
}

TEST_P(MultiStageConfigTest, RespectOverlappingMultStageMoveLimit) {
  auto solver = basicSetup(GetParam());

  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {1, 3, 6, 7};
  multiStageConfig1.moveLimit() = 15;

  MultiStageConfig multiStageConfig2;
  multiStageConfig2.stageIds() = {2, 0, 1, 4};
  multiStageConfig2.moveLimit() = 15;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1, multiStageConfig2};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  stageSolverSpec.stopAfterMoves() = 30;
  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();

  /*
   We have 2 multiStageConfigs, each with only 15 moves allowed.
   In the setup of the solver, each stage will try to move 10 objects from their
   container This means assuming no move limits, each stage should only make 10
   moves

   Since stage 0 only has a move limit of 15 (from multiStageConfig2), it will
   make all 10 moves multiStageConfig2 will now only have a move limit of 5 for
   future stages

   Stage 1 has overlapping multiStageConfigs as both multiStageConfig1 and
   multiStageConfig2 contains stage 1

   Stage 1 will also try to make 10 moves
   But since multiStageConfig2 now only has a move limit of 5, we will only make
   5 moves

   multiStageConfig2 move limit is now 0
   multiStageConfig1 move limit is now 10 because we made 5 moves

   Since multiStageConfig2 move limit is now 0, stage 2 will make 0 moves

   Stage 3 will use 10 moves and make multiStageConfig1 have 0 moves for its
   move limit

   Our current total moves made is 25

   Since stage 5 is not in a multiStageConfig, it will make 5 moves as we have
   made 25 moves already and the global move limit is 30
  */
  EXPECT_EQ(10, getMoveStats(solution, 0));
  EXPECT_EQ(5, getMoveStats(solution, 1));
  EXPECT_EQ(0, getMoveStats(solution, 2));
  EXPECT_EQ(10, getMoveStats(solution, 3));
  EXPECT_EQ(0, getMoveStats(solution, 4));
  EXPECT_EQ(5, getMoveStats(solution, 5));
  EXPECT_EQ(0, getMoveStats(solution, 6));
  EXPECT_EQ(0, getMoveStats(solution, 7));
}

TEST_P(MultiStageConfigTest, RespectStopAfterMovesTillStage) {
  auto solver = basicSetup(GetParam());

  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {0, 1, 2, 3, 4, 5, 6, 7};
  multiStageConfig1.moveLimit() = 40;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    if (i == 2) {
      stage.stopAfterMovesTillStage() = 22;
    }
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();

  for (const auto i : folly::irange(2)) {
    EXPECT_EQ(10, getMoveStats(solution, i));
  }
  EXPECT_EQ(2, getMoveStats(solution, 2));
  EXPECT_EQ(10, getMoveStats(solution, 3));
  EXPECT_EQ(8, getMoveStats(solution, 4));
  for (const auto i : folly::irange(5, 8)) {
    EXPECT_EQ(0, getMoveStats(solution, i));
  }
}

TEST_P(MultiStageConfigTest, BasicTimeSetup) {
  auto solver = basicSetupForTimeLimit(GetParam());
  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  multiStageConfig1.solveTime() = 3;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();
  double actualSolveTime = 0;
  for (const auto i : folly::irange(10)) {
    actualSolveTime += getTimeStats(solution, i);
  }

  // I don't expect the actual solve time to be exactly 3.0, but it should be
  // close
  EXPECT_LE(actualSolveTime, 4);
}

TEST_P(MultiStageConfigTest, RespectGlobalTimeLimit) {
  auto solver = basicSetupForTimeLimit(GetParam());
  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  multiStageConfig1.solveTime() = 3;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  stageSolverSpec.solveTime() = 2;

  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();
  double actualSolveTime = 0;
  for (const auto i : folly::irange(10)) {
    actualSolveTime += getTimeStats(solution, i);
  }

  // I don't expect the actual solve time to be exactly 2.0, but it should be
  // close
  EXPECT_LT(actualSolveTime, 3.0);
}

TEST_P(MultiStageConfigTest, RespectStageTimeLimit) {
  auto solver = basicSetupForTimeLimit(GetParam());
  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  multiStageConfig1.solveTime() = 4;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    if (i % 2 == 0) {
      stage.solverSpec()->solveTime() = 1;
    } else {
      stage.solverSpec()->solveTime() = 0;
    }
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();
  double actualSolveTime = 0;
  for (const auto i : folly::irange(10)) {
    actualSolveTime += getTimeStats(solution, i);
  }

  // I don't expect the actual solve time to be exactly 4.0, but it should be
  // close
  EXPECT_LE(actualSolveTime, 5);

  // all odd stages should have 0 time
  for (int i = 1; i < 10; i += 2) {
    EXPECT_NEAR(getTimeStats(solution, i), 0, 0.01);
  }
}

TEST_P(MultiStageConfigTest, RespectMultipleMultiStageLimit) {
  auto solver = basicSetupForTimeLimit(GetParam());
  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {0, 1};
  multiStageConfig1.solveTime() = 2;

  MultiStageConfig multiStageConfig2;
  multiStageConfig2.stageIds() = {1};
  multiStageConfig2.solveTime() = 1;

  MultiStageConfig multiStageConfig3;
  multiStageConfig3.stageIds() = {2, 3, 4, 5, 6, 7, 8, 9};
  multiStageConfig3.solveTime() = 0;

  stageSolverSpec.multiStageConfigs() = {
      multiStageConfig1, multiStageConfig2, multiStageConfig3};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();

  // first 2 stages should have solve time less than or equal to 5 seconds
  double actualSolveTime = 0;
  for (const auto i : folly::irange(2)) {
    actualSolveTime += getTimeStats(solution, i);
  }
  EXPECT_LE(actualSolveTime, 5.0);

  // Wall-clock duration is bounded above by the configured stage time limit,
  // but the solver can't preempt mid-evaluation, so it routinely overshoots
  // by microseconds. Allow a small slack to absorb scheduling jitter.
  EXPECT_LE(getTimeStats(solution, 1), 1.05);
}

TEST_P(MultiStageConfigTest, RespectBothMoveLimitAndStageTime) {
  auto solver = basicSetupForTimeLimit(GetParam());
  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {0};
  multiStageConfig1.solveTime() = 1;
  multiStageConfig1.moveLimit() = 600;

  MultiStageConfig multiStageConfig2;
  multiStageConfig2.stageIds() = {1};
  multiStageConfig2.solveTime() = 2;
  multiStageConfig2.moveLimit() = 10;

  MultiStageConfig multiStageConfig3;
  multiStageConfig3.stageIds() = {2, 3, 4, 5, 6, 7, 8, 9};
  multiStageConfig3.solveTime() = 0;

  stageSolverSpec.multiStageConfigs() = {
      multiStageConfig1, multiStageConfig2, multiStageConfig3};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();

  EXPECT_LE(getTimeStats(solution, 0), 2.0);
  EXPECT_LE(getMoveStats(solution, 0), 60);

  EXPECT_LE(getTimeStats(solution, 1), 3.0);
  EXPECT_LE(getMoveStats(solution, 1), 10);
}

TEST_P(MultiStageConfigTest, RespectMinStageTimeLimit) {
  auto solver = basicSetupForTimeLimit(GetParam());
  LocalSearchStageSolverSpec stageSolverSpec;

  MultiStageConfig multiStageConfig1;
  multiStageConfig1.stageIds() = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  multiStageConfig1.solveTime() = 0;

  stageSolverSpec.multiStageConfigs() = {multiStageConfig1};

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

  for (const auto i : folly::irange(10)) {
    LocalSearchStageSpec stage;
    stage.name() = fmt::format("stage{}", i);
    stage.begin() = i;
    stage.end() = i + 1;
    stage.solverSpec() = solverSpec;
    stage.minRuntimeSec() = 1;
    stageSolverSpec.stageSpecs()->push_back(stage);
  }

  solver->addSolver(stageSolverSpec);
  auto solution = solver->solve();
  double actualSolveTime = 0;
  for (const auto i : folly::irange(10)) {
    actualSolveTime += getTimeStats(solution, i);
  }

  // minStageTime should be respected
  EXPECT_LE(actualSolveTime, 11);
  EXPECT_GT(actualSolveTime, 0);
}
