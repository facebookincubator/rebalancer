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

namespace facebook::rebalancer::interface::tests {

class MinCycleObjectiveImprovementTest : public ::testing::TestWithParam<int> {
 protected:
  static std::unique_ptr<ProblemSolver> makeProblem(int executorThreadCount) {
    auto solver = initializeTestProblemSolver(
        {.executorThreadCount = executorThreadCount});

    solver->setObjectName("shard");
    solver->setContainerName("host");

    const std::map<std::string, std::vector<std::string>> initial_assignment = {
        {"h1", {"s1", "s2", "s3", "s4", "s5", "s6"}},
        {"h2", {}},
        {"h3", {}},
    };
    solver->setAssignment(initial_assignment);

    const std::map<std::string, double> shard_cpu = {
        {"s1", 11},
        {"s2", 23},
        {"s3", 37},
        {"s4", 17},
        {"s5", 29},
        {"s6", 7},
    };
    const std::map<std::string, double> host_cpu = {
        {"h1", 200},
        {"h2", 200},
        {"h3", 200},
    };
    solver->addObjectDimension("cpu", shard_cpu);
    solver->addContainerDimension("cpu", host_cpu);

    CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "cpu";
    capacitySpec.name() = "do not exceed cpu";
    solver->addConstraint(capacitySpec);

    BalanceSpec balanceSpec;
    balanceSpec.scope() = "host";
    balanceSpec.dimension() = "cpu";
    balanceSpec.name() = "balance cpu";
    solver->addGoal(balanceSpec, /*weight=*/100);

    return solver;
  }

  static LocalSearchSolverSpec makeSolverSpec() {
    LocalSearchSolverSpec spec;
    spec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleFastMoveTypeSpec{}));
    return spec;
  }
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    MinCycleObjectiveImprovementTest,
    testThreadCounts());

TEST_P(MinCycleObjectiveImprovementTest, StopsEarlyWithHighAbsoluteThreshold) {
  auto solver = makeProblem(GetParam());

  auto spec = makeSolverSpec();
  MinCycleObjectiveImprovementConfig config;
  config.defaultThreshold()->absolute() = 1e9;
  spec.minCycleObjectiveImprovement() = config;

  solver->addSolver(spec);
  const auto solution = solver->solve();

  ASSERT_EQ(1, solution.solverSummaries()->size());
  EXPECT_EQ(
      EndReason::HIT_MIN_CYCLE_OBJECTIVE_IMPROVEMENT,
      *solution.solverSummaries()->at(0).endReason());
  EXPECT_GT(solution.movesSummary()->size(), 0);
}

TEST_P(MinCycleObjectiveImprovementTest, StopsEarlyWithHighPercentThreshold) {
  auto solver = makeProblem(GetParam());

  auto spec = makeSolverSpec();
  MinCycleObjectiveImprovementConfig config;
  // 100% means improvement must exceed the entire current value — impossible
  config.defaultThreshold()->percent() = 100.0;
  spec.minCycleObjectiveImprovement() = config;

  solver->addSolver(spec);
  const auto solution = solver->solve();

  ASSERT_EQ(1, solution.solverSummaries()->size());
  EXPECT_EQ(
      EndReason::HIT_MIN_CYCLE_OBJECTIVE_IMPROVEMENT,
      *solution.solverSummaries()->at(0).endReason());
  EXPECT_GT(solution.movesSummary()->size(), 0);
}

} // namespace facebook::rebalancer::interface::tests
