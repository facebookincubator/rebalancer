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

#include <folly/container/irange.h>
#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

static void verifyScopeItemGroupUtilMetrics(
    const std::string& dimensionName,
    const std::string& scopeName,
    const AssignmentSolution& solution,
    const std::vector<
        std::tuple<std::string, std::string, std::string, double>>&
        expectedInitialMetrics,
    const std::vector<
        std::tuple<std::string, std::string, std::string, double>>&
        expectedFinalMetrics,
    const std::string& utilMetric = "after") {
  auto& actualInitialUtilMetrics = solution.initialMetrics()
                                       ->utilMetricToScopeUtils()
                                       ->at(utilMetric)
                                       .scopeToDimensionUtils()
                                       ->at(scopeName);
  auto& actualFinalUtilMetrics = solution.finalMetrics()
                                     ->utilMetricToScopeUtils()
                                     ->at(utilMetric)
                                     .scopeToDimensionUtils()
                                     ->at(scopeName);

  auto assertEqual = [&](const auto& actualUtilMetrics,
                         const auto& expectedUtilMetrics) {
    for (auto& [scopeItem, partition, group, expectedValue] :
         expectedUtilMetrics) {
      auto& dimensionUtils =
          actualUtilMetrics.dimensionToScopeItemUtils()->at(dimensionName);
      EXPECT_TRUE(
          dimensionUtils.scopeItemToPartitionUtils()->contains(scopeItem))
          << "scopeItem " << scopeItem
          << " not found in scopeItemToPartitionUtils";

      auto& partitionUtils =
          dimensionUtils.scopeItemToPartitionUtils()->at(scopeItem);
      auto& groupUtils = partitionUtils.partitionToGroupUtils()->at(partition);
      auto actualValue = groupUtils.groupToValue()->at(group);
      EXPECT_NEAR(actualValue, expectedValue, 1e-8)
          << "Value mismatch for scopeItem=" << scopeItem
          << ", partition=" << partition << ", group=" << group
          << " expectedValue=" << expectedValue
          << " actualValue=" << actualValue;
    }
  };

  assertEqual(actualInitialUtilMetrics, expectedInitialMetrics);
  assertEqual(actualFinalUtilMetrics, expectedFinalMetrics);
}

// The first 5 tasks belong to job0, the remaining 4 tasks belong to job1.
const std::map<std::string, std::string> taskToJob = {
    {"task0", "job0"},
    {"task1", "job0"},
    {"task2", "job0"},
    {"task3", "job0"},
    {"task4", "job0"},
    {"task5", "job1"},
    {"task6", "job1"},
    {"task7", "job1"},
    {"task8", "job1"},
};

const std::map<std::string, std::vector<std::string>> rackToHost = {
    {"rack1", {"host0", "host1"}},
    {"rack2", {}},
};

class GroupCountTest : public ::testing::TestWithParam<std::tuple<int, bool>> {
 protected:
  void SetUp() override {
    solver = initializeTestProblemSolver(
        {.executorThreadCount = std::get<0>(GetParam())});
    solver->setObjectName("task");
    solver->setContainerName("host");

    // host0 has the first 5 tasks, host1 has the remaining 4 tasks.
    solver->setAssignment(
        std::vector<std::pair<std::string, std::vector<std::string>>>{
            {"host0", {"task0", "task1", "task2", "task3", "task4"}},
            {"host1", {"task5", "task6", "task7", "task8"}},
        });
    // Let Rebalancer know of the job partition, so we can later refer to it.
    solver->addPartition("job", taskToJob);
    solver->addScope("rack", rackToHost);

    // For illustration purposes, make local search's behavior predictable and
    // simple by allowing single moves only.
    LocalSearchSolverSpec localSearchSolverSpec;
    localSearchSolverSpec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
    solver->addSolver(localSearchSolverSpec);
  }

  std::string addDynamicTaskCount() {
    // when useDynamicDimension() is true, we add a dynamic object dimension
    // for task count, namely, 'dynamic_task_count'. This dimension is exactly
    // the same as 'task_count' dimension that is added by default, but just
    // that it is added as a dynamic dimension. This is simply used to test that
    // dynamicDimensions are supported in all code paths where usual static
    // dimensions are supported. (If it is not supported, we expect that some
    // test will fail since getVal(objectId) will throw when using a
    // dynamicDimension)
    auto dimName = "dynamic_task_count";
    solver->addDynamicObjectDimension(
        dimName,
        "host",
        folly::
            F14FastMap<std::string, folly::F14FastMap<std::string, double>>{},
        1);

    return dimName;
  }

  static bool useDynamicDimension() {
    return std::get<1>(GetParam());
  }

  std::unique_ptr<ProblemSolver> solver;
};

INSTANTIATE_TEST_CASE_P(
    GroupCountTestWithStaticDimension,
    GroupCountTest,
    ::testing::Combine(testThreadCounts(), ::testing::Values(false)));

INSTANTIATE_TEST_CASE_P(
    BasicGroupCountTestWithDynamicDimension,
    GroupCountTest,
    ::testing::Combine(testThreadCounts(), ::testing::Values(true)));

