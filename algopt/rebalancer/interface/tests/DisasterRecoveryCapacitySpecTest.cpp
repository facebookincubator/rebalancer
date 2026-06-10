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

#include "algopt/rebalancer/algopt_common/TestUtils.h"
#include "algopt/rebalancer/interface/tests/utils.h"
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

class DisasterRecoveryCapacitySpecTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    solver_ = initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver_->setObjectName("task");
    solver_->setContainerName("host");

    solver_->setAssignment(
        std::vector<std::pair<std::string, std::vector<std::string>>>{
            {"host0", {"task0", "task1", "task2"}},
            {"host1", {"task3", "task4", "task5", "task6"}},
            {"host2", {}},
        });

    std::map<std::string, double> tasksToLoadOrCapacity;
    tasksToLoadOrCapacity = {
        {"task0", 1},
        {"task1", 2},
        {"task2", 5},
        {"task3", 1},
        {"task4", 2},
        {"task5", 5},
        {"task6", 5},
    };

    solver_->addObjectDimension("load", tasksToLoadOrCapacity);
    solver_->addScopeDimension(
        "load",
        "host",
        std::unordered_map<std::string, double>{
            {"host0", 0}, {"host1", 0}, {"host2", 0}});

    // defining another dimension called "capacity" for objects (with same
    // values as "load") and scopeItems
    solver_->addObjectDimension("capacity", tasksToLoadOrCapacity);
    solver_->addScopeDimension(
        "capacity",
        "host",
        std::map<std::string, double>{
            {"host0", 6}, {"host1", 8}, {"host2", 4}});
  }

  std::unique_ptr<ProblemSolver> solver_;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    DisasterRecoveryCapacitySpecTest,
    testThreadCounts());

TEST_P(DisasterRecoveryCapacitySpecTest, SecondaryObjectsNotASet) {
  // Tests that an error is raised if secondaryObjects associated with a
  // primaryObject has a repetition
  DisasterRecoveryCapacitySpec spec;
  spec.scope() = "host";
  spec.dimension() = "load";
  spec.primaryToSetOfSecondaryObjects() = {
      {"task0", {"task1"}}, {"task2", {"task3", "task4", "task3"}}};
  spec.sharedDisasterGroups() = {{"host0", "host1"}, {"host1", "host2"}};

  REBALANCER_EXPECT_RUNTIME_ERROR(
      solver_->addGoal(spec),
      "The set of secondary objects associated with primaryObject task2 has a repetition.");
}

TEST_P(DisasterRecoveryCapacitySpecTest, OnlyPrimaryObjectsMatter) {
  // This tests that the primaryValues are computed correctly. The example is
  // defined in SetUp(). In this example, there are three containers
  // (host0...host2, which are also scopeItems) and 6 objects (task0...task5).
  // Initially, task0...task2 are in host0 and the rest are in host1.
  DisasterRecoveryCapacitySpec spec;
  spec.scope() = "host";
  spec.dimension() = "load";
  spec.primaryToSetOfSecondaryObjects() = {
      {"task0", {"task3"}},
      {"task1", {"task4"}},
      {"task2", {"task5"}},
  };

  solver_->addGoal(spec);
  solver_->addSolver(makeDefaultLocalSearchSolver());
  auto solution = solver_->solve();

  /*
  Since there are no constraints on how a pair of primary and secondary object
  are to be placed, the best one can do is to ensure that place both are placed
  in the same scopeItem. This ensures that only the "primary usage" is counted
  (i.e., "load" associated with primary Objects) since whenever a disaster
  happens, a primary object and its corresponding secondary object are removed
  together.

  Hence, final objective value is 1 (for task0) + 2(for task1) + 5(for task2)
  = 8.
  (NOTE: values are not normalized since totalCapacity of the hosts is 0)
  */
  EXPECT_NEAR(8.0, *solution.finalObjective()->value(), 1e-8);
}

