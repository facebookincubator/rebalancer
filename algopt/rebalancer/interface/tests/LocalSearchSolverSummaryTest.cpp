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

#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;

class LocalSearchSolverSummaryTest : public ::testing::TestWithParam<int> {
 protected:
  static std::unique_ptr<ProblemSolver> setUpProblem() {
    auto solver =
        initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver->setObjectName("task");
    solver->setContainerName("host");
    solver->setAssignment(
        std::vector<std::pair<std::string, std::vector<std::string>>>{
            {"h0", {"t0", "t1"}},
            {"h1", {}},
        });

    facebook::rebalancer::interface::BalanceSpec balanceSpec;
    balanceSpec.scope() = "host";
    balanceSpec.dimension() = "task_count";
    solver->addGoal(balanceSpec);

    return solver;
  }
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    LocalSearchSolverSummaryTest,
    testThreadCounts());

TEST_P(LocalSearchSolverSummaryTest, basic) {
  auto solver = setUpProblem();
  solver->addSolver(makeDefaultLocalSearchSolver());
  solver->enableMoveStats();
  auto solution = solver->solve();

  EXPECT_EQ(1, solution.solverSummaries()->size());

  auto& solverSummary = solution.solverSummaries()->at(0);

  ASSERT_EQ(std::nullopt, solverSummary.stagesSummaries());

  EXPECT_EQ(4, solverSummary.moveStats()->moveTypes()->size());
  EXPECT_EQ("SINGLE", solverSummary.moveStats()->moveTypes()->at(0));
  EXPECT_EQ("SWAP", solverSummary.moveStats()->moveTypes()->at(1));
  EXPECT_EQ("TRIPLE_LOOP", solverSummary.moveStats()->moveTypes()->at(2));
  EXPECT_EQ("KL_SEARCH", solverSummary.moveStats()->moveTypes()->at(3));

  EXPECT_EQ(1, solution.movesSummary()->size());
  EXPECT_EQ(1, solution.movesSummary()->at(0).moves()->size());
  EXPECT_EQ(std::nullopt, solution.movesSummary()->at(0).stageId());
  ASSERT_TRUE(solution.movesSummary()->at(0).cycleId().has_value());
  EXPECT_EQ(1, solution.movesSummary()->at(0).cycleId().value());

  ASSERT_TRUE(solverSummary.evalStats().has_value());
  EXPECT_GE(*solverSummary.evalStats()->numCycles(), 1);
}

TEST_P(LocalSearchSolverSummaryTest, ZeroSolveTime) {
  auto solver = setUpProblem();
  solver->addSolver(
      makeDefaultLocalSearchSolver(100 /*moveLimit*/, 0 /*timeLimit*/));
  auto solution = solver->solve();
  EXPECT_EQ(1, solution.solverSummaries()->size());
  auto& solverSummary = solution.solverSummaries()->at(0);
  EXPECT_EQ(EndReason::HIT_TIME_LIMIT, solverSummary.endReason());
}

TEST_P(LocalSearchSolverSummaryTest, ZeroMoveLimit) {
  auto solver = setUpProblem();
  solver->addSolver(
      makeDefaultLocalSearchSolver(0 /*moveLimit*/, 100 /*timeLimit*/));
  auto solution = solver->solve();
  EXPECT_EQ(1, solution.solverSummaries()->size());
  auto& solverSummary = solution.solverSummaries()->at(0);
  EXPECT_EQ(EndReason::HIT_MOVE_LIMIT, solverSummary.endReason());
}
