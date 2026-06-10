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
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

class GroupMoveLimitSpecTest
    : public ::testing::TestWithParam<std::tuple<int, bool>> {
 protected:
  void SetUp() override;
  std::unique_ptr<ProblemSolver> solver_;
  std::map<std::string, std::string> taskToGroup_;
};

/** Instance description:
    - There are 3 hosts, 4 tasks divided into two groups
    - group 0 = {task0, task1}  group 1 = {task2, task3}
    - Initially all 4 tasks are assigned to host0
    - group0 tasks prefer host1 and group1 tasks prefer host2
    - Goal: optimize for preference while respecting move limits
      (Move limits will be set by individual testcases)
  */
void GroupMoveLimitSpecTest::SetUp() {
  solver_ = initializeTestProblemSolver(
      {.executorThreadCount = std::get<0>(GetParam())});
  solver_->setObjectName("task");
  solver_->setContainerName("host");

  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
      });

  taskToGroup_ = {
      {"task0", "group0"},
      {"task1", "group0"},
      {"task2", "group1"},
      {"task3", "group1"},
  };
  // Let Rebalancer know of the task group partition
  solver_->addPartition("group", taskToGroup_);

  // Assign preference to other hosts than itself
  std::vector<AssignmentAffinity> affinities;
  // Make task0, task1 prefer host1
  for (auto taskId : {"task0", "task1"}) {
    affinities.push_back(makeAssignmentAffinity(taskId, "host1", 1));
  }
  // Make task2, task3 prefer host2
  for (auto taskId : {"task2", "task3"}) {
    affinities.push_back(makeAssignmentAffinity(taskId, "host2", 1));
  }
  AssignmentAffinitiesSpec assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = std::move(affinities);
  solver_->addGoal(assignmentAffinitiesSpec);

  // If parameter is true, add an optimal solver spec. The default behavior is
  // with a local search solver spec, so we don't need to do anything if
  // parameter is false.
  if (std::get<1>(GetParam())) {
    REBALANCER_SKIP_IF_NO_MIP_SOLVER();
    solver_->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());
  } else {
    solver_->addSolver(makeDefaultLocalSearchSolver());
  }
}

// Check if move limits are respected when both groups are moveable
TEST_P(GroupMoveLimitSpecTest, BothGroupsMoveable) {
  GroupMoveLimitSpec groupMoveLimitSpec;
  groupMoveLimitSpec.partitionName() = "group";
  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1;
  groupMoveLimitSpec.limit() = limit;
  solver_->addConstraint(groupMoveLimitSpec);

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  // Count how many tasks from each group are in each host.
  std::map<std::string, std::map<std::string, int>> numTasksPerHostPerGroup;
  for (auto& [task, host] : *solution.assignment()) {
    auto group = taskToGroup_.at(task);
    ++numTasksPerHostPerGroup[host][group];
  }
  EXPECT_EQ(numTasksPerHostPerGroup["host0"]["group0"], 1);
  EXPECT_EQ(numTasksPerHostPerGroup["host1"]["group0"], 1);
  EXPECT_EQ(numTasksPerHostPerGroup["host2"]["group1"], 1);
}

// Check if move limits are respected when some groups are moveable and some are
// not moveable
TEST_P(GroupMoveLimitSpecTest, SomeGroupsMoveable) {
  GroupMoveLimitSpec groupMoveLimitSpec;
  groupMoveLimitSpec.partitionName() = "group";
  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.groupLimits() = {{"group0", 2}, {"group1", 0}};
  groupMoveLimitSpec.limit() = limit;
  solver_->addConstraint(groupMoveLimitSpec);

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  // Count how many tasks from each group are in each host.
  std::map<std::string, std::map<std::string, int>> numTasksPerHostPerGroup;
  for (auto& [task, host] : *solution.assignment()) {
    auto group = taskToGroup_.at(task);
    ++numTasksPerHostPerGroup[host][group];
  }
  EXPECT_EQ(numTasksPerHostPerGroup["host0"]["group0"], 0);
  EXPECT_EQ(numTasksPerHostPerGroup["host1"]["group0"], 2);

  EXPECT_EQ(numTasksPerHostPerGroup["host0"]["group1"], 1);
  EXPECT_EQ(numTasksPerHostPerGroup["host1"]["group1"], 1);
}