TEST_P(
    DisasterRecoveryCapacitySpecTest,
    PrimaryAndSecondaryNotInSameScopeItem) {
  DisasterRecoveryCapacitySpec spec;
  ExclusiveObjectsSpec exclusiveObjectsSpec;

  auto pair = ObjectPair();
  pair.object1() = "task2";
  pair.object2() = "task5";

  exclusiveObjectsSpec.scope() = "host";
  exclusiveObjectsSpec.pairs() = {std::move(pair)};

  spec.scope() = "host";
  spec.dimension() = "load";
  spec.primaryToSetOfSecondaryObjects() = {
      {"task0", {"task3"}},
      {"task1", {"task4"}},
      {"task2", {"task5"}},
  };

  // adding an ExclusiveObjectSpec to ensure that task2 and task5 are not placed
  // together
  solver_->addConstraint(exclusiveObjectsSpec);
  solver_->addGoal(spec);

  solver_->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver_->solve();

  /*
  Unlike in the test OnlyPrimaryObjectsMatter where there is no constraint on
  how objects should be placed, here task2 and task5 cannot be placed together.
  This implies that when host which has task2 goes offline due to a disaster,
  one of the other hosts (the one where task5 is placed) will handle its load.

  Hence, we have,
    Primary Usage = 1 (for task0) + 2(for task1) + 5(for task2) = 8.

    DisasterRecoveryUsage = 5 (for task5, which is secondaryObject associated
  with task2)

  Final objective value = 8 + 5 = 13.
  (NOTE: values are not normalized since totalCapacity of the hosts is 0)
  */

  EXPECT_NEAR(13.0, *solution.finalObjective()->value(), 1e-8);
}

TEST_P(DisasterRecoveryCapacitySpecTest, overlappingDisasterGroups) {
  std::map<std::string, std::vector<std::string>> primarToSetOfSecondaryObjects;
  primarToSetOfSecondaryObjects = {
      {"task0", {"task3"}},
      {"task1", {"task4"}},
      {"task2", {"task5", "task6"}},
  };

  {
    DisasterRecoveryCapacitySpec spec;
    spec.scope() = "host";
    spec.dimension() = "capacity";
    spec.primaryToSetOfSecondaryObjects() = primarToSetOfSecondaryObjects;

    // disaster can simultaneously happen at any pair of hosts
    spec.sharedDisasterGroups() = {
        {"host0", "host1"},
        {"host1", "host2"},
        {"host0", "host2"},
    };

    solver_->addGoal(spec);
  }

  {
    // avoid assigning "task2" and "task4" to "host0" or "host1"
    AvoidAssignmentsSpec avoidSpec;
    avoidSpec.scope() = "host";
    avoidSpec.assignments() = {
        makeAvoidAssignment("task2", {"host0", "host1"}),
        makeAvoidAssignment("task4", {"host0", "host1"}),
    };

    solver_->addConstraint(avoidSpec);
  }

  // use optimal solver
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  auto optimalSolver = facebook::algopt::makeAvailableOptimalSolverSpec();
  optimalSolver.solveTime() = 10;
  solver_->addSolver(optimalSolver);

  /*
  Given the scopeDimension "load", the avoidAssignment constraint, and the
  incentive to minimize the excess disasterRecoveryCapacity required, the
  following assignment is (one of) the best as it results in an excess capacity
  of zero at hosts 0 and host1, and a (normalized) excess capacity of 0.5 at
  host3.

  "host0" : "task0", "task6"
  "host1" : "task1", "task3", "task5"
  "host2" : "task2", "task4"

  Hence, we have,

  for host0: primaryUsage = 1, maxDisasterRecoveryUsage = 5 (because of "task6"
  for the group ("host1", "host2"));
  excess = totalUsage - load of "host0" = ((1 + 5) - 6) / 6  = 0, where division
  by 6 is because of normalization by the average caapcity of the hosts (which
  is 6)

  for host1: primaryUsage = 2, maxDisasterRecoveryUsage = 6 (for the group
  ("host0", "host2"));
  excess = totalUsage - load of "host1" = ((2 + 6) - 8) / 6 = 0

  for host2: primaryUsage = 5, maxDisasterRecoveryUsage = 2 (for the group
  ("host0", "host1"));
  excess = totalUsage - load of "host2" = ((5 + 2) - 4) / 6 = 0.5

  Final objective value = 0.5
  */

  auto solution = solver_->solve();
  EXPECT_NEAR(0.5, *solution.finalObjective()->value(), 1e-8);
}

