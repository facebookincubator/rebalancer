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

using namespace facebook::rebalancer::interface;
using facebook::algopt::isSolverUnavailable;
using facebook::algopt::solverName;
using facebook::algopt::testSolverPackages;

class RegrettableMovesTest
    : public ::testing::TestWithParam<std::tuple<int, OptimalSolverPackage>> {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    RegrettableMovesTest,
    ::testing::Combine(testThreadCounts(), testSolverPackages()));

TEST_P(RegrettableMovesTest, Basic) {
  const auto [threads, solverPkg] = GetParam();
  if (isSolverUnavailable(solverPkg)) {
    GTEST_SKIP() << solverName(solverPkg) << " not available";
  }
  // This example contains 10 hosts and 6 tasks. The carefully chosen set of
  // regrettable moves creates all possible combinations of moves from/to
  // dynamic/static containers in optimal subset solver.
  auto solver = initializeTestProblemSolver({.executorThreadCount = threads});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // Set initial assignment.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0"}},
          {"host1", {}},
          {"host2", {}},
          {"host3", {"task2"}},
          {"host4", {"task4"}},
          {"host5", {"task1"}},
          {"host6", {"task3"}},
          {"host7", {}},
          {"host8", {}},
          {"host9", {"task5"}},
      });

  // Override list of hosts from hottest to coldest.
  solver->overrideContainerHotnessRanking({
      "host0",
      "host1",
      "host2",
      "host3",
      "host4",
      "host5",
      "host6",
      "host7",
      "host8",
      "host9",
  });

  // Define preferences for tasks to be placed in specific hosts.
  {
    AssignmentAffinitiesSpec spec;
    spec.affinities() = {
        makeAssignmentAffinity("task0", "host3", 1),
        makeAssignmentAffinity("task0", "host8", 2),
        makeAssignmentAffinity("task1", "host0", 1),
        makeAssignmentAffinity("task1", "host8", 2),
        makeAssignmentAffinity("task2", "host0", 1),
        makeAssignmentAffinity("task2", "host5", 2),
        makeAssignmentAffinity("task3", "host0", 3),
        makeAssignmentAffinity("task3", "host5", 4),
        makeAssignmentAffinity("task4", "host0", 5),
        makeAssignmentAffinity("task4", "host5", 6),
        makeAssignmentAffinity("task5", "host0", 7),
        makeAssignmentAffinity("task5", "host5", 8),
    };
    solver->addGoal(spec);
  }

  {
    OptimalSubsetSolverSpec spec;
    spec.maxSubsetRuns() = 1;
    // Force hosts 0-4 to be dynamic, and hosts 5-9 to be static.
    spec.containerChoice() = {{"HOT", 5}};
    // Define regrettable moves.
    spec.regrettableMoves() = {
        {"task0", "host2"},
        {"task1", "host1"},
        {"task2", "host7"},
        {"task3", "host8"},
    };
    OptimalSolverSpec optConfig;
    optConfig.solverPackage() = solverPkg;
    spec.optimalConfig() = optConfig;
    solver->addSolver(spec);
  }

  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  // task0 wants to be in host8 but that host is frozen. It moves to the second
  // preference host3. The regrettable move where task0 moves to host2 is
  // irrelevant.
  EXPECT_EQ("host3", assignment.at("task0"));

  // task1 wants to move to host8 but that host is frozen. It moves to the
  // second preference host0. The regrettable move where task1 moves to host1 is
  // irrelevant.
  EXPECT_EQ("host0", assignment.at("task1"));

  // task2 wants to move to either host0 or host5, but it can't move because
  // it's already regrettably moving to the frozen host host7.
  EXPECT_EQ("host7", assignment.at("task2"));

  // task3 wants to move to either host0 or host5, but it can't move because
  // it's already regrettably moving to the frozen host host8.
  EXPECT_EQ("host8", assignment.at("task3"));

  // The first preference of task4 is host5, but that's a frozen host, so it
  // moves to its second preference host0.
  EXPECT_EQ("host0", assignment.at("task4"));

  // task5 wants to move to either host0 or host5, but it can't move because
  // it's already placed in the frozen host host9.
  EXPECT_EQ("host9", assignment.at("task5"));
}

TEST_P(RegrettableMovesTest, FixedObject) {
  const auto [threads, solverPkg] = GetParam();
  if (isSolverUnavailable(solverPkg)) {
    GTEST_SKIP() << solverName(solverPkg) << " not available";
  }
  // Fixed objects are optimized away from the problem at materialization time,
  // and regrettable moves may mention these objects. This unit test recreates
  // that specific scenario.
  auto solver = initializeTestProblemSolver({.executorThreadCount = threads});
  solver->setObjectName("task");
  solver->setContainerName("host");

  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0"}},
          {"host1", {}},
      });

  {
    AvoidMovingSpec spec;
    spec.objects() = {"task0"};
    solver->addConstraint(spec);
  }

  {
    OptimalSubsetSolverSpec spec;
    spec.maxSubsetRuns() = 1;
    spec.containerChoice() = {{"HOT", 2}};
    spec.regrettableMoves() = {
        {"task0", "host1"},
    };
    OptimalSolverSpec optConfig;
    optConfig.solverPackage() = solverPkg;
    spec.optimalConfig() = optConfig;
    solver->addSolver(spec);
  }

  auto solution = solver->solve();
  auto& assignment = *solution.assignment();
  EXPECT_EQ("host0", assignment.at("task0"));
}

TEST_P(RegrettableMovesTest, WarmStart) {
  const auto [threads, solverPkg] = GetParam();
  if (isSolverUnavailable(solverPkg)) {
    GTEST_SKIP() << solverName(solverPkg) << " not available";
  }
  // This test creates a tricky situation which in the past made the optimal
  // solver create invalid warm start hints. Here we re-create the tricky
  // situation and expect the warm start hints to be handled successfully.
  auto solver = initializeTestProblemSolver({.executorThreadCount = threads});
  solver->setObjectName("task");
  solver->setContainerName("host");

  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0"}},
          {"host1", {"task1", "task2"}},
          {"host2", {}},
      });

  // Override list of hosts from hottest to coldest.
  solver->overrideContainerHotnessRanking({
      "host0",
      "host1",
      "host2",
  });

  solver->addObjectDimension(
      "custom",
      std::map<std::string, double>{{"task0", 1}, {"task1", 2}, {"task2", 2}});

  {
    BalanceSpec spec;
    spec.scope() = "host";
    spec.dimension() = "custom";
    solver->addGoal(spec);
  }

  {
    OptimalSubsetSolverSpec spec;
    spec.maxSubsetRuns() = 1;
    spec.containerChoice() = {{"HOT", 2}};
    spec.regrettableMoves() = {
        {"task2", "host2"},
    };
    OptimalSolverSpec optConfig;
    optConfig.solverPackage() = solverPkg;
    spec.optimalConfig() = optConfig;
    solver->addSolver(spec);
  }

  auto solution = solver->solve();

  auto optimalSolverProfile = solution.problemProfile()->optimalSolverProfile();
  EXPECT_TRUE(optimalSolverProfile); // optional is set

  // Warm start reporting and acceptance are solver-dependent. XPRESS reports
  // and accepts these hints; other solvers may not report warm start status.
  auto isWarmStartSuccessful = optimalSolverProfile->isWarmStartSuccessful();
  if (solverPkg == OptimalSolverPackage::XPRESS) {
    EXPECT_TRUE(isWarmStartSuccessful)
        << "XPRESS should report warm start status";
    EXPECT_TRUE(*isWarmStartSuccessful)
        << "XPRESS should accept warm start hints for this problem";
  }
}
