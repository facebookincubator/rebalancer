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
#include <set>
#include <string>

using namespace facebook::rebalancer::interface;

class GroupIsolationLimitSpecTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    GroupIsolationLimitSpecTest,
    testThreadCounts());

TEST_P(
    GroupIsolationLimitSpecTest,
    InitiallyBrokenGroupIsolationLimitConstraint) {
  // In this example there are 2 hosts and 2 jobs. job0 contains 2 tasks. Job1
  // contains 3 tasks. Initially, task0 and task2 are assigned to host0. task1,
  // task3 and task4 are assigned to host1. Rebalancer should detect host0 and
  // host1 initially break the constraint that at most one group may coexist in
  // the same host. In this case, switching task2 with task1 completely fixes
  // the constraint violation.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task2"}},
          {"host1", {"task1", "task3", "task4"}},
      });

  // The first 2 tasks belong to job0, the remaining 3 tasks belong to job1.
  const std::map<std::string, std::string> taskToJob = {
      {"task0", "job0"},
      {"task1", "job0"},
      {"task2", "job1"},
      {"task3", "job1"},
      {"task4", "job1"},
  };

  // Let Rebalancer know of the job partition, so we can later refer to it.
  solver->addPartition("job", taskToJob);

  // Enforce group isolation limit on hosts. Note that the default limit for
  // this spec uses LimitType::ABSOLUTE and sets globalLimit = 0
  GroupIsolationLimitSpec groupIsolationLimitSpec;
  groupIsolationLimitSpec.scope() = "host";
  groupIsolationLimitSpec.partitionName() = "job";

  solver->addConstraint(groupIsolationLimitSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    const auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  EXPECT_TRUE(
      (hostToJobCount["host0"]["job0"] == 2 &&
       hostToJobCount["host0"]["job1"] == 0) ||
      (hostToJobCount["host0"]["job0"] == 0 &&
       hostToJobCount["host0"]["job1"] == 3));

  EXPECT_TRUE(
      (hostToJobCount["host1"]["job0"] == 2 &&
       hostToJobCount["host1"]["job1"] == 0) ||
      (hostToJobCount["host1"]["job0"] == 0 &&
       hostToJobCount["host1"]["job1"] == 3));
}

// All groups have a limit of 0, groupsAllowed is 2 (or any number greater than
// 1). This shows the more general way of saying "at most N groups may coexist
// in the same scope item".
TEST_P(GroupIsolationLimitSpecTest, GlobalGroupsAllowedLimitConstraint) {
  // In this example there are 2 hosts and 3 jobs.
  // Initially, job0/task0, job1/task2 and job2/task5 are assigned to host0.
  // Rebalancer should detect host0 and host1 initially break the constraint
  // that more than 2 groups coexist in the same scope. In this case, switching
  // just one task from host0 to host 1 completely fixes the constraint
  // violation.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task2", "task5"}},
          {"host1", {"task1", "task3", "task4", "task6", "task7"}},
      });

  const std::map<std::string, std::string> taskToJob = {
      {"task0", "job0"},
      {"task1", "job0"},
      {"task2", "job1"},
      {"task3", "job1"},
      {"task4", "job1"},
      {"task5", "job2"},
      {"task6", "job2"},
      {"task7", "job2"},
  };

  // Let Rebalancer know of the job partition, so we can later refer to it.
  solver->addPartition("job", taskToJob);

  GroupIsolationLimitSpec groupIsolationLimitSpec;
  groupIsolationLimitSpec.scope() = "host";
  groupIsolationLimitSpec.partitionName() = "job";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 0;
  groupIsolationLimitSpec.limit() = limit;

  groupIsolationLimitSpec.groupsAllowed() = 2;

  solver->addConstraint(groupIsolationLimitSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::set<std::string>> hostToJobSet;
  for (auto& [task, host] : *solution.assignment()) {
    const auto& job = taskToJob.at(task);
    hostToJobSet[host].insert(job);
  }

  for (auto& [host, jobSet] : hostToJobSet) {
    EXPECT_LE(jobSet.size(), 2);
  }
}

TEST_P(GroupIsolationLimitSpecTest, PerGroupIsolationLimit) {
  // In this example there are 2 hosts and 3 jobs.
  // Initially, job0/task0, job1/task2 and job2/task5 are assigned to host0.
  // Initially, both host0 and host1 break the constraint that only one group is
  // allowed in each scope. Note that job1 does not count if not more than 2
  // tasks are in a scope. Rebalancer should switch tasks with a job1's task
  // from host1 to fix the constraint violation.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  // host0 has task0 and task5. host1 has the rest of tasks.
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task5"}},
          {"host1", {"task1", "task2", "task3", "task4", "task6", "task7"}},
      });

  // The first 2 tasks belong to job0, the next 3 tasks belong to job1, and the
  // rest of tasks belong to job2.
  const std::map<std::string, std::string> taskToJob = {
      {"task0", "job0"},
      {"task1", "job0"},
      {"task2", "job1"},
      {"task3", "job1"},
      {"task4", "job1"},
      {"task5", "job2"},
      {"task6", "job2"},
      {"task7", "job2"},
  };

  // Let Rebalancer know of the job partition, so we can later refer to it.
  solver->addPartition("job", taskToJob);

  GroupIsolationLimitSpec groupIsolationLimitSpec;
  groupIsolationLimitSpec.scope() = "host";
  groupIsolationLimitSpec.partitionName() = "job";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 0;
  limit.groupLimits() = {{"job1", 2}};
  groupIsolationLimitSpec.limit() = limit;

  groupIsolationLimitSpec.groupsAllowed() = 1;

  solver->addConstraint(groupIsolationLimitSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    const auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  EXPECT_LT(hostToJobCount["host0"]["job1"], 3);
  EXPECT_LT(hostToJobCount["host1"]["job1"], 3);

  EXPECT_TRUE(
      hostToJobCount["host0"]["job0"] == 0 ||
      hostToJobCount["host1"]["job0"] == 0);

  EXPECT_TRUE(
      hostToJobCount["host0"]["job2"] == 0 ||
      hostToJobCount["host1"]["job2"] == 0);
}