TEST_P(
    GroupMoveLimitSpecTest,
    UnmovableGroupsWithCertainSourceAndDestinationNotAffectingLimit) {
  GroupMoveLimitSpec groupMoveLimitSpec;
  groupMoveLimitSpec.partitionName() = "group";
  groupMoveLimitSpec.limit()->globalLimit() = 0;
  groupMoveLimitSpec.sourceScopeItemsAffectingLimitFilter()
      ->itemsWhitelist() = {"host0", "host2"};
  groupMoveLimitSpec.destinationScopeItemsAffectingLimitFilter()
      ->itemsBlacklist() = {"host1"};

  solver_->addConstraint(groupMoveLimitSpec);
  solver_->addGoal(MinimizeMovementSpec());

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  // Count how many tasks from each group are in each host.
  std::map<std::string, std::map<std::string, int>> numTasksPerHostPerGroup;
  for (auto& [task, host] : *solution.assignment()) {
    auto group = taskToGroup_.at(task);
    ++numTasksPerHostPerGroup[host][group];
  }
  EXPECT_EQ(numTasksPerHostPerGroup["host0"]["group0"], 0);
  // although the limit is 0, both tasks of group1 can move since moves to
  // "host1" as the destination do not affect the limit
  EXPECT_EQ(numTasksPerHostPerGroup["host1"]["group0"], 2);

  EXPECT_EQ(numTasksPerHostPerGroup["host0"]["group1"], 1);
  // although the limit is 0, task3 in host1 can move since moves from
  // "host1" as source do not affect the limit
  EXPECT_EQ(numTasksPerHostPerGroup["host2"]["group1"], 1);
}

TEST_P(GroupMoveLimitSpecTest, CustomDimension) {
  solver_->addObjectDimension(
      "weight",
      folly::F14FastMap<std::string, double>{{"task0", 2}, {"task1", 2}},
      1.0);

  GroupMoveLimitSpec groupMoveLimitSpec;
  groupMoveLimitSpec.partitionName() = "group";
  groupMoveLimitSpec.dimension() = "weight";
  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1;
  groupMoveLimitSpec.limit() = limit;
  solver_->addConstraint(groupMoveLimitSpec);

  // Get a solution from Rebalancer.
  // Given the weight of task0 and task1 are both 2, we expect that none of them
  // to move. Either task2 or task3 should move to host2.
  auto solution = solver_->solve();
  auto& assignment = *solution.assignment();

  // Property 1: group0 tasks can't move (each has weight=2, exceeding limit=1)
  EXPECT_EQ("host0", assignment.at("task0"));
  EXPECT_EQ("host0", assignment.at("task1"));

  // Property 2: exactly one group1 task moves to its preferred host2.
  // Both task2 (on host0) and task3 (on host1) have weight=1 and equal
  // preference for host2, so the solver may pick either.
  const auto task2Host = assignment.at("task2");
  const auto task3Host = assignment.at("task3");
  EXPECT_TRUE(task2Host == "host2" || task3Host == "host2")
      << "At least one group1 task should move to host2";

  // Property 3: move limit respected — at most one group1 task moved
  const int group1Moves =
      (task2Host != "host0" ? 1 : 0) + (task3Host != "host1" ? 1 : 0);
  EXPECT_LE(group1Moves, 1) << "Group1 move limit of 1 should be respected";
}

