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

#include <string>

using namespace facebook::rebalancer::interface;

class ExclusiveGroupsTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, ExclusiveGroupsTest, testThreadCounts());

TEST_P(ExclusiveGroupsTest, Simple) {
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  // In this example there are 2 hosts and 2 jobs and we require exclusive use
  // of hosts by jobs.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // host0 has 3 tasks of job0 and 3 tasks of job1.
  // host1 has 3 tasks of job0 and 1 task of job1.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2", "task6", "task7", "task8"}},
          {"host1", {"task3", "task4", "task5", "task9"}},
      });

  // job0 is composed of tasks 0-5.
  // job1 is composed of tasks 6-9.
  solver->addPartition(
      "job",
      std::map<std::string, std::string>{
          {"task0", "job0"},
          {"task1", "job0"},
          {"task2", "job0"},
          {"task3", "job0"},
          {"task4", "job0"},
          {"task5", "job0"},
          {"task6", "job1"},
          {"task7", "job1"},
          {"task8", "job1"},
          {"task9", "job1"}});

  // Tasks take 1 slot each.
  solver->addObjectDimension(
      "slots",
      std::vector<std::pair<std::string, double>>{
          {"task0", 1},
          {"task1", 1},
          {"task2", 1},
          {"task3", 1},
          {"task4", 1},
          {"task5", 1},
          {"task6", 1},
          {"task7", 1},
          {"task8", 1},
          {"task9", 1}});

  // host0 has 7 slots, host 1 has only 5 slots.
  solver->addContainerDimension(
      "slots",
      folly::F14FastMap<std::string, double>{{"host0", 7}, {"host1", 5}});

  // Add a capacity constraint for slots.
  {
    CapacitySpec spec;
    spec.scope() = "host";
    spec.dimension() = "slots";
    solver->addConstraint(spec);
  }

  // Add an exclusive groups constraint.
  {
    ExclusiveGroupsSpec spec;
    spec.scope() = "host";
    spec.dimension() = "slots";
    spec.partitionName() = "job";
    spec.name() = "abc";
    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // Because job0 only fits in host0, there's a single valid solution.
  EXPECT_EQ("host0", assignment.at("task0"));
  EXPECT_EQ("host0", assignment.at("task1"));
  EXPECT_EQ("host0", assignment.at("task2"));
  EXPECT_EQ("host0", assignment.at("task3"));
  EXPECT_EQ("host0", assignment.at("task4"));
  EXPECT_EQ("host0", assignment.at("task5"));
  EXPECT_EQ("host1", assignment.at("task6"));
  EXPECT_EQ("host1", assignment.at("task7"));
  EXPECT_EQ("host1", assignment.at("task8"));
  EXPECT_EQ("host1", assignment.at("task9"));

  // Verify broken constraint values in initial and final assignments.
  EXPECT_NEAR(1.0, *solution.initialConstraint()->brokenVal(), 1e-8);
  EXPECT_NEAR(0.0, *solution.finalConstraint()->brokenVal(), 1e-8);

  // Verify the spec's metadata contains the internal tagging of scope items
  // with groups.
  auto& metadata = solution.specNameToMetadata()->at("abc");
  EXPECT_EQ("abc", *metadata.specName());
  EXPECT_TRUE(
      metadata.exclusiveGroupsTagging()); // exclusiveGroupsTagging is set
  auto& scopeItemToGroup =
      *metadata.exclusiveGroupsTagging()->scopeItemToGroup();
  EXPECT_EQ(2, scopeItemToGroup.size());
  EXPECT_EQ("job0", scopeItemToGroup.at("host0"));
  EXPECT_EQ("job1", scopeItemToGroup.at("host1"));
}