TEST_P(GroupCountTest, GlobalAbsoluteLimit) {
  // At most 3 tasks of the same group can be placed in the same scope item.
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 3;
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);
  solver->publishMetrics();

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // job0 initially had 5 tasks in host0, while the limit is 3: we expect
  // Rebalancer to have moved 2 tasks of job0 from host0 to host1.
  EXPECT_EQ(3, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job0"]);

  // On the other hand, job1 initially had 4 tasks in host1, while the limit
  // for it is also 3: we expect Rebalancer to have moved 1 task of job1 from
  // host1 to host0.
  EXPECT_EQ(1, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(3, hostToJobCount["host1"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "host",
      solution,
      {{"host1", "job", "job1", 4}, {"host0", "job", "job0", 5}},
      {{"host1", "job", "job1", 3},
       {"host1", "job", "job0", 2},
       {"host0", "job", "job1", 1},
       {"host0", "job", "job0", 3}});
}

TEST_P(GroupCountTest, GlobalRelativeLimit) {
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  // At most 70% of tasks of the same group can be placed in the same scope
  // item.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 0.7;
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);
  solver->publishMetrics();

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // job0 initially had 5 tasks in host0, while the limit is 70% of the job's
  // tasks. In other words no more than 3 of the 5 tasks of job0 can be placed
  // in the same host. We expect Rebalancer to have moved 2 tasks of job0 from
  // host0 to host1.
  EXPECT_EQ(3, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job0"]);

  // On the other hand, job1 initially had 4 tasks in host1, while the limit
  // is 70% of the job's tasks. In other words, no more than 2 of the 4 tasks
  // of job1 can be placed in the same host. We expect Rebalancer to have moved
  // 2 tasks of job1 from host1 to host0.
  EXPECT_EQ(2, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "host",
      solution,
      {{"host1", "job", "job1", 4}, {"host0", "job", "job0", 5}},
      {{"host1", "job", "job1", 2},
       {"host1", "job", "job0", 2},
       {"host0", "job", "job1", 2},
       {"host0", "job", "job0", 3}});
}

TEST_P(GroupCountTest, PerScopeItemAbsoluteLimit) {
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  // At most 3 tasks of the same job can be placed in host0. The remaining hosts
  // accept up to 5 tasks of the same job. Note that scope items that are not
  // explicitly listed in the limit spec fall back to using the global limit.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 5;
  limit.scopeItemLimits() = {{"host0", 3}};
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);
  solver->publishMetrics();

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // host0 contains 5 tasks of the same job, but the limit is 3. We expect
  // Rebalancer to move 2 tasks of job0 from host0 to host1.
  EXPECT_EQ(3, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job0"]);

  // On the other hand, host1 initially has only 4 tasks of the same job. Since
  // the limit for host1 is 5, there's no incentive to move any tasks from host1
  // to host0. We expect Rebalancer to not move any tasks of job1.
  EXPECT_EQ(0, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(4, hostToJobCount["host1"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "host",
      solution,
      {{"host1", "job", "job1", 4}, {"host0", "job", "job0", 5}},
      {{"host1", "job", "job1", 4},
       {"host1", "job", "job0", 2},
       {"host0", "job", "job0", 3}});
}

TEST_P(GroupCountTest, PerScopeItemRelativeLimit) {
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  // host1 doesn't accept more than 50% of tasks of the same job, while host0
  // doesn't have any restrictions. By making globalLimit=1 we are saying that
  // any hosts not explicitly listed may accept 100% of tasks of the same job.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 1.0;
  limit.scopeItemLimits() = {{"host1", 0.5}};
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);
  solver->publishMetrics();

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // host0 has virtually no limits on how many tasks of the same job it can
  // accept. We expect rebalancer to not move any of job0's tasks.
  EXPECT_EQ(5, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(0, hostToJobCount["host1"]["job0"]);

  // On the other hand, host1 can't accept more than 50% of a job's tasks. Since
  // all 4 tasks of job1 are initially placed in host1, we expect Rebalancer to
  // move half of them from host1 to host0.
  EXPECT_EQ(2, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "host",
      solution,
      {{"host1", "job", "job1", 4}, {"host0", "job", "job0", 5}},
      {{"host1", "job", "job1", 2},
       {"host0", "job", "job1", 2},
       {"host0", "job", "job0", 5}});
}

TEST_P(GroupCountTest, PerGroupAbsoluteLimit) {
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  // job1 may have up to 3 of its task placed in the same host. Any other job
  // may have up to 4 tasks placed in the same host.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 4;
  limit.groupLimits() = {{"job1", 3}};
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);

  // Get a solution from Rebalancer.
  solver->publishMetrics();
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // job0 has 5 tasks initially in host0. Since the limit for job0 is 4, we
  // expect Rebalancer to move 1 task from host0 to host1.
  EXPECT_EQ(4, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(1, hostToJobCount["host1"]["job0"]);

  // job1 has 4 tasks initially in host1. Since the limit for job1 is 3, we
  // expect Rebalancer to move 1 task from host1 to host0.
  EXPECT_EQ(1, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(3, hostToJobCount["host1"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "host",
      solution,
      {{"host1", "job", "job1", 4}, {"host0", "job", "job0", 5}},
      {{"host1", "job", "job1", 3},
       {"host1", "job", "job0", 1},
       {"host0", "job", "job1", 1},
       {"host0", "job", "job0", 4}});
}

TEST_P(GroupCountTest, PerGroupRelativeLimit) {
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  // job0 may have up to 60% of its tasks placed in the same host. Since job0
  // has 5 tasks, that's equivalent to saying at most 3 tasks can be in the
  // same host. On the other hand, job1 may have up to 50% of its tasks placed
  // in the same host, and since it has 4 tasks, that's equivalent to a limit of
  // 2 tasks per host. Other jobs, if there were any, would have virtually no
  // limits since we are setting the global limit to 1, which means that jobs
  // not listed explicitly may have up to 100% of its task on the same host.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 1.0;
  limit.groupLimits() = {{"job0", 0.6}, {"job1", 0.5}};
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);

  // Get a solution from Rebalancer.
  solver->publishMetrics();
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // We expect Rebalancer to move 2 tasks of job0 from host0 to host1.
  EXPECT_EQ(3, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job0"]);

  // We expect Rebalancer to move 2 tasks of job1 from host1 to host0.
  EXPECT_EQ(2, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "host",
      solution,
      {{"host1", "job", "job1", 4}, {"host0", "job", "job0", 5}},
      {{"host1", "job", "job1", 2},
       {"host1", "job", "job0", 2},
       {"host0", "job", "job1", 2},
       {"host0", "job", "job0", 3}});
}

TEST_P(GroupCountTest, PerScopeItemAndGroupAbsoluteLimit) {
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  // job0 may have up to 2 tasks placed in host0 and up to 4 tasks placed in
  // host1. job1 may have up to 1 task placed in host0. Any other combination
  // has a limit of 5.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 5;
  limit.scopeItemToGroupLimits() = {
      {"host0", {{"job0", 2}, {"job1", 1}}},
      {"host1", {{"job0", 4}}},
  };
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);
  solver->publishMetrics();

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // Because job0 can only have up to 2 tasks placed in host0, while it can
  // have up to 4 in host1, we expect Rebalancer to move 3 of its 5 tasks from
  // host0 to host1.
  EXPECT_EQ(2, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(3, hostToJobCount["host1"]["job0"]);

  // Because job1 can have up to 5 tasks in host1, there's no incentive to move
  // any of its tasks, and we expect Rebalancer to keep them all in host1.
  EXPECT_EQ(0, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(4, hostToJobCount["host1"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "host",
      solution,
      {{"host1", "job", "job1", 4}, {"host0", "job", "job0", 5}},
      {{"host1", "job", "job1", 4},
       {"host1", "job", "job0", 3},
       {"host0", "job", "job0", 2}});
}

TEST_P(GroupCountTest, PerScopeItemAndGroupAbsoluteLimitEmptyScope) {
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  // Test for empty scopes.
  // All tasks will be in rack 1. Rack 2 has no tasks because it has no hosts.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "rack";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;
  groupCountSpec.bound() = GroupCountSpecBound::MAX;
  groupCountSpec.zeroAllowed() = true;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 0;

  // Everything must be in rack 1
  limit.scopeItemToGroupLimits() = {
      {"rack1", {{"job0", 1000}, {"job1", 1000}}},
      {"rack2", {{"job0", 0}, {"job1", 0}}},
  };
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);

  // Get a solution from Rebalancer.
  solver->publishMetrics();
  auto solution = solver->solve();

  std::map<std::string, std::string> hostToRack;
  for (const auto& [rack, hosts] : rackToHost) {
    for (const auto& host : hosts) {
      hostToRack[host] = rack;
    }
  }

  // Count how many tasks from each job are in each rack
  std::map<std::string, std::map<std::string, int>> rackToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    const auto& rack = hostToRack.at(host);
    ++rackToJobCount[rack][job];
  }

  // Nothing in rack2 because no hosts are in rack2
  EXPECT_GE(rackToJobCount["rack1"]["job0"], 0);
  EXPECT_GE(rackToJobCount["rack1"]["job1"], 0);

  EXPECT_EQ(0, rackToJobCount["rack2"]["job0"]);
  EXPECT_EQ(0, rackToJobCount["rack2"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "rack",
      solution,
      {{"rack1", "job", "job1", 4}, {"rack1", "job", "job0", 5}},
      {{"rack1", "job", "job1", 4}, {"rack1", "job", "job0", 5}});
}

TEST_P(GroupCountTest, PerScopeItemAndGroupRelativeLimit) {
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  // Any job may have up to 80% of its tasks placed in the same host, except
  // job0 which may only place up to 50% of its tasks in host0.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 0.8;
  limit.scopeItemToGroupLimits() = {{"host0", {{"job0", 0.5}}}};
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);

  // Get a solution from Rebalancer.
  solver->publishMetrics();
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // Because job0 may only have up to 50% of its tasks placed in host0, we
  // expect Rebalancer to move 3 of its 5 tasks from host0 to host1.
  EXPECT_EQ(2, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(3, hostToJobCount["host1"]["job0"]);

  // Because job1 can only have up to 80% of its tasks in host1, we expect
  // Rebalancer to move 1 of its 4 tasks from host1 to host0.
  EXPECT_EQ(1, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(3, hostToJobCount["host1"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "host",
      solution,
      {{"host1", "job", "job1", 4}, {"host0", "job", "job0", 5}},
      {{"host1", "job", "job1", 3},
       {"host1", "job", "job0", 3},
       {"host0", "job", "job1", 1},
       {"host0", "job", "job0", 2}});
}

TEST_P(GroupCountTest, PerScopeItemAndGroupMinAbsoluteLimit) {
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  // There's a minimum requirement for job1 to have 3 tasks in host0.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;
  groupCountSpec.bound() = GroupCountSpecBound::MIN;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 0.0;
  limit.scopeItemToGroupLimits() = {{"host0", {{"job1", 3}}}};
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);

  // Get a solution from Rebalancer.
  solver->publishMetrics();
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // Because job0 is not affected by the constraint, the solver has no incentive
  // to move any of its tasks.
  EXPECT_EQ(5, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(0, hostToJobCount["host1"]["job0"]);

  // Exactly 3 tasks of job1 move from host1 to host0 to satisfy the constraint.
  EXPECT_EQ(3, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(1, hostToJobCount["host1"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "host",
      solution,
      {{"host1", "job", "job1", 4}, {"host0", "job", "job0", 5}},
      {{"host1", "job", "job1", 1},
       {"host0", "job", "job1", 3},
       {"host0", "job", "job0", 5}});
}

TEST_P(GroupCountTest, PerScopeItemAndGroupMinRelativeLimit) {
  auto dimension = useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  // There's a minimum requirement for job1 to have 50% of its tasks in host0.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = dimension;
  groupCountSpec.bound() = GroupCountSpecBound::MIN;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 0.0;
  limit.scopeItemToGroupLimits() = {{"host0", {{"job1", 0.5}}}};
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);

  // Get a solution from Rebalancer.
  solver->publishMetrics();
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // Because job0 is not affected by the constraint, the solver has no incentive
  // to move any of its tasks.
  EXPECT_EQ(5, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(0, hostToJobCount["host1"]["job0"]);

  // Exactly 2 tasks of job1 move from host1 to host0 to satisfy the constraint.
  EXPECT_EQ(2, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job1"]);

  verifyScopeItemGroupUtilMetrics(
      dimension,
      "host",
      solution,
      {{"host1", "job", "job1", 4}, {"host0", "job", "job0", 5}},
      {{"host1", "job", "job1", 2},
       {"host0", "job", "job1", 2},
       {"host0", "job", "job0", 5}});
}

TEST_P(GroupCountTest, MinBound) {
  // Each job must have at least 1 task placed in every host.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() =
      useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  groupCountSpec.bound() = GroupCountSpecBound::MIN;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1;
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // Because job0 doesn't have any tasks in host1, Rebalancer must move at
  // least 1 task of job0 from host0 to host1.
  EXPECT_EQ(4, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(1, hostToJobCount["host1"]["job0"]);

  // Because job1 doesn't have any tasks in host0, Rebalancer must move at
  // least 1 task of job1 from host1 to host0.
  EXPECT_EQ(1, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(3, hostToJobCount["host1"]["job1"]);
}

TEST_P(GroupCountTest, MultipleBound) {
  // Each job must have even number of tasks (0, 2, 4 ..) placed on host0.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() =
      useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  groupCountSpec.bound() = GroupCountSpecBound::MULTIPLE;
  groupCountSpec.filter()->itemsWhitelist() = {"host0"};

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 2;
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);

  // host0 can only contain even number of task of each job
  // so we must move one oof the five tasks of job0 to host1, and move all four
  // tasks of job1 to host0
  ToFreeSpec toFreeSpec;
  toFreeSpec.containers() = {"host1"};
  solver->addGoal(toFreeSpec);

  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
    XLOG(INFO) << task << " => " << host;
  }

  // Because job0 doesn't have any tasks in host1, Rebalancer must move at
  // least 1 task of job0 from host0 to host1.
  EXPECT_EQ(0, hostToJobCount["host0"]["job0"] % 2);
  EXPECT_EQ(0, hostToJobCount["host0"]["job1"] % 2);
  EXPECT_EQ(1, hostToJobCount["host1"]["job0"]);
  EXPECT_EQ(0, hostToJobCount["host1"]["job1"]);
}

TEST_P(GroupCountTest, SquaresObjective) {
  // By setting the upper limit to 0 tasks per host and enabling squares, we are
  // essentially minimizing the sum of squares of: the number of tasks of job X
  // in host Y, for every combination of job X and host Y. That's a reasonable
  // definition of a balance objective: minimizing it will spread out the tasks
  // of each job as evenly as possible among hosts.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() =
      useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  groupCountSpec.squares() = true;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 0;
  groupCountSpec.limit() = limit;

  solver->addGoal(groupCountSpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // job0 has an odd number of tasks, so they can't be perfectly balanced among
  // the 2 hosts. The best balance attainable, however, is obtained when
  // Rebalancer moves 2 tasks of job0 from host0 to host1.
  EXPECT_EQ(3, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job0"]);

  // Because job1 has even number of tasks, it can be balanced perfectly among
  // the 2 hosts. The best balance is obtained when Rebalance moves 2 tasks of
  // job1 from host1 to host0.
  EXPECT_EQ(2, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job1"]);
}

TEST_P(GroupCountTest, LimitRelativeToScopeItemUtil) {
  // By setting a limit relative to the scope item util, we add the constraint
  // that job0 may not account for more than 50% of the utilization of host0.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() =
      useDynamicDimension() ? addDynamicTaskCount() : "task_count";
  groupCountSpec.limitRelativeTo() =
      GroupCountSpecLimitRelativeTo::SCOPE_ITEM_UTIL;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 1;
  limit.scopeItemToGroupLimits() = {{"host0", {{"job0", 0.5}}}};
  groupCountSpec.limit() = limit;

  solver->addConstraint(groupCountSpec);

  // Prevent the trivial solution where host0 simply becomes empty by imposing
  // that hosts must contain at least one task.
  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "task_count";
  capacitySpec.bound() = CapacitySpecBound::MIN;

  Limit limit2;
  limit2.type() = LimitType::ABSOLUTE;
  limit2.globalLimit() = 1;
  capacitySpec.limit() = limit2;

  solver->addConstraint(capacitySpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  // In the solution Rebalancer generates, job0 accounts for exactly 50% of the
  // utilization of host0, which is valid. host0 has 2 tasks, one of job0, and
  // the other of job1. All other tasks are in host1.
  EXPECT_EQ(1, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(4, hostToJobCount["host1"]["job0"]);
  EXPECT_EQ(1, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(3, hostToJobCount["host1"]["job1"]);
}

TEST_P(GroupCountTest, LimitRelativeToScopeItemUtilWithCustomDimension) {
  // This unit test sets up GroupCountSpec as a goal and it asserts its value
  // for the initial assignment.
  solver = initializeTestProblemSolver();
  solver->setObjectName("task");
  solver->setContainerName("host");

  // There are 6 tasks evenly spread across the 2 hosts.
  solver->setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task2", "task4"}},
          {"host1", {"task1", "task3", "task5"}}});

  // Tasks are evenly partitioned into 3 jobs.
  solver->addPartition(
      "job",
      folly::F14FastMap<std::string, std::string>{
          {"task0", "job0"},
          {"task1", "job0"},
          {"task2", "job1"},
          {"task3", "job1"},
          {"task4", "job2"},
          {"task5", "job2"}});

  // Define a dimension on both objects and containers.
  solver->addObjectDimension(
      "weight",
      std::vector<std::pair<std::string, double>>{
          {"task0", 4.1},
          {"task1", 8},
          {"task2", 0.9},
          {"task3", 6},
          {"task4", 5},
          {"task5", 6}});
  solver->addContainerDimension(
      "weight",
      folly::F14FastMap<std::string, double>{{"host0", 20}, {"host1", 5}});

  // Add a GroupCountSpec goal with a limit of 40% relative to the scope item
  // utilization.
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = "weight";
  groupCountSpec.limitRelativeTo() =
      GroupCountSpecLimitRelativeTo::SCOPE_ITEM_UTIL;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 0.4;
  groupCountSpec.limit() = limit;

  solver->addGoal(groupCountSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Assert goal value for the initial assignment:
  // - host0
  //   - total relative utilization: (4.1 + 0.9 + 5) / 20 = 0.5
  //   - limit per group is 40% of the total relative utilization:
  //     40% * 0.5 = 0.2
  //   - job0
  //     - relative utilization: 4.1 / 20 = 0.205
  //     - limit exceeded by: 0.205 - 0.2 = 0.005
  //   - job1
  //     - relative utilization: 0.9 / 20 = 0.045
  //     - limit of 0.2 not exceeded
  //   - job2
  //     - relative utilization: 5 / 20 = 0.25
  //     - limit exceeded by: 0.25 - 0.2 = 0.05
  // - host1
  //   - total relative utilization: (8 + 6 + 6) / 5 = 4
  //   - limit per group is 40% of the total relative utilization:
  //     40% * 4 = 1.6
  //   - job0
  //     - relative utilization: 8 / 5 = 1.6
  //     - limit of 1.6 not exceeded
  //   - job1
  //     - relative utilization: 6 / 5 = 1.2
  //     - limit of 1.6 not exceeded
  //   - job2
  //     - relative utilization: 6 / 5 = 1.2
  //     - limit of 1.6 not exceeded
  // - sum of limit excess for all combinations of host and job:
  //   0.005 + 0.05 = 0.055
  const double goalValue =
      *solution.initialGlobalObjective()->goals()->at(0).value();
  EXPECT_NEAR(0.055, goalValue, 1e-8);
}

TEST_P(GroupCountTest, MinAbsoluteLimit) {
  // This test shows how scope item dimensions are irrelevant when using
  // absolute limits.
  solver->addObjectDimension(
      "weight",
      std::vector<std::pair<std::string, double>>{
          {"task0", 10.0},
          {"task1", 10.0},
          {"task2", 10.0},
          {"task3", 10.0},
          {"task4", 10.0},
          {"task5", 10.0},
          {"task6", 10.0},
          {"task7", 10.0},
          {"task8", 10.0}});

  // The container dimension values are irrelevant since we are defining
  // absolute limits in the spec.
  solver->addContainerDimension(
      "weight",
      folly::F14FastMap<std::string, double>{{"host0", 0.5}, {"host1", 500.0}});

  GroupCountSpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.dimension() = "weight";
  spec.bound() = GroupCountSpecBound::MIN;
  spec.limit()->type() = LimitType::ABSOLUTE;
  spec.limit()->globalLimit() = 20.0;
  spec.limit()->groupLimits()->emplace("job0", 30.0);
  solver->addGoal(spec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJob.at(task);
    ++hostToJobCount[host][job];
  }

  EXPECT_LE(2, hostToJobCount["host0"]["job0"]);
  EXPECT_LE(2, hostToJobCount["host1"]["job0"]);
  EXPECT_EQ(2, hostToJobCount["host0"]["job1"]);
  EXPECT_EQ(2, hostToJobCount["host1"]["job1"]);

  const double initialValue =
      *solution.initialGlobalObjective()->goals()->at(0).value();
  const double finalValue =
      *solution.finalGlobalObjective()->goals()->at(0).value();

  EXPECT_NEAR(50.0, initialValue, 1e-8);
  EXPECT_NEAR(10.0, finalValue, 1e-8);
}

TEST_P(GroupCountTest, ZeroAllowed) {
  // Example using zeroAllowed=true.
  solver = initializeTestProblemSolver();
  solver->setObjectName("task");
  solver->setContainerName("host");

  std::map<std::string, std::vector<std::string>> initialAssignment;
  for (const auto i : folly::irange(100)) {
    initialAssignment["host0"].push_back(fmt::format("task{}", i));
  }
  initialAssignment["host1"] = {};
  solver->setAssignment(initialAssignment);

  // for this test we will use a trivial taskToJob partition
  std::map<std::string, std::string> taskToJobTrivial;
  for (const auto i : folly::irange(100)) {
    taskToJobTrivial[fmt::format("task{}", i)] = "job0";
  }
  solver->addPartition("job", taskToJobTrivial);

  {
    // Either no tasks or at least 10% of tasks are placed in each host.
    GroupCountSpec spec;
    spec.scope() = "host";
    spec.partitionName() = "job";
    spec.dimension() =
        useDynamicDimension() ? addDynamicTaskCount() : "task_count";
    spec.limit()->type() = LimitType::RELATIVE;
    spec.limit()->globalLimit() = 0.1;
    spec.bound() = GroupCountSpecBound::MIN;
    spec.zeroAllowed() = true;
    solver->addConstraint(spec);
  }

  {
    // Place at least 1 task in each host.
    CapacitySpec spec;
    spec.scope() = "host";
    spec.dimension() = "task_count";
    spec.bound() = CapacitySpecBound::MIN;
    spec.limit()->globalLimit() = 1;
    solver->addConstraint(spec);
  }

  {
    // Make tasks prefer host0.
    AssignmentAffinitiesSpec spec;
    spec.scope() = "host";
    for (const auto i : folly::irange(100)) {
      AssignmentAffinity affinity;
      affinity.objectName() = fmt::format("task{}", i);
      affinity.scopeItemName() = "host0";
      affinity.affinity() = 1;
      spec.affinities()->push_back(affinity);
    }
    solver->addGoal(spec);
  }

  // Use the optimal solver.
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Count how many tasks from each job are in each host.
  std::map<std::string, std::map<std::string, int>> hostToJobCount;
  for (auto& [task, host] : *solution.assignment()) {
    auto& job = taskToJobTrivial.at(task);
    ++hostToJobCount[host][job];
  }

  // The only optimal solution is to place 90 tasks in host0 and 10 tasks in
  // host1.
  EXPECT_EQ(90, hostToJobCount["host0"]["job0"]);
  EXPECT_EQ(10, hostToJobCount["host1"]["job0"]);
}

TEST_P(GroupCountTest, MinBoundDoNotWorsen) {
  // Showcase the behavior of not worsening the constraint brokenness of any
  // particular scope item even if overall brokenness stays the same.
  solver = initializeTestProblemSolver();
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0"}},
          {"host1", {"task1"}},
      });

  // Both tasks belong to the same job.
  solver->addPartition(
      "job",
      folly::F14FastMap<std::string, std::string>{
          {"task0", "job0"}, {"task1", "job0"}});

  {
    // Each host requires a minimum of 2 tasks of each job.
    GroupCountSpec spec;
    spec.scope() = "host";
    spec.partitionName() = "job";
    spec.dimension() =
        useDynamicDimension() ? addDynamicTaskCount() : "task_count";
    spec.limit()->type() = LimitType::ABSOLUTE;
    spec.limit()->globalLimit() = 2;
    spec.bound() = GroupCountSpecBound::MIN;
    spec.zeroAllowed() = false;
    solver->addConstraint(spec);
  }

  {
    // Add an incentive for task0 to move to host1.
    AssignmentAffinitiesSpec spec;
    spec.scope() = "host";
    {
      AssignmentAffinity affinity;
      affinity.objectName() = "task0";
      affinity.scopeItemName() = "host1";
      affinity.affinity() = 1;
      spec.affinities()->push_back(affinity);
    }
    solver->addGoal(spec);
  }

  {
    // Penalize moving objects.
    MinimizeMovementSpec spec;
    spec.scope() = "host";
    spec.dimension() = "task_count";
    solver->addGoal(spec);
  }

  // Use the optimal solver.
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  // task0 moves to host1 because it has an incentive, but then task1 moves to
  // host0, even though it pays a penalty, because the min bound constraint must
  // not worsen in host0.
  EXPECT_EQ("host1", assignment.at("task0"));
  EXPECT_EQ("host0", assignment.at("task1"));
}

class GroupCountTestWithDynamicDimension : public ::testing::Test {
 protected:
  void SetUp() override {
    solver = initializeTestProblemSolver();
    solver->setObjectName("task");
    solver->setContainerName("host");

    // There are 6 tasks evenly spread across the 2 hosts.
    solver->setAssignment(
        std::map<std::string, std::vector<std::string>>{
            {"host0", {"task0", "task2", "task4"}},
            {"host1", {"task1", "task3", "task5"}}});

    // Tasks are evenly partitioned into 3 jobs.
    solver->addPartition(
        "job",
        std::map<std::string, std::string>{
            {"task0", "job0"},
            {"task1", "job0"},
            {"task2", "job1"},
            {"task3", "job1"},
            {"task4", "job2"},
            {"task5", "job2"}});

    // Define a dynamic dimension on objects.
    solver->addDynamicObjectDimension(
        "dynamicWeight",
        "host",
        folly::F14FastMap<std::string, std::map<std::string, double>>{
            {"host0", {{"task0", 7.1}, {"task2", 4}, {"task4", 5}}},
            {"host1", {{"task1", 2.9}, {"task3", 6}, {"task5", 5}}}});
    solver->addSolver(makeDefaultLocalSearchSolver());
  }

  std::unique_ptr<ProblemSolver> solver;
};

TEST_F(
    GroupCountTestWithDynamicDimension,
    LimitRelativeToGroupSizeWithDynamicDimension) {
  // Add a GroupCountSpec goal with a limit of 10% relative to group size
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = "dynamicWeight";
  groupCountSpec.limitRelativeTo() = GroupCountSpecLimitRelativeTo::GROUP_SIZE;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 0.1;
  groupCountSpec.limit() = limit;

  solver->addGoal(groupCountSpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Assert goal value for the initial assignment:
  // - host0
  //   - limit of job0 = 0.1 * 7.1, job1 = 0.1 * 4, job3 = 0.1*5
  // (Observe that the limit value for a group differs based on the scopeItem
  // because of dynamic dimensions)
  //   - job0
  //     - utilization: 7.1
  //     - limit exceeded by: 7.1 - 0.1*7.1 = 0.9*7.1
  //   - job1
  //     - utilization: 4
  //     - limit exceeded by: 4 - 0.1*4 = 0.9*4
  //   - job2
  //     - utilization = 5
  //     - limit exceeded by: 5 - 0.1*5 = 0.9*5
  // - host1
  //   - limit of job0 = 0.1 * 2.9, job1 = 0.1 * 6, job3 = 0.1*5
  // (Observe that the limit value for a group differs based on the scopeItem
  // because of dynamic dimensions)
  //   - job0
  //     - utilization: 2.9
  //     - limit exceeded by: 2.9 - 0.1*2.9 = 0.9*2.9
  //   - job1
  //     - utilization: 6
  //     - limit exceeded by: 6 - 0.1*6 = 0.9*6
  //   - job2
  //     - utilization = 5
  //     - limit exceeded by: 5 - 0.1*5 = 0.9*5
  // - sum of limit excess for all combinations of host and job:
  //   0.9*30 = 27
  const double initialGoalValue =
      *solution.initialGlobalObjective()->goals()->at(0).value();
  EXPECT_NEAR(27.0, initialGoalValue, 1e-8);

  // there is a unique solution which achieves a goal value of 0, which is to
  // just move every object in host0 to host1 and vice-versa
  const double finalGoalValue =
      *solution.finalGlobalObjective()->goals()->at(0).value();
  EXPECT_NEAR(0.0, finalGoalValue, 1e-8);

  auto& assignment = *solution.assignment();
  EXPECT_EQ("host0", assignment.at("task1"));
  EXPECT_EQ("host0", assignment.at("task3"));
  EXPECT_EQ("host0", assignment.at("task5"));
  EXPECT_EQ("host1", assignment.at("task0"));
  EXPECT_EQ("host1", assignment.at("task2"));
  EXPECT_EQ("host1", assignment.at("task4"));
}

TEST_F(GroupCountTestWithDynamicDimension, AbsoluteLimitWithDynamicDimension) {
  // Add a GroupCountSpec goal with an absolute limit of 1
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.dimension() = "dynamicWeight";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1;
  groupCountSpec.limit() = limit;

  solver->addGoal(groupCountSpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Assert goal value for the initial assignment:
  // - host0
  //   - job0
  //     - utilization: 7.1
  //     - limit exceeded by: 7.1 - 1 = 6.1
  //   - job1
  //     - utilization: 4
  //     - limit exceeded by: 4 - 1 = 3
  //   - job2
  //     - utilization = 5
  //     - limit exceeded by: 5 - 1 = 4
  // - host1
  //   - job0
  //     - utilization: 2.9
  //     - limit exceeded by: 2.9 - 1 = 1.9
  //   - job1
  //     - utilization: 6
  //     - limit exceeded by: 6 - 1 = 5
  //   - job2
  //     - utilization = 5
  //     - limit exceeded by: 5 - 1 = 4
  // - sum of limit excess for all combinations of host and job:
  //   6.1 + 3 + 4 + 1.9 + 5 +  4 = 24
  const double initialGoalValue =
      *solution.initialGlobalObjective()->goals()->at(0).value();
  EXPECT_NEAR(24.0, initialGoalValue, 1e-8);

  // there is a unique solution which achieves a goal value of 0, which is to
  // just move every object in host0 to host1 and vice-versa
  const double finalGoalValue =
      *solution.finalGlobalObjective()->goals()->at(0).value();
  EXPECT_NEAR(0.0, finalGoalValue, 1e-8);

  auto& assignment = *solution.assignment();
  EXPECT_EQ("host0", assignment.at("task1"));
  EXPECT_EQ("host0", assignment.at("task3"));
  EXPECT_EQ("host0", assignment.at("task5"));
  EXPECT_EQ("host1", assignment.at("task0"));
  EXPECT_EQ("host1", assignment.at("task2"));
  EXPECT_EQ("host1", assignment.at("task4"));
}

class GroupCountSpecWithRoutingTrafficLimitTest : public ::testing::Test {
 protected:
  std::vector<std::vector<std::string>> routingLogic1 = {
      {"region1"},
      {"region0", "region2"}};
  std::vector<std::vector<std::string>> routingLogic2 = {
      {"region0", "region1", "region2"}};
  std::vector<std::vector<std::string>> routingLogic3 = {
      {"region2"},
      {"region0", "region1"}};

  std::unordered_map<std::string, GroupRoutingRings> getGroupToRoutingRings(
      bool useDefaultRoutingLogic) {
    std::unordered_map<std::string, GroupRoutingRings> groupToRoutingRings;

    constexpr double totalTrafficUnitsPerTenant = 100;
    // 98% of the traffic for tenant0 originates in region0, 2% at region1,
    GroupRoutingRings tenant0Rings;
    tenant0Rings.routingRings()->push_back(getRoutingRing(
        "region0", // origin
        0.98 * totalTrafficUnitsPerTenant, // originTraffic
        useDefaultRoutingLogic
            ? std::nullopt
            : std::make_optional(routingLogic1) // routingLogic
        ));

    tenant0Rings.routingRings()->push_back(getRoutingRing(
        "region1", // origin
        0.02 * totalTrafficUnitsPerTenant, // originTraffic
        useDefaultRoutingLogic
            ? std::nullopt
            : std::make_optional(routingLogic2) // routingLogic
        ));

    groupToRoutingRings.emplace("tenant0", tenant0Rings);

    // 100% of the traffic for tenant1 originates in region2.
    GroupRoutingRings tenant1Rings;
    tenant1Rings.routingRings()->push_back(getRoutingRing(
        "region2", // origin
        totalTrafficUnitsPerTenant, // originTraffic
        useDefaultRoutingLogic
            ? std::nullopt
            : std::make_optional(routingLogic3) // routingLogic
        ));

    groupToRoutingRings.emplace("tenant1", tenant1Rings);

    return groupToRoutingRings;
  }

  std::unordered_map<std::string, std::vector<std::vector<std::string>>>
  getDefaultOriginToDestinationScopeItemSets(bool useDefaultRoutingLogic) {
    std::unordered_map<std::string, std::vector<std::vector<std::string>>>
        defaultMap;
    if (useDefaultRoutingLogic) {
      defaultMap.emplace("region0", routingLogic1);
      defaultMap.emplace("region1", routingLogic2);
      defaultMap.emplace("region2", routingLogic3);
    }
    return defaultMap;
  }

  void setUpProblem(bool useDefaultRoutingLogic) {
    // Setup is very similar to the one in ObjectPartitionRoutingDimensionTest.
    // simulate 2 services with 2 replicas each
    solver_ = initializeTestProblemSolver();
    solver_->setObjectName("replica");
    solver_->setContainerName("region");

    solver_->setAssignment(
        std::vector<std::pair<std::string, std::vector<std::string>>>{
            {"region0", {"replica1:0"}},
            {"region1", {"replica0:0", "replica1:1"}},
            {"region2", {"replica0:1"}},
            {"dummyRegion0", {}},
            {"dummyRegion1", {}},
        });

    // Group replicas of the same tenant together in a partition.
    {
      replicaToTenant_ = {
          {"replica0:0", "tenant0"},
          {"replica0:1", "tenant0"},
          {"replica1:0", "tenant1"},
          {"replica1:1", "tenant1"},
      };
      solver_->addPartition("tenant", replicaToTenant_);
    }

    // define a scope without dummy regions
    solver_->addScope(
        "realRegions",
        std::vector<std::pair<std::string, std::string>>{
            {{"region0", "region0"},
             {"region1", "region1"},
             {"region2", "region2"}}});

    {
      // define routingConfig
      const std::unordered_map<
          std::string,
          std::unordered_map<std::string, double>>
          originToDestinationLatency = {
              {"region0", {{"region0", 5}, {"region1", 10}, {"region2", 5}}},
              {"region1", {{"region0", 5}, {"region1", 0}, {"region2", 15}}},
              {"region2", {{"region0", 50}, {"region1", 40}, {"region2", 0}}}};

      solver_->addRoutingConfig(
          "routingConfig1",
          "realRegions",
          "tenant",
          getGroupToRoutingRings(useDefaultRoutingLogic),
          originToDestinationLatency,
          getDefaultOriginToDestinationScopeItemSets(useDefaultRoutingLogic));
    }

    {
      // Define a dynamic dimension on objects. scope is deliberately set as
      // "region" to test the scenario where GroupCountSpec's scope is not the
      // same as the dynamic dimension's scope
      solver_->addDynamicObjectDimension(
          "load",
          "region",
          folly::
              F14FastMap<std::string, folly::F14FastMap<std::string, double>>{
                  {
                      "region0",
                      {
                          {"replica0:0", 5},
                          {"replica0:1", 5},
                          {"replica1:0", 10},
                          {"replica1:1", 10},
                      },
                  },
                  {"region1", {{"replica1:0", 10}, {"replica1:1", 10}}},
                  {"region2", {{"replica0:0", 5}, {"replica0:1", 5}}},
              },
          0.0 /*defaultValue*/);
    }

    // Constraint: At most one replica of the same tenant can be placed in the
    // same region. No replica should be placed in any dummy region
    {
      GroupCountSpec spec;
      spec.scope() = "region";
      spec.partitionName() = "tenant";
      spec.dimension() = "replica_count";
      spec.limit()->globalLimit() = 1;
      spec.limit()->scopeItemLimits() = {
          {"dummyRegion0", 0.0}, {"dummyRegion1", 0.0}};
      solver_->addConstraint(spec);
    }

    // Goal:  minimize limit violations
    {
      GroupCountSpec spec;
      spec.scope() = "realRegions";
      spec.partitionName() = "tenant";
      spec.dimension() = "load";
      spec.bound() = GroupCountSpecBound::MIN;

      Limit limit;
      limit.type() = LimitType::RELATIVE;
      limit.globalLimit() = 20;
      limit.groupLimits() = {{"tenant0", 10}};

      spec.limit() = std::move(limit);
      spec.limitRelativeTo() =
          GroupCountSpecLimitRelativeTo::GROUP_TO_SCOPE_ITEM_ROUTING_TRAFFIC;

      spec.routingConfigForLimit() = "routingConfig1";

      solver_->addGoal(spec);
    }
    solver_->addSolver(makeDefaultLocalSearchSolver());
  }

  static RoutingRing getRoutingRing(
      const std::string& origin,
      double originTraffic,
      const std::optional<std::vector<std::vector<std::string>>>&
          destinationScopeItemSets) {
    RoutingRing ring;
    ring.originScopeItem() = origin;
    ring.originTraffic() = originTraffic;
    if (destinationScopeItemSets.has_value()) {
      ring.destinationScopeItemSets() = destinationScopeItemSets.value();
    }

    return ring;
  }

  std::unique_ptr<ProblemSolver> solver_;
  std::map<std::string, std::string> replicaToTenant_;
};

TEST_F(GroupCountSpecWithRoutingTrafficLimitTest, Basic) {
  setUpProblem(false);

  auto solution = solver_->solve();

  // In the initial assignment:
  //  Tenant0:
  //    - 98% of traffic is from region0:
  //      -- 98% goes to region1
  //    - 2% of traffic is from region1
  //      -- 1% goes to region1, 1% to region2
  //
  //  Tenant1:
  //    - 100% of traffic is from region2:
  //      -- 50% goes to region0, 50% to region1
  //
  // Limits:
  // for each pair (tenant, region),
  // finalLimit(tenant, region) = traffic(tenant, region) * limit(tenant,
  // region), where traffic(tenant, region) is the fraction of tenant's traffic
  // that is sent to region and limit(tenant, region) is based on value
  // specified in spec.limit().
  //
  // util(tenant, region) = sum of load values of replicas that belong to tenant
  // that are placed in region
  //
  // goalValue = max(0.0, finalLimit - util), since the bound is MIN (and hence
  // we want util >= finalLimit)
  //
  //  --tenant0, region0:
  //            util = 0.0 (no replica of tenant0 is in region0)
  //            finalLimit = 0.0 * 10 = 0.0
  //            goalValue = max(0.0, 0.0 - 0.0) = 0.0
  //  --tenant0, region1:
  //            util = 0.0 (dimension value is 0)
  //            finalLimit = 0.99 * 10.0 = 9.9
  //            goalValue = max(0.0, 9.9 - 0) = 9.9
  //  --tenant0, region2:
  //            util = 5.0
  //            finalLimit = 0.01 * 10 = 0.1
  //            goalValue = max(0, 0.1 - 5.0) = 0.0
  //  --tenant1, region0:
  //            util = 10.0
  //            finalLimit = 0.5 * 20 = 10.0
  //            goalValue = max(0, 10.0 - 10.0) = 0.0
  //  --tenant1, region1:
  //            util = 10
  //            finalLimit = 0.5 * 20 = 10.0
  //            goalValue = max(0.0, 10 - 10) = 0.0
  //  --tenant1, region2:
  //            util = 0.0 (no replica of tenant1 is in region2)
  //            finalLimit = 0.0 * 20 = 0.0
  //            goalValue = max(0.0, 0 - 0) = 0.0

  // initial objective value = 9.9 + 0.0 = 9.9
  EXPECT_NEAR(9.9, *solution.initialObjective()->value(), 1e-8);

  // In the final assignment: move a replica of tenant0 from region1 to
  // region0. This enables 98% from region0 of traffic to be evenly spread
  // across region0 and region2. Also, after this 2% traffic from
  // region 1 is equally spread to region0 and region2.

  //  Tenant0:
  //    - 98% of traffic is from region0:
  //      -- 49% goes to region0, 49% to region2
  //    - 2% of traffic is from region1
  //      -- 1% goes to region0, 1% to region2
  //
  //  Tenant1:
  //    - 100% of traffic is from region2:
  //      -- 50% goes to region0, 50% to region1
  //
  // Limits:
  // for each pair (tenant, region), finalLimit = traffic(tenant, region) *
  // limit(tenant, region), where limit(tenant, region) are based on values in
  // spec.limit()
  //
  //  --tenant0, region0:
  //            util = 5.0
  //            finalLimit = 0.5 * 10 = 5.0
  //            goalValue = max(0, 5 - 5) = 0.0
  //  --tenant0, region1:
  //            util = 0.0 (no replica of tenant0 is in region1)
  //            finalLimit = 0.0 * 10 = 0.0
  //            goalValue = max(0, 0.0 - 0.0) = 0.0
  //  --tenant0, region2:
  //            util = 5.0
  //            finalLimit = 0.5 * 10 = 5.0
  //            goalValue = max(0.0, 5 - 5) = 0.0
  //  --tenant1, region0:
  //            util = 10.0
  //            finalLimit = 0.5 * 20 = 10.0
  //            goalValue = max(0.0, 10 - 10) = 0.0
  //  --tenant1, region1:
  //            util = 10.0
  //            finalLimit = 0.5 * 20 = 10.0
  //            goalValue = max(0.0, 10 - 10) = 0.0
  //  --tenant1, region2:
  //            util = 0.0 (no replica of tenant1 is in region2)
  //            finalLimit = 0.0 * 20 = 0.0
  //            goalValue = max(0.0, 0.0 - 0.0) = 0.0

  // final objective value = 0.0
  EXPECT_NEAR(0.0, *solution.finalObjective()->value(), 1e-8);

  auto& assignment = *solution.assignment();
  std::unordered_map<std::string, std::unordered_map<std::string, int>>
      regionTenantCount;
  for (auto& [traffic, region] : assignment) {
    auto tenant = replicaToTenant_.at(traffic);
    ++regionTenantCount[region][tenant];
  }

  // we expect a replica of tenant0 to have moved from region1 to region0
  EXPECT_EQ(1, regionTenantCount["region0"]["tenant0"]);
  EXPECT_EQ(1, regionTenantCount["region0"]["tenant1"]);

  // we expect that region2 has no replica of tenant1 (since if so, then all of
  // tenant1's traffic will be sent there, which in turn would result in a limit
  // violation since dimension value of any replica of tenant1 in region2 is
  // zero)
  EXPECT_EQ(1, regionTenantCount["region2"]["tenant0"]);
  EXPECT_EQ(0, regionTenantCount["region2"]["tenant1"]);
}

TEST_F(GroupCountSpecWithRoutingTrafficLimitTest, useDefaultRoutingLogic) {
  setUpProblem(true);

  auto solution = solver_->solve();

  EXPECT_NEAR(9.9, *solution.initialObjective()->value(), 1e-8);

  EXPECT_NEAR(0.0, *solution.finalObjective()->value(), 1e-8);

  auto& assignment = *solution.assignment();
  std::unordered_map<std::string, std::unordered_map<std::string, int>>
      regionTenantCount;
  for (auto& [traffic, region] : assignment) {
    auto tenant = replicaToTenant_.at(traffic);
    ++regionTenantCount[region][tenant];
  }

  EXPECT_EQ(1, regionTenantCount["region0"]["tenant0"]);
  EXPECT_EQ(1, regionTenantCount["region0"]["tenant1"]);

  EXPECT_EQ(1, regionTenantCount["region2"]["tenant0"]);
  EXPECT_EQ(0, regionTenantCount["region2"]["tenant1"]);
}

class GroupCountAllowedAssignmentsTest : public ::testing::Test {};

TEST_F(GroupCountAllowedAssignmentsTest, AllowedAssignmentsConstraint) {
  // only a small subset of the objects have "AllowedAssignment"-type
  // constraints
  auto solver = initializeTestProblemSolver();
  solver->setObjectName("task");
  solver->setContainerName("host");

  // host0 has the first 5 tasks, host1 has the remaining 4 tasks. two empty
  // hosts host2 and host3
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2", "task3", "task4"}},
          {"host1", {"task5", "task6", "task7", "task8"}},
          {"host2", {}},
          {"host3", {}},
      });

  // create a partition with just one group "constrained_objects" which has all
  // the objects tha have "AllowedAssignment" constraints. Here those objects
  // are "task0" and "task1"
  solver->addPartition(
      "constrained_objects_partition",
      folly::F14FastMap<std::string, std::vector<std::string>>{
          {"constrained_objects", {"task0", "task1"}}});

  // create a dynamic dimension "avoid_assignments" w.r.t. objects in
  // "constrained_objects". The other objects will have a value of 1.0 (but
  // these values are not going to be used in any constraint)
  // in this example, task0 can only be assigned to host1 and host3, while task1
  // can only be assigned to host0 and host1
  solver->addDynamicObjectDimension(
      "avoid_assignments",
      "host",
      folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>{
          {"host0", {{"task1", 0.0}}},
          {"host1", {{"task0", 0.0}, {"task1", 0.0}}},
          {"host3", {{"task0", 0.0}}},
      },
      /*defaultValue=*/1.0);

  // create a GroupCountSpec with dimension='allowed_assignments', scope='host',
  // and partition='constrained_objects_partition'. this enforces the allowed
  // assignment constraints for objects in group "constrained_objects"
  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "constrained_objects_partition";
  groupCountSpec.dimension() = "avoid_assignments";
  groupCountSpec.limit()->globalLimit() = 0.0;
  solver->addConstraint(groupCountSpec);

  // just want to force moves to host3 so that we have a unique destination for
  // task0
  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "task_count";
  capacitySpec.bound() = CapacitySpecBound::MIN;
  capacitySpec.limit()->globalLimit() = 0.0;
  capacitySpec.limit()->scopeItemLimits() = {{"host3", 1.0}};
  solver->addConstraint(capacitySpec);

  // add single move type
  LocalSearchSolverSpec spec;
  spec.moveTypeList() = {ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec())};
  solver->addSolver(spec);

  // solve
  const auto solution = solver->solve();

  // uncomment line below save to manifold
  // solver->persistToManifold();

  // assert that task0 moved to host3
  EXPECT_EQ("host3", solution.assignment()->at("task0"));
}
