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
#include <algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSolver_types.h>

#include <folly/container/irange.h>
#include <folly/Portability.h>
#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

class FrozenContainersTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, FrozenContainersTest, testThreadCounts());

static std::unique_ptr<ProblemSolver> setUpLargeProblem(
    int threadCount,
    const int kHosts = 10000,
    const int kTasks = 100000,
    const int kFrozenHosts = 9998) {
  // This is a stress test for the optimization in optimal solver where frozen
  // containers are modeled as constants. An unoptimized version of this problem
  // would require 1B variables, which is an intractable size. However, a
  // properly optimized model requires only 20 variables, as there are only 2
  // non-frozen containers and 10 non-frozen objects.

  auto solver =
      initializeTestProblemSolver({.executorThreadCount = threadCount});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // Place 10 tasks in each host.
  std::map<std::string, std::vector<std::string>> initialAssignment;
  for (const auto taskId : folly::irange(kTasks)) {
    int hostId = taskId * kHosts / kTasks;
    initialAssignment[fmt::format("host{}", hostId)].push_back(
        fmt::format("task{}", taskId));
  }
  solver->setAssignment(initialAssignment);

  // Give each task a different cpu dimension, so they are all different in the
  // eyes of the equivalence sets optimization.
  std::map<std::string, double> taskCpu;
  for (const auto taskId : folly::irange(kTasks)) {
    taskCpu[fmt::format("task{}", taskId)] = taskId * 1e-5;
  }
  solver->addObjectDimension("cpu", taskCpu);

  std::map<std::string, double> hostCpu;
  for (const auto hostId : folly::irange(kHosts)) {
    hostCpu[fmt::format("host{}", hostId)] = 10.0;
  }
  solver->addContainerDimension("cpu", hostCpu);

  // Prevent frozen containers from getting any new objects.
  {
    NonAcceptingSpec spec;
    spec.scope() = "host";
    for (const auto hostId : folly::irange(kFrozenHosts)) {
      spec.items()->push_back(fmt::format("host{}", hostId));
    }
    solver->addConstraint(spec);
  }

  // Prevent objects initially in frozen containers from moving.
  {
    AvoidMovingSpec spec;
    for (const auto hostId : folly::irange(kFrozenHosts)) {
      for (auto& taskName :
           initialAssignment.at(fmt::format("host{}", hostId))) {
        spec.objects()->push_back(taskName);
      }
    }
    solver->addConstraint(spec);
  }

  // Optimize for cpu balance.
  {
    BalanceSpec spec;
    spec.scope() = "host";
    spec.dimension() = "cpu";
    spec.formula() = BalanceSpecFormula::MAX;
    solver->addGoal(spec);
  }
  return solver;
}

TEST_P(FrozenContainersTest, LargeOptimal) {
  auto solver = setUpLargeProblem(GetParam());
  // Use the optimal solver with a time limit of only 10 seconds.
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  if (folly::kIsSanitizeThread &&
      facebook::algopt::getAvailableMIPSolver() ==
          OptimalSolverPackage::GUROBI) {
    // TSAN data races are from inside libgurobi and apparently known false
    // positives; see T236804704.
    GTEST_SKIP() << "Skipping Gurobi under TSAN (T236804704)";
  }
  {
    auto spec = facebook::algopt::makeAvailableOptimalSolverSpec();
    spec.solveTime() = 10;
    solver->addSolver(spec);
  }
  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Expect optimality.
  EXPECT_NEAR(0.499950, *solution.initialObjective()->value(), 1e-8);
  EXPECT_NEAR(0.499900, *solution.finalObjective()->value(), 1e-8);

  // The fixed objects optimization triggered by the "avoid moving" constraint
  // results in only 20 (unique) tasks being considered as dynamic by the
  // optimal solver.
  auto& profile = *solution.problemProfile()->optimalSolverProfile();
  EXPECT_EQ(20, *profile.dynamicEquivalenceSets());

  // Materializer detects and optimizes for fixed containers. The
  // precise combination of "avoid moving objects" and "non-accepting
  // containers" results in only 2 dynamic containers.
  EXPECT_EQ(2, *profile.dynamicContainers());
}

TEST_P(FrozenContainersTest, LargeLocalSearch) {
  // solve using local search, since we just want to verify that local search
  // makes no moves, a relatively small sample size is ok
  constexpr int kHosts = 1000;
  constexpr int kFrozenHosts = 998;
  constexpr int kTasks = 10000;
  auto solver = setUpLargeProblem(GetParam(), kHosts, kTasks, kFrozenHosts);

  // also mark remaining two containers as non-accepting, so local search has no
  // moves left to make and should finish quickly
  NonAcceptingSpec nonAcceptingSpec;
  nonAcceptingSpec.scope() = "host";
  for (int hostId = kFrozenHosts; hostId < kHosts; ++hostId) {
    nonAcceptingSpec.items()->push_back(fmt::format("host{}", hostId));
  }
  solver->addConstraint(nonAcceptingSpec);

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  solver->addSolver(solverSpec);
  // Get a solution from Rebalancer
  const facebook::algopt::Timer timer(true);
  auto solution = solver->solve();
  // solve should finish in 20 seconds or less
  EXPECT_GE(20, timer.getSeconds());
  // no moves should have been evaluated, so eval stats should be empty
  auto& summary = solution.solverSummaries()->at(0);
  EXPECT_FALSE(summary.evalStats().has_value());
}
