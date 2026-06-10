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

#include <string>

using namespace facebook::rebalancer::interface;

class AvoidMovingTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, AvoidMovingTest, testThreadCounts());

TEST_P(AvoidMovingTest, PreventObjectFromMovingToPreferredContainer) {
  // In this example there are 2 hosts and 3 tasks. All tasks are initially
  // placed in host0. Two tasks prefer to be placed in host1, but one of them
  // is prevented from moving with an AvoidMoving constraint. In an optimal
  // solution, only one of the tasks moves to host1.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // host0 initially has 3 tasks in it, and host1 is empty.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {}},
      });

  // task0 and task1 prefer to be placed in host1.
  AssignmentAffinitiesSpec assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";

  assignmentAffinitiesSpec.affinities() = {
      makeAssignmentAffinity("task0", "host1", 1),
      makeAssignmentAffinity("task1", "host1", 1),
  };

  solver->addGoal(assignmentAffinitiesSpec);

  // Avoid moving task0.
  AvoidMovingSpec avoidMovingSpec;
  avoidMovingSpec.objects() = {"task0"};
  solver->addConstraint(avoidMovingSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // We expect only task1 to move from host0 to host1. task0 would prefer host1
  // but it can't move. task2 doesn't have an incentive to move.
  EXPECT_EQ("host0", assignment["task0"]);
  EXPECT_EQ("host1", assignment["task1"]);
  EXPECT_EQ("host0", assignment["task2"]);
}
