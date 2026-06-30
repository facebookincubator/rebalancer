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

class MinimizeMovementTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(ThreadCounts, MinimizeMovementTest, testThreadCounts());

TEST_P(MinimizeMovementTest, SameCostPerObjectWithoutNormalization) {
  // In this example there are 3 hosts and 2 tasks. Both tasks start in host0
  // and both have an affinity for host1. Only tasks with an affinity greater
  // than the movement cost can move.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // host0 initially has 2 tasks in it, and the other hosts are empty.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {}},
      });

  // Both tasks prefer host1: task0 has an affinity of 3, task1 has an affinity
  // of 10.
  facebook::rebalancer::interface::AssignmentAffinitiesSpec
      assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.name() = "affinities";
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = {
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task0", "host1", 3.0),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task1", "host1", 10.0),
  };

  solver->addGoal(assignmentAffinitiesSpec);

  // The cost of moving a task is 4.
  facebook::rebalancer::interface::MinimizeMovementSpec minimizeMovementSpec;
  minimizeMovementSpec.name() = "minimize_movement";
  minimizeMovementSpec.scope() = "host";
  minimizeMovementSpec.dimension() =
      "task_count"; // Defined as 1 for each object.
  minimizeMovementSpec.magicScaling() =
      false; // The value of magicScaling is ignored when doNotNormalize is
             // true.
  minimizeMovementSpec.doNotNormalize() = true;

  solver->addGoal(minimizeMovementSpec, 4);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // We expect task1 to move from host0 to host1 because the affinity gain (10)
  // is greater than the movement cost (4), while task0 should not move because
  // the affinity gain (3) is less than the movement cost (4).
  EXPECT_EQ("host0", assignment["task0"]);
  EXPECT_EQ("host1", assignment["task1"]);

  const auto& initialGoal =
      *solution.initialGlobalObjective()->goals()->at(0).objs();
  const auto& finalGoal =
      *solution.finalGlobalObjective()->goals()->at(0).objs();

  // Expect the affinity gain to be 10.
  const double initialAffinities = getObjectiveValue(initialGoal, "affinities");
  const double finalAffinities = getObjectiveValue(finalGoal, "affinities");
  const double affinitiesGain = initialAffinities - finalAffinities;
  EXPECT_NEAR(10.0, affinitiesGain, 1e-8);

  // Expect the movement cost to be 4.
  const double initialMovement =
      getObjectiveValue(initialGoal, "minimize_movement");
  const double finalMovement =
      getObjectiveValue(finalGoal, "minimize_movement");
  const double movementCost = finalMovement - initialMovement;
  EXPECT_NEAR(4.0, movementCost, 1e-8);
}

TEST_P(MinimizeMovementTest, SameCostPerObjectWithNormalization) {
  // In this example there are 3 hosts and 2 tasks. Both tasks start in host0
  // and both have an affinity for host1. Only tasks with an affinity greater
  // than the movement cost can move.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // host0 initially has 2 tasks in it, and the other hosts are empty.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {}},
      });

  // Both tasks prefer host1: task0 has an affinity of 10, task1 has an affinity
  // of 3.
  facebook::rebalancer::interface::AssignmentAffinitiesSpec
      assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.name() = "affinities";
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = {
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task0", "host1", 10.0),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task1", "host1", 3.0),
  };

  solver->addGoal(assignmentAffinitiesSpec);

  // The cost of moving an object is defined as:
  // - When magicScaling=false: {weight} / {number of scope items}
  // - When magicScaling=true: 0.001 * {weight} / {number of scope items}
  // In this example, since magicScaling=false, the weight is 4, and there are 3
  // hosts, the effective cost of moving an object is 4/3.
  facebook::rebalancer::interface::MinimizeMovementSpec minimizeMovementSpec;
  minimizeMovementSpec.name() = "minimize_movement";
  minimizeMovementSpec.scope() = "host";
  minimizeMovementSpec.dimension() =
      "task_count"; // Defined as 1 for each object.
  minimizeMovementSpec.magicScaling() = false;
  minimizeMovementSpec.doNotNormalize() = false;

  solver->addGoal(minimizeMovementSpec, 4);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // We expect both tasks to movehost0 to host1 because their affinity gains
  // (10 and 3) are both greater than the movement cost (~2.66).
  EXPECT_EQ("host1", assignment["task0"]);
  EXPECT_EQ("host1", assignment["task1"]);

  const auto& initialGoal =
      *solution.initialGlobalObjective()->goals()->at(0).objs();
  const auto& finalGoal =
      *solution.finalGlobalObjective()->goals()->at(0).objs();

  // Expect the affinity gain to be 13: 10 + 3.
  const double initialAffinities = getObjectiveValue(initialGoal, "affinities");
  const double finalAffinities = getObjectiveValue(finalGoal, "affinities");
  const double affinitiesGain = initialAffinities - finalAffinities;
  EXPECT_NEAR(13.0, affinitiesGain, 1e-8);

  // Expect the movement cost to be 8/3 (2 tasks move with a cost of 4/3 each).
  const double initialMovement =
      getObjectiveValue(initialGoal, "minimize_movement");
  const double finalMovement =
      getObjectiveValue(finalGoal, "minimize_movement");
  const double movementCost = finalMovement - initialMovement;
  EXPECT_NEAR(2.66666666, movementCost, 1e-8);
}