TEST_P(DisasterRecoveryCapacitySpecTest, DynamicObjectDimension) {
  std::map<std::string, std::vector<std::string>>
      primaryToSetOfSecondaryObjects;
  primaryToSetOfSecondaryObjects = {
      {"task0", {"task3"}},
      {"task1", {"task4"}},
      {"task2", {"task5", "task6"}},
  };

  // task3 varies in load between host 0 & host 1. It is also more than it's
  // primary, task0
  std::map<std::string, std::map<std::string, double>> hostToTaskToLoad;
  hostToTaskToLoad = {
      {
          "host0",
          {
              {"task0", 1},
              {"task1", 2},
              {"task3", 1},
              {"task5", 5},
              {"task6", 5},
          },
      },
      {
          "host1",
          {
              {"task0", 1},
              {"task1", 2},
              {"task2", 5},
              {"task3", 2},
              {"task4", 2},
              {"task5", 5},
              {"task6", 5},
          },
      },
      {
          "host2",
          {{"task2", 5}, {"task4", 2}},
      },
  };

  solver_->addDynamicObjectDimension(
      "dynamic_load", "host", hostToTaskToLoad, /*defaultValue=*/100);
  // scope dimension is set to 0 to make goal to minimize the excess
  solver_->addScopeDimension(
      "dynamic_load",
      "host",
      std::unordered_map<std::string, double>{
          {"host0", 0}, {"host1", 0}, {"host2", 0}});

  {
    DisasterRecoveryCapacitySpec spec;
    spec.scope() = "host";
    spec.dimension() = "dynamic_load";
    spec.primaryToSetOfSecondaryObjects() = primaryToSetOfSecondaryObjects;

    // disaster can simultaneously happen at any pair of hosts
    spec.sharedDisasterGroups() = {
        {"host0", "host1"},
        {"host1", "host2"},
        {"host0", "host2"},
    };

    solver_->addGoal(spec);
  }

  {
    // avoid assigning "task2" and "task4" to "host0" or "host1"; also avoid
    // "task3" at "host0 and "task0" at "host1
    AvoidAssignmentsSpec avoidSpec;
    avoidSpec.scope() = "host";
    avoidSpec.assignments() = {
        makeAvoidAssignment("task2", {"host0", "host1"}),
        makeAvoidAssignment("task4", {"host0", "host1"}),
        makeAvoidAssignment("task3", {"host0"}),
        makeAvoidAssignment("task0", {"host1"}),
    };

    solver_->addConstraint(avoidSpec);
  }

  // use optimal solver
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  auto optimalSolver = facebook::algopt::makeAvailableOptimalSolverSpec();
  optimalSolver.solveTime() = 10;
  solver_->addSolver(optimalSolver);

  /*
  Given the scopeDimension "dynamic_load", the avoidAssignment constraint, and
  the incentive to minimize the excess disasterRecoveryCapacity required, the
  following assignment is (one of) the best as it results in an excess capacity
  of zero at host 0, and excess capacity of 3 at host1, and 6 at host2.

  "host0" : "task0",
  "host1" : "task1", "task3"
  "host2" : "task2", "task4", "task5"

  Hence, we have,

  for host0: primaryUsage = 1, maxDisasterRecoveryUsage = 0
  excess = totalUsage - 'dynamic_load' dimension value of "host0" = ((1) - 0)  =
  1

  for host1: primaryUsage = 2, maxDisasterRecoveryUsage = 2 (w.r.t. "task3" for
  the group "host0", "host2"));
  excess = totalUsage - 'dynamic_load' dimension value of "host1" = ((2 + 2) -
  0) = 4

  for host2: primaryUsage = 5, maxDisasterRecoveryUsage = 2 (w.r.t. "task4" for
  the group ("host0", "host1"));
  excess = totalUsage - 'dynamic_load' dimension value of "host2" = ((5 + 2) -
  0)  = 7

  Final objective value = 1 + 4 + 7 = 12
  */

  auto solution = solver_->solve();
  EXPECT_NEAR(12.0, *solution.finalObjective()->value(), 1e-8);
}
