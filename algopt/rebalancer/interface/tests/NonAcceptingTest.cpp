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

class NonAcceptingTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, NonAcceptingTest, testThreadCounts());

TEST_P(NonAcceptingTest, MovePrevented) {
  // In this example there are 2 tasks that prefer to be in different hosts.
  // A NonAccepting constraint defined on one of the preferred hosts prevents
  // one of the tasks from moving to where it would be optimal.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // There are 3 hosts and 2 tasks, both in host0 initially.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {}},
      });

  // host1 does not accept any new objects.
  facebook::rebalancer::interface::NonAcceptingSpec nonAcceptingSpec;
  nonAcceptingSpec.scope() = "host";
  nonAcceptingSpec.items() = {"host1"};
  solver->addConstraint(nonAcceptingSpec);

  // task0 prefers host1 and task1 prefers host2.
  facebook::rebalancer::interface::AssignmentAffinitiesSpec
      assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = {
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task0", "host1", 1.0),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task1", "host2", 1.0),
  };

  solver->addGoal(assignmentAffinitiesSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // We expect task1 to move from host0 to host2, which it prefers. task0 can't
  // move to host1 because it would violate the NonAccepting constraint defined
  // on host1.
  EXPECT_EQ("host0", assignment["task0"]);
  EXPECT_EQ("host2", assignment["task1"]);
}

TEST_P(NonAcceptingTest, InterScopeMovePrevented) {
  // In this example there are 3 tasks that prefer to be in different hosts.
  // task0 prefers host1, task1 prefers host2, task2 prefers host 0
  // We define NoAccepting constraints on rack0. This ensures that rack0
  // does not accept new tasks from outside rack0 (BetweenScopeMove). However,
  // (within-scope-move) is not prevented
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // There are 3 hosts and 3 tasks
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {"task2"}},
      });

  solver->addScope(
      "rack",
      folly::F14FastMap<std::string, std::string>{
          {"host0", "rack0"},
          {"host1", "rack0"},
          {"host2", "rack1"},
      });

  // rack0 does not accept any new objects (from other racks).
  facebook::rebalancer::interface::NonAcceptingSpec nonAcceptingSpec;
  nonAcceptingSpec.scope() = "rack";
  nonAcceptingSpec.items() = {"rack0"};
  solver->addConstraint(nonAcceptingSpec);

  // task0 prefers host1, task1 prefers host2, task2 prefers host0
  facebook::rebalancer::interface::AssignmentAffinitiesSpec
      assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = {
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task0", "host1", 1.0),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task1", "host2", 1.0),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task2", "host0", 1.0),
  };

  solver->addGoal(assignmentAffinitiesSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // We expect task0 to move to its preferred host: host1 (within scope move),
  // task1 moves to its preferred host host2 (rack1 is still accepting)
  // However, task2 cannot move to its preffered host0 because it would violate
  // the NonAccepting constraint (between scope Move) defined on rack0.
  EXPECT_EQ("host1", assignment["task0"]);
  EXPECT_EQ("host2", assignment["task1"]);
  EXPECT_EQ("host2", assignment["task2"]);
}

TEST_P(NonAcceptingTest, MultipleSpecs) {
  // In this example there are 3 tasks that prefer to be in different racks than
  // their current assignment.
  // - tasks 0,1 prefer rack1 over rack0
  // - task2 prefers rack0
  // We define NoAccepting constraints on rack0, so task2 can't move.
  // We also define additional NoAccepting constraints on host2 so all tasks
  // have to move to host1 (which lies in rack1)
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // There are 3 hosts and 3 tasks
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {"task2"}},
      });

  solver->addScope(
      "rack",
      folly::F14FastMap<std::string, std::string>{
          {"host0", "rack0"},
          {"host1", "rack1"},
          {"host2", "rack1"},
      });

  // rack0 does not accept any new objects (from other racks).
  facebook::rebalancer::interface::NonAcceptingSpec nonAcceptingSpecRack;
  nonAcceptingSpecRack.name() = "non_accepting_rack";
  nonAcceptingSpecRack.scope() = "rack";
  nonAcceptingSpecRack.items() = {"rack0"};
  solver->addConstraint(nonAcceptingSpecRack);

  // host2 does not accept any new objects (from any other host).
  facebook::rebalancer::interface::NonAcceptingSpec nonAcceptingSpecHost;
  nonAcceptingSpecHost.name() = "non_accepting_host";
  nonAcceptingSpecHost.scope() = "host";
  nonAcceptingSpecHost.items() = {"host2"};
  solver->addConstraint(nonAcceptingSpecHost);

  // task0 prefers host1, task1 prefers host2, task2 prefers host0
  facebook::rebalancer::interface::AssignmentAffinitiesSpec
      assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "rack";
  assignmentAffinitiesSpec.affinities() = {
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task0", "rack1", 1.0),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task1", "rack1", 1.0),
      facebook::rebalancer::interface::makeAssignmentAffinity(
          "task2", "rack0", 1.0),
  };

  solver->addGoal(assignmentAffinitiesSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // We expect task0, task1 to move to its preferred rack: rack1
  // but they must move to host1 because host2 is non-accepting
  // task2 is unable to move because rack0 is non-accpeting
  EXPECT_EQ("host1", assignment["task0"]);
  EXPECT_EQ("host1", assignment["task1"]);
  EXPECT_EQ("host2", assignment["task2"]);
}