TEST_P(MinimizeMovementTest, DifferentCostPerObjectWithoutNormalization) {
  // In this example there are 3 hosts and 2 tasks. Both tasks start in host0
  // and both have an affinity for host1. Only tasks with an affinity greater
  // than the movement cost can move.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // host0 initially has 2 tasks in it, and the other hosts are empty.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {}},
      });

  // Both tasks prefer host1: task0 has an affinity of 10, task1 has an affinity
  // of 3.
  facebook::rebalancer::interface::AssignmentAffinitiesSpec
      assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.name() = "affinities";
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = {
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task0", "host1", 10.0),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task1", "host1", 3.0),
  };

  solver->addGoal(assignmentAffinitiesSpec);

  // The cost of moving each object is defined as a dimension.
  solver->addObjectDimension(
      "size_gb",
      folly::F14FastMap<std::string, double>{{"task0", 7}, {"task1", 2}});

  // The cost of moving a task is size_gb.
  facebook::rebalancer::interface::MinimizeMovementSpec minimizeMovementSpec;
  minimizeMovementSpec.name() = "minimize_movement";
  minimizeMovementSpec.scope() = "host";
  minimizeMovementSpec.dimension() =
      "size_gb"; // Defined as 7 for task0 and 2 for task1.
  minimizeMovementSpec.magicScaling() =
      false; // The value of magicScaling is ignored when doNotNormalize is
             // true.
  minimizeMovementSpec.doNotNormalize() = true;

  solver->addGoal(minimizeMovementSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // We expect both tasks to move from host0 to host1 because their respective
  // affinity gains (10 and 3) are greater than their respective movement costs
  // (7 and 2).
  EXPECT_EQ("host1", assignment["task0"]);
  EXPECT_EQ("host1", assignment["task1"]);

  const auto& initialGoal =
      *solution.initialGlobalObjective()->goals()->at(0).objs();
  const auto& finalGoal =
      *solution.finalGlobalObjective()->goals()->at(0).objs();

  // Expect the affinity gain to be 13: 10 + 3.
  const double initialAffinities = getObjectiveValue(initialGoal, "affinities");
  const double finalAffinities = getObjectiveValue(finalGoal, "affinities");
  const double affinitiesGain = initialAffinities - finalAffinities;
  EXPECT_NEAR(13.0, affinitiesGain, 1e-8);

  // Expect the movement cost to be 9: 7 + 2.
  const double initialMovement =
      getObjectiveValue(initialGoal, "minimize_movement");
  const double finalMovement =
      getObjectiveValue(finalGoal, "minimize_movement");
  const double movementCost = finalMovement - initialMovement;
  EXPECT_NEAR(9.0, movementCost, 1e-8);
}