TEST_P(ExclusiveGroupsTest, ThreeGroups) {
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  // In this example there are 3 jobs and 4 hosts. There's a unique optimal
  // tagging of hosts with jobs where the churn of moves is minimized.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task3"}},
          {"host1", {"task4", "task6"}},
          {"host2", {"task5", "task7", "task8"}},
          {"host3", {"task2"}},
      });

  // Each job contains 3 tasks in consecutive order.
  solver->addPartition(
      "job",
      folly::F14FastMap<std::string, std::string>{
          {"task0", "job0"},
          {"task1", "job0"},
          {"task2", "job0"},
          {"task3", "job1"},
          {"task4", "job1"},
          {"task5", "job1"},
          {"task6", "job2"},
          {"task7", "job2"},
          {"task8", "job2"}});

  // Add an exclusive groups constraint.
  {
    ExclusiveGroupsSpec spec;
    spec.scope() = "host";
    spec.dimension() = "task_count";
    spec.partitionName() = "job";
    spec.name() = "abc";
    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // In an optimal tagging, both host0 and host3 get assigned job0, and these
  // tasks don't have an incentive to move.
  EXPECT_EQ("host0", assignment.at("task0"));
  EXPECT_EQ("host0", assignment.at("task1"));
  EXPECT_EQ("host3", assignment.at("task2"));

  // In an optimal assignment, only host1 is assigned job1.
  EXPECT_EQ("host1", assignment.at("task3"));
  EXPECT_EQ("host1", assignment.at("task4"));
  EXPECT_EQ("host1", assignment.at("task5"));

  // In an optimal assignment, only host2 is assigned job2.
  EXPECT_EQ("host2", assignment.at("task6"));
  EXPECT_EQ("host2", assignment.at("task7"));
  EXPECT_EQ("host2", assignment.at("task8"));

  // Verify broken constraint values in initial and final assignments.
  EXPECT_NEAR(3.0, *solution.initialConstraint()->brokenVal(), 1e-8);
  EXPECT_NEAR(0.0, *solution.finalConstraint()->brokenVal(), 1e-8);

  // Verify the spec's metadata contains the internal tagging of scope items
  // with groups.
  auto& metadata = solution.specNameToMetadata()->at("abc");
  EXPECT_EQ("abc", *metadata.specName());
  EXPECT_TRUE(
      metadata.exclusiveGroupsTagging()); // exclusiveGroupsTagging is set
  auto& scopeItemToGroup =
      *metadata.exclusiveGroupsTagging()->scopeItemToGroup();
  EXPECT_EQ(4, scopeItemToGroup.size());
  EXPECT_EQ("job0", scopeItemToGroup.at("host0"));
  EXPECT_EQ("job1", scopeItemToGroup.at("host1"));
  EXPECT_EQ("job2", scopeItemToGroup.at("host2"));
  EXPECT_EQ("job0", scopeItemToGroup.at("host3"));
}

TEST_P(ExclusiveGroupsTest, Goal) {
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  // This example creates a scenario where the exclusive groups constraint is
  // initially broken but other constraints prevent fixing it. We expect the
  // solver to perform zero moves, as there's nothing that can be improved. This
  // scenario used to behave unexpectedly due to a bug affecting cases where
  // scope items have different sizes and the exlusive groups spec is used as a
  // goal.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task3"}},
          {"host1", {"task1"}},
          {"host2", {"task2"}},
      });

  solver->addPartition(
      "job",
      std::vector<std::pair<std::string, std::string>>{
          {"task0", "job0"},
          {"task1", "job1"},
          {"task2", "job2"},
          {"task3", "job1"}});

  // Tasks take 1 slot each.
  solver->addObjectDimension(
      "slots",
      std::unordered_map<std::string, double>{
          {"task0", 1}, {"task1", 1}, {"task2", 1}, {"task3", 1}});

  // host0 and host1 have 2 slots each, while host2 has 3 slots
  solver->addContainerDimension(
      "slots",
      folly::F14FastMap<std::string, double>{
          {"host0", 2}, {"host1", 2}, {"host2", 3}});

  // Add an exclusive groups goal.
  {
    ExclusiveGroupsSpec spec;
    spec.scope() = "host";
    spec.dimension() = "slots";
    spec.partitionName() = "job";
    spec.name() = "abc";
    solver->addGoal(spec);
  }

  // Prevent task3 from being placed in host1.
  {
    AvoidAssignmentsSpec spec;
    spec.scope() = "host";
    AvoidAssignment assignment;
    assignment.object() = "task3";
    assignment.scopeItems() = {"host1"};
    spec.assignments() = {assignment};
    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // Verify goal values in initial and final assignments.
  EXPECT_NEAR(0.42857142857142855, *solution.initialObjective()->value(), 1e-8);
  EXPECT_NEAR(0.42857142857142855, *solution.finalObjective()->value(), 1e-8);

  // Verify the internal tagging of hosts with jobs.
  auto& metadata = solution.specNameToMetadata()->at("abc");
  auto& scopeItemToGroup =
      *metadata.exclusiveGroupsTagging()->scopeItemToGroup();
  EXPECT_EQ(3, scopeItemToGroup.size());
  EXPECT_EQ("job0", scopeItemToGroup.at("host0"));
  EXPECT_EQ("job1", scopeItemToGroup.at("host1"));
  EXPECT_EQ("job2", scopeItemToGroup.at("host2"));

  // All tasks are expected to stay in their initial host.
  EXPECT_EQ("host0", assignment.at("task0"));
  EXPECT_EQ("host1", assignment.at("task1"));
  EXPECT_EQ("host2", assignment.at("task2"));
  EXPECT_EQ("host0", assignment.at("task3"));
}
