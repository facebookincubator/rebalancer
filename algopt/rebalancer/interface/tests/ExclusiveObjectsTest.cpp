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

class ExclusiveObjectsTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, ExclusiveObjectsTest, testThreadCounts());

TEST_P(ExclusiveObjectsTest, ExclusiveObjectsConstraint) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  const std::map<std::string, std::vector<std::string>> initialAssignment = {
      {"host1", {"task1"}},
      {"host2", {}},
      {"host3", {"task2", "task3"}},
      {"host4", {"task4"}},
  };
  solver->setAssignment(initialAssignment);

  {
    auto pair = ObjectPair();
    pair.object1() = "task1";
    pair.object2() = "task4";

    ExclusiveObjectsSpec exclusiveObjectsSpec;
    exclusiveObjectsSpec.name() = "test_constraint";
    exclusiveObjectsSpec.scope() = "host";
    exclusiveObjectsSpec.pairs() = {std::move(pair)};
    solver->addConstraint(std::move(exclusiveObjectsSpec));
  }

  {
    auto pair = ObjectPair();
    pair.object1() = "task2";
    pair.object2() = "task3";

    ExclusiveObjectsSpec exclusiveObjectsSpec;
    exclusiveObjectsSpec.name() = "test_goal";
    exclusiveObjectsSpec.scope() = "host";
    exclusiveObjectsSpec.pairs() = {std::move(pair)};
    solver->addGoal(std::move(exclusiveObjectsSpec));
  }

  // Use the optimal solver, rather than the default (local search).
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Initial constraint is satisfied, hence constraint value is 0
  EXPECT_NEAR(0, *solution.initialConstraint()->brokenVal(), 1e-9);
  EXPECT_EQ(0, *solution.initialConstraint()->brokenCount());
  ASSERT_EQ(1, solution.initialConstraint()->constraints()->size());
  EXPECT_EQ(
      "test_constraint",
      *solution.initialConstraint()->constraints()->at(0).name());
  EXPECT_EQ(0, *solution.initialConstraint()->constraints()->at(0).value());

  EXPECT_EQ(
      "test_goal",
      *solution.initialGlobalObjective()->goals()->at(0).objs()->at(0).name());

  // ensure host for task2 and task3 are not same, ExclusiveObjectsSpec goal
  // will ensure task is moved to new host
  auto assignment = *solution.assignment();
  auto initalAssignment = *solution.initialAssignment();
  EXPECT_TRUE(assignment.at("task2") != assignment.at("task3"));
}
