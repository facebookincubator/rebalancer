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
#include <algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSolver_types.h>

#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

class GroupAssignmentAffinitiesTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    GroupAssignmentAffinitiesTest,
    testThreadCounts());

TEST_P(GroupAssignmentAffinitiesTest, Simple) {
  // In this example there are 3 hosts and 1 job composed of 4 tasks. We
  // add group assignment affinities to fully define the final layout.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // There are 3 hosts and 4 tasks initially in host0.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {{"task0", "task1", "task2", "task3"}}},
          {"host1", {}},
          {"host2", {}},
      });

  solver->addPartition(
      "job",
      std::unordered_map<std::string, std::vector<std::string>>{
          {"job0", {"task0", "task1", "task2", "task3"}}});

  GroupAssignmentAffinitiesSpec spec;
  spec.scope() = "host";
  spec.partition() = "job";
  spec.dimension() = "task_count";

  {
    // 1 task wants to be in host0
    GroupScopeItemAffinity affinity;
    affinity.group() = "job0";
    affinity.scopeItem() = "host0";
    affinity.targetDimensionValue() = 1.0;
    affinity.affinity() = 1.0;
    spec.affinities()->push_back(affinity);
  }

  {
    // 2 tasks want to be in host1
    GroupScopeItemAffinity affinity;
    affinity.group() = "job0";
    affinity.scopeItem() = "host1";
    affinity.targetDimensionValue() = 2.0;
    affinity.affinity() = 1.0;
    spec.affinities()->push_back(affinity);
  }

  {
    // 1 task wants to be in host2
    GroupScopeItemAffinity affinity;
    affinity.group() = "job0";
    affinity.scopeItem() = "host2";
    affinity.targetDimensionValue() = 1.0;
    affinity.affinity() = 1.0;
    spec.affinities()->push_back(affinity);
  }

  solver->addGoal(spec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  std::map<std::string, int> hostToTaskCount;
  for (auto& [_, host] : assignment) {
    ++hostToTaskCount[host];
  }

  EXPECT_EQ(3, hostToTaskCount.size());
  EXPECT_EQ(1, hostToTaskCount.at("host0"));
  EXPECT_EQ(2, hostToTaskCount.at("host1"));
  EXPECT_EQ(1, hostToTaskCount.at("host2"));
}

TEST_P(GroupAssignmentAffinitiesTest, CompetingObjects) {
  // In this example there are 3 hosts and 2 jobs composed of 4 tasks each.
  // Both jobs compete for host1, which has limited resources. The job with
  // larger affinity takes precedence and uses up host1 before the other job.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // There are 3 hosts and 8 tasks initially in host0.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0",
           {"task0",
            "task1",
            "task2",
            "task3",
            "task4",
            "task5",
            "task6",
            "task7"}},
          {"host1", {}},
          {"host2", {}},
      });

  std::map<std::string, std::string> taskToJob = {
      {"task0", "job0"},
      {"task1", "job0"},
      {"task2", "job0"},
      {"task3", "job0"},
      {"task4", "job1"},
      {"task5", "job1"},
      {"task6", "job1"},
      {"task7", "job1"}};

  solver->addPartition("job", taskToJob);

  {
    // Group assignment affinities goal.
    GroupAssignmentAffinitiesSpec spec;
    spec.scope() = "host";
    spec.partition() = "job";
    spec.dimension() = "task_count";

    {
      // job0 wants 1 task in host0
      GroupScopeItemAffinity affinity;
      affinity.group() = "job0";
      affinity.scopeItem() = "host0";
      affinity.targetDimensionValue() = 1.0;
      affinity.affinity() = 2.0;
      spec.affinities()->push_back(affinity);
    }

    {
      // job0 wants 3 tasks in host1
      GroupScopeItemAffinity affinity;
      affinity.group() = "job0";
      affinity.scopeItem() = "host1";
      affinity.targetDimensionValue() = 3.0;
      affinity.affinity() = 2.0;
      spec.affinities()->push_back(affinity);
    }

    {
      // job1 wants 2 tasks in host1
      GroupScopeItemAffinity affinity;
      affinity.group() = "job1";
      affinity.scopeItem() = "host1";
      affinity.targetDimensionValue() = 2.0;
      affinity.affinity() = 1.0;
      spec.affinities()->push_back(affinity);
    }

    {
      // job1 wants 2 tasks in host2
      GroupScopeItemAffinity affinity;
      affinity.group() = "job1";
      affinity.scopeItem() = "host2";
      affinity.targetDimensionValue() = 2.0;
      affinity.affinity() = 1.0;
      spec.affinities()->push_back(affinity);
    }

    solver->addGoal(spec);
  }

  {
    // Capacity constraint.
    CapacitySpec spec;
    spec.scope() = "host";
    spec.dimension() = "task_count";
    spec.filter()->itemsWhitelist() = {"host1"};
    spec.limit()->globalLimit() = 4;

    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  std::map<std::string, std::map<std::string, int>> hostToJobToTaskCount;
  for (auto& [task, host] : assignment) {
    ++hostToJobToTaskCount[host][taskToJob.at(task)];
  }

  // Only 1 full job fits in host1. Since job0 has a higher affinity than
  // job1, job0 gets all 3 desired tasks placed in host1, while job1 only
  // gets 1.
  EXPECT_EQ(1, hostToJobToTaskCount.at("host0").at("job0"));
  EXPECT_EQ(3, hostToJobToTaskCount.at("host1").at("job0"));
  EXPECT_EQ(1, hostToJobToTaskCount.at("host1").at("job1"));
  EXPECT_EQ(2, hostToJobToTaskCount.at("host2").at("job1"));
}
