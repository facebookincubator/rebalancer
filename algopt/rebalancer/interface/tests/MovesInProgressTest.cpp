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

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

class MovesInProgressTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    solver = initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver->setObjectName("task");
    solver->setContainerName("host");

    // host0 initially has 3 tasks and host1 has 1 task.
    initialAssignment = {
        {"host0", {"task0", "task1", "task2"}},
        {"host1", {"task3"}},
    };
    solver->setAssignment(initialAssignment);

    // Constrain that task2 is moving from host0 to host1. Rebalancer must
    // respect this constraint and it may not choose a different destination
    // for task2.
    facebook::rebalancer::interface::MovesInProgressSpec movesInProgressSpec;
    facebook::rebalancer::interface::MoveInProgress move;
    move.objName() = "task2";
    move.toContainer() = "host1";
    movesInProgressSpec.moves() = {move};
    solver->addConstraint(movesInProgressSpec);

    solver->addSolver(makeDefaultLocalSearchSolver());
  }

  std::unique_ptr<ProblemSolver> solver;
  std::map<std::string, std::vector<std::string>> initialAssignment;
};

INSTANTIATE_TEST_CASE_P(ThreadCounts, MovesInProgressTest, testThreadCounts());

TEST_P(MovesInProgressTest, SingleConstraint) {
  // In this example we run Rebalancer with only the MovesInProgress constraint.
  // We expect the move in progress to be materialized in the solution, without
  // any other changes.

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  // task2 has moved from host0 to host1. There's no incentive for other tasks
  // to move from their initial assignment.
  EXPECT_EQ("host0", assignment["task0"]);
  EXPECT_EQ("host0", assignment["task1"]);
  EXPECT_EQ("host1", assignment["task2"]);
  EXPECT_EQ("host1", assignment["task3"]);

  // Move in progress should NOT apply the specified moves to the initial
  // assignment
  auto& initialAssignment = *solution.initialAssignment();
  for (auto& [task, host] : initialAssignment) {
    EXPECT_EQ(initialAssignment[task], host);
  }
}

TEST_P(MovesInProgressTest, CapacityAfter) {
  // In this example we add a Capacity constraint forcing hosts to have at most
  // 3 tasks each "after" the moves are performed. An AssignmentAffinities goal
  // makes task3 prefer host0. Rebalancer can move task3 from host1 to host0
  // without violating the Capacity constraint.

  // Hosts may not have more than 3 tasks each "after" all moves are performed.
  facebook::rebalancer::interface::CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "task_count";
  capacitySpec.definition() =
      facebook::rebalancer::interface::CapacitySpecDefinition::AFTER;

  facebook::rebalancer::interface::Limit limit;
  limit.type() = facebook::rebalancer::interface::LimitType::ABSOLUTE;
  limit.globalLimit() = 3;
  capacitySpec.limit() = limit;

  solver->addConstraint(capacitySpec);

  // task3 has a preference for host0.
  facebook::rebalancer::interface::AssignmentAffinitiesSpec
      assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";

  facebook::rebalancer::interface::AssignmentAffinity affinity;
  affinity.objectName() = "task3";
  affinity.scopeItemName() = "host0";
  affinity.affinity() = 1;
  assignmentAffinitiesSpec.affinities() = {affinity};

  solver->addGoal(assignmentAffinitiesSpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // task2 has moved from host0 to host1. Additionally, task3 moves from host1
  // to host0 to match its preference. This move does not violate the capacity
  // constraint, as host0 has exactly 3 tasks "after" performing the moves.
  EXPECT_EQ("host0", assignment["task0"]);
  EXPECT_EQ("host0", assignment["task1"]);
  EXPECT_EQ("host1", assignment["task2"]);
  EXPECT_EQ("host0", assignment["task3"]);
}

TEST_P(MovesInProgressTest, CapacityDuring) {
  // In this example we add a Capacity constraint forcing hosts to have at most
  // 3 tasks each "during" the performance of moves. An AssignmentAffinities
  // goal makes task3 prefer host0. Rebalancer can't move task3 from host1 to
  // host0 without violating the Capacity constraint.

  // Hosts may not have more than 3 tasks each at any time, including "during"
  // the performance of moves.
  facebook::rebalancer::interface::CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "task_count";
  capacitySpec.definition() =
      facebook::rebalancer::interface::CapacitySpecDefinition::DURING;

  facebook::rebalancer::interface::Limit limit;
  limit.type() = facebook::rebalancer::interface::LimitType::ABSOLUTE;
  limit.globalLimit() = 3;
  capacitySpec.limit() = limit;

  solver->addConstraint(capacitySpec);

  // task3 has a preference for host0.
  facebook::rebalancer::interface::AssignmentAffinitiesSpec
      assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";

  facebook::rebalancer::interface::AssignmentAffinity affinity;
  affinity.objectName() = "task3";
  affinity.scopeItemName() = "host0";
  affinity.affinity() = 1;
  assignmentAffinitiesSpec.affinities() = {affinity};

  solver->addGoal(assignmentAffinitiesSpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // Only task2 moves from host0 to host1. Moving task3 from host1 to host0 to
  // match its preference would violate the Capacity "during" constraint. If
  // Rebalancer moved task3, then host0 would have 4 tasks while the moves are
  // being performed (all tasks would be concurrently in host0) and would
  // violate the Capacity constraint with definition "during".
  EXPECT_EQ("host0", assignment["task0"]);
  EXPECT_EQ("host0", assignment["task1"]);
  EXPECT_EQ("host1", assignment["task2"]);
  EXPECT_EQ("host1", assignment["task3"]);
}

TEST_P(MovesInProgressTest, SameOriginAndDestination) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // 2 hosts, initially 1 task in each.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0"}},
          {"host1", {"task1"}},
      });

  // Override final container of task0 with host0, the same as the initial.
  facebook::rebalancer::interface::MovesInProgressSpec movesInProgressSpec;
  facebook::rebalancer::interface::MoveInProgress move;
  move.objName() = "task0";
  move.toContainer() = "host0";
  movesInProgressSpec.moves() = {move};
  solver->addConstraint(movesInProgressSpec);

  // Each host may have at most 2 tasks.
  facebook::rebalancer::interface::CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "task_count";
  capacitySpec.definition() =
      facebook::rebalancer::interface::CapacitySpecDefinition::DURING;

  facebook::rebalancer::interface::Limit limit;
  limit.type() = facebook::rebalancer::interface::LimitType::ABSOLUTE;
  limit.globalLimit() = 2;
  capacitySpec.limit() = limit;

  solver->addConstraint(capacitySpec);

  // Add an incentive for task0 to move to host1, and task1 to move to
  // host0.
  facebook::rebalancer::interface::AssignmentAffinitiesSpec
      assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = {
      ::facebook::rebalancer::interface::makeAssignmentAffinity(
          "task0", "host1", 1.0),
      ::facebook::rebalancer::interface::makeAssignmentAffinity(
          "task1", "host0", 1.0),
  };

  solver->addGoal(assignmentAffinitiesSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // Both tasks have an incentive to move, but only task1 moves because task0
  // has a MovesInProgress constraint overriding its final assignment.
  EXPECT_EQ("host0", assignment["task0"]);
  EXPECT_EQ("host0", assignment["task1"]);
}