TEST_P(MinimizeMovementTest, DifferentCostPerObjectWithNormalization) {
  // In this example there are 3 hosts and 2 tasks. Both tasks start in host0.
  // There's a constraint forcing host0 to become empty. Each task moves to the
  // host with the lowest movement cost.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // host0 initially has 2 tasks in it, and the other hosts are empty.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {}},
      });

  // Force tasks to move out of host0.
  facebook::rebalancer::interface::ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "test";
  toFreeSpec.containers() = {"host0"};
  solver->addConstraint(toFreeSpec);

  // The cost of moving each object is defined as a dimension.
  solver->addObjectDimension(
      "size_gb",
      folly::F14FastMap<std::string, double>{{"task0", 7}, {"task1", 2}});

  // If defined, the cost of moving an object is normalized by dividing it by
  // the destination scope item's capacity.
  solver->addContainerDimension(
      "size_gb",
      std::vector<std::pair<std::string, double>>{
          {"host0", 30}, {"host1", 15}, {"host2", 20}});

  // The cost of moving a task is determined by size_gb of the task moving,
  // as well as the size_gb of the destination host, given by the formula:
  // 0.001 * {weight} * {task size_gb} / {destination's size_gb} / {#hosts}
  // For example, the cost of moving task0 from host0 to host1 would be:
  // 0.001 * 1 * 7 / 15 / 3 = ~0.00015555
  facebook::rebalancer::interface::MinimizeMovementSpec minimizeMovementSpec;
  minimizeMovementSpec.name() = "minimize_movement";
  minimizeMovementSpec.scope() = "host";
  minimizeMovementSpec.dimension() =
      "size_gb"; // Defined as 7 for task0 and 2 for task1.
  minimizeMovementSpec.magicScaling() =
      true; // When magicScaling=true, the cost gets internally multiplied by
            // 0.001.
  minimizeMovementSpec.doNotNormalize() = false;
  solver->addGoal(minimizeMovementSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // We expect both tasks to move out of host0, because the ToFree constraint
  // forces them to. They will move to host2 rather than host1 because the cost
  // is smaller, as host2's capacity (20) is greater than host1's (15).
  EXPECT_EQ("host2", assignment["task0"]);
  EXPECT_EQ("host2", assignment["task1"]);

  const auto& initialGoal =
      *solution.initialGlobalObjective()->goals()->at(0).objs();
  const auto& finalGoal =
      *solution.finalGlobalObjective()->goals()->at(0).objs();

  const double initialMovement =
      getObjectiveValue(initialGoal, "minimize_movement");
  EXPECT_NEAR(0.0, initialMovement, 1e-8);

  // Expect the movement cost to be: 0.001 * (7 / 20 + 2 / 20) / 3.
  const double finalMovement =
      getObjectiveValue(finalGoal, "minimize_movement");
  EXPECT_NEAR(0.00015, finalMovement, 1e-8);
}

TEST_P(MinimizeMovementTest, AllowanceConstraint) {
  // In this example there are 2 hosts and 4 tasks. All tasks start in host0 but
  // they all prefer host1. A minimize movement constraint limits the allowed
  // moves to 3, so all but 1 task move from host0 to host1.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // host0 initially has 4 tasks in it, and host1 is empty.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2", "task3"}},
          {"host1", {}},
      });

  // All tasks prefer being placed in host1.
  facebook::rebalancer::interface::AssignmentAffinitiesSpec
      assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = {
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task0", "host1", 1),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task1", "host1", 1),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task2", "host1", 1),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task3", "host1", 1),
  };

  solver->addGoal(assignmentAffinitiesSpec);

  // Impose an allowance of 3 objects that can move between hosts.
  auto minimizeMovementSpec = MinimizeMovementSpec();
  minimizeMovementSpec.scope() = "host";
  // Implicitly defined as 1 for each object.
  minimizeMovementSpec.dimension() = "task_count";
  minimizeMovementSpec.magicScaling() = false;
  minimizeMovementSpec.doNotNormalize() = true;
  minimizeMovementSpec.allowance() = 3;

  solver->addConstraint(minimizeMovementSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // We expect 3 tasks to move from host0 to host1, and 1 task to stay in host0.
  std::map<std::string, int> tasksInHost;
  for (const auto& [_, host] : assignment) {
    ++tasksInHost[host];
  }
  EXPECT_EQ(1, tasksInHost["host0"]);
  EXPECT_EQ(3, tasksInHost["host1"]);
}

TEST_P(MinimizeMovementTest, CrossScope) {
  // This example shows the behavior of moves from/to the outside of the scope.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // Three hosts, one task in each.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0"}},
          {"host1", {"task1"}},
          {"host2", {"task2"}},
      });

  // Define the rack scope, with host2 being an orphan container which doesn't
  // belong to any scope items.
  solver->addScope(
      "rack",
      folly::F14FastMap<std::string, std::string>{
          {"host0", "rack0"},
          {"host1", "rack0"},
      });

  // Each task prefers the next host.
  {
    AssignmentAffinitiesSpec spec;
    spec.scope() = "host";
    spec.affinities() = {
        makeAssignmentAffinity("task0", "host1", 1),
        makeAssignmentAffinity("task1", "host2", 1),
        makeAssignmentAffinity("task2", "host0", 1),
        makeAssignmentAffinity("task1", "host0", -1),
    };
    solver->addGoal(spec);
  }

  // A minimize movement constraint prevents moves across racks.
  {
    MinimizeMovementSpec spec;
    spec.scope() = "rack";
    spec.dimension() = "task_count";
    spec.allowance() = 0;
    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  // Only task0 is expected to move to host1. All other moves would cross a
  // rack and break the constraint.
  EXPECT_EQ("host1", assignment.at("task0"));
  EXPECT_EQ("host1", assignment.at("task1"));
  EXPECT_EQ("host2", assignment.at("task2"));
}