TEST_P(GroupMoveLimitSpecTest, CustomDynamicDimension) {
  solver_->addDynamicObjectDimension(
      "weight",
      "host",
      folly::F14FastMap<std::string, std::map<std::string, double>>{
          {"host0", {{"task0", 1}, {"task1", 1}}},
          {"host1", {{"task0", 2}, {"task1", 2}}},
          {"host2", {{"task0", 2}, {"task1", 2}, {"task3", 2}}}},
      1.0);

  GroupMoveLimitSpec groupMoveLimitSpec;
  groupMoveLimitSpec.partitionName() = "group";
  groupMoveLimitSpec.dimension() = "weight";
  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1;
  groupMoveLimitSpec.limit() = limit;
  solver_->addConstraint(groupMoveLimitSpec);

  // Get a solution from Rebalancer.
  // Given the weight of task0 and task1 are both 2, we expect that none of them
  // to move. task3 cannot move to host2 (because of weight 2), so we expect
  // only task2 to move to host2.
  auto solution = solver_->solve();
  // Count how many tasks from each group are in each host.
  std::map<std::string, std::map<std::string, int>> numTasksPerHostPerGroup;
  for (auto& [task, host] : *solution.assignment()) {
    auto group = taskToGroup_.at(task);
    ++numTasksPerHostPerGroup[host][group];
  }
  EXPECT_EQ(numTasksPerHostPerGroup["host0"]["group0"], 2);
  EXPECT_EQ(numTasksPerHostPerGroup["host1"]["group0"], 0);
  EXPECT_EQ(numTasksPerHostPerGroup["host1"]["group1"], 1);
  EXPECT_EQ(numTasksPerHostPerGroup["host2"]["group1"], 1);
}

TEST_P(
    GroupMoveLimitSpecTest,
    DynamicDimensionWithCertainSourceAndDestinationNotAffectingLimit) {
  solver_->addDynamicObjectDimension(
      "weight",
      "host",
      folly::F14FastMap<std::string, std::map<std::string, double>>{
          {"host0", {{"task0", 1}, {"task1", 1}}},
          {"host1", {{"task0", 2}, {"task1", 2}}},
          {"host2", {{"task0", 2}, {"task1", 2}, {"task3", 2}}}},
      1.0);

  GroupMoveLimitSpec groupMoveLimitSpec;
  groupMoveLimitSpec.partitionName() = "group";
  groupMoveLimitSpec.dimension() = "weight";
  groupMoveLimitSpec.limit()->globalLimit() = 0;
  groupMoveLimitSpec.sourceScopeItemsAffectingLimitFilter()
      ->itemsWhitelist() = {"host0", "host2"};
  groupMoveLimitSpec.destinationScopeItemsAffectingLimitFilter()
      ->itemsBlacklist() = {"host1"};

  solver_->addConstraint(groupMoveLimitSpec);
  solver_->addGoal(MinimizeMovementSpec());

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  // Count how many tasks from each group are in each host.
  std::map<std::string, std::map<std::string, int>> numTasksPerHostPerGroup;
  for (auto& [task, host] : *solution.assignment()) {
    auto group = taskToGroup_.at(task);
    ++numTasksPerHostPerGroup[host][group];
  }

  EXPECT_EQ(numTasksPerHostPerGroup["host0"]["group0"], 0);
  // although the limit is 0, both tasks of group1 can move since moves to
  // "host1" as the destination do not affect the limit
  EXPECT_EQ(numTasksPerHostPerGroup["host1"]["group0"], 2);

  EXPECT_EQ(numTasksPerHostPerGroup["host0"]["group1"], 1);
  // although the limit is 0, task3 in host1 can move since moves from
  // "host1" as source do not affect the limit
  EXPECT_EQ(numTasksPerHostPerGroup["host2"]["group1"], 1);
}

INSTANTIATE_TEST_SUITE_P(
    GroupMoveLimitSpecTest,
    GroupMoveLimitSpecTest,
    ::testing::Combine(testThreadCounts(), ::testing::Values(false, true)));
