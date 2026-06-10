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

#include "algopt/rebalancer/algopt_common/TestUtils.h"
#include "algopt/rebalancer/interface/tests/utils.h"

#include <fmt/core.h>
#include <folly/container/irange.h>
#include <folly/Random.h>
#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

using namespace folly;
using namespace std;
using namespace facebook::rebalancer::interface;

class MinimizeNthLargestUtilizationTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    solver_ = initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver_->setObjectName("task");
    solver_->setContainerName("host");
  }

  void setUpTasksAndHosts(
      int nTasks,
      int nHosts,
      pair<map<string, double>, double> taskCpuWithDefault = {{}, 0.0},
      map<string, double> hostCpu = {},
      optional<map<string, vector<string>>> initialAssignment = std::nullopt) {
    map<string, vector<string>> assignment;
    if (initialAssignment) {
      assignment = initialAssignment.value();
    } else {
      // if an initial assignment is not given, then create one
      for (const auto i : folly::irange(nHosts)) {
        auto host = fmt::format("host{}", i);
        assignment.emplace(host, vector<string>());
      }

      for (const auto i : folly::irange(nTasks)) {
        const string host = fmt::format("host{}", Random::rand32(0, 4));
        const string task = fmt::format("task{}", i);
        assignment[host].push_back(task);
      }
    }

    solver_->setAssignment(assignment);
    solver_->addContainerDimension("cpu", hostCpu);
    solver_->addObjectDimension(
        "cpu", taskCpuWithDefault.first, taskCpuWithDefault.second);
  }

  std::unique_ptr<ProblemSolver> solver_;
};

INSTANTIATE_TEST_CASE_P(
    ThreadCounts,
    MinimizeNthLargestUtilizationTest,
    testThreadCounts());

TEST_P(MinimizeNthLargestUtilizationTest, ErrorOnNegativeN) {
  setUpTasksAndHosts(1, 1);

  facebook::rebalancer::interface::MinimizeNthLargestUtilizationSpec
      minimizeNthLargestUtilizationSpec;
  minimizeNthLargestUtilizationSpec.scope() = "host";
  minimizeNthLargestUtilizationSpec.dimension() = "cpu";
  minimizeNthLargestUtilizationSpec.n() = -1;

  REBALANCER_EXPECT_RUNTIME_ERROR(
      solver_->addGoal(minimizeNthLargestUtilizationSpec),
      "MinimizeNthLargestUtilizationSpec::n must be non-negative");
}

TEST_P(MinimizeNthLargestUtilizationTest, ErrorOnNegativeTargetUtilization) {
  setUpTasksAndHosts(1, 1);

  facebook::rebalancer::interface::MinimizeNthLargestUtilizationSpec
      minimizeNthLargestUtilizationSpec;
  minimizeNthLargestUtilizationSpec.scope() = "host";
  minimizeNthLargestUtilizationSpec.dimension() = "cpu";
  minimizeNthLargestUtilizationSpec.n() = 1;
  minimizeNthLargestUtilizationSpec.targetUtilization() = -0.1;

  REBALANCER_EXPECT_RUNTIME_ERROR(
      solver_->addGoal(minimizeNthLargestUtilizationSpec),
      "MinimizeNthLargestUtilizationSpec::targetUtilization must be non-negative");
}

TEST_P(MinimizeNthLargestUtilizationTest, Basic) {
  // create a problem with 4 hosts and 63 tasks; each task is asssigned to a
  // random host.

  // host0 has 100 cpu, all the others have 10 cpu
  const map<string, double> hostCpu = {
      {"host0", 100}, {"host1", 10}, {"host2", 10}, {"host3", 10}};

  // all tasks take 1 cpu
  const pair<map<string, double>, double> taskCpuWithDefault = {{}, 1};

  setUpTasksAndHosts(63, 4, taskCpuWithDefault, hostCpu);

  // in the ideal (relative) balance definition:
  // - the largest relative utilization is minimal
  // - while the above holds true, the second largest relative utilization is
  // minimal
  // - while the above holds true, the third largest relative utilization is
  // minimal
  // - and so on
  for (const auto i : folly::irange(4)) {
    if (i != 0) {
      solver_->addGoalBoundary();
    }

    facebook::rebalancer::interface::MinimizeNthLargestUtilizationSpec
        minimizeNthLargestUtilizationSpec;
    minimizeNthLargestUtilizationSpec.scope() = "host";
    minimizeNthLargestUtilizationSpec.dimension() = "cpu";
    minimizeNthLargestUtilizationSpec.n() = i;

    solver_->addGoal(minimizeNthLargestUtilizationSpec);
  }

  solver_->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver_->solve();

  map<string, int> taskCount;
  for (auto& [_, host] : *solution.assignment()) {
    ++taskCount[host];
  }

  // we expect host0 to have 50 tasks (relative utilization = 0.5)
  EXPECT_EQ(50, taskCount["host0"]);

  // the remaining hosts are expected to have 4 or 5 tasks each (relative
  // utilization between 0.4 and 0.5)
  for (const auto i : folly::irange(1, 4)) {
    auto host = fmt::format("host{}", i);
    EXPECT_LE(4, taskCount[host]);
    EXPECT_GE(5, taskCount[host]);
  }
}

TEST_P(MinimizeNthLargestUtilizationTest, BasicWithFilter) {
  // This is the same example as the one above, but where host0 is filtered.

  // create a problem with  4 hosts and 63 tasks; each task is asssigned to a
  // random host.

  // host0 has 100 cpu, all the others have 10 cpu
  const map<string, double> hostCpu = {
      {"host0", 100}, {"host1", 10}, {"host2", 10}, {"host3", 10}};

  // all tasks take 1 cpu
  const pair<map<string, double>, double> taskCpuWithDefault = {{}, 1};

  setUpTasksAndHosts(63, 4, taskCpuWithDefault, hostCpu);

  for (const auto i : folly::irange(4)) {
    if (i != 0) {
      solver_->addGoalBoundary();
    }

    facebook::rebalancer::interface::MinimizeNthLargestUtilizationSpec
        minimizeNthLargestUtilizationSpec;
    minimizeNthLargestUtilizationSpec.scope() = "host";
    minimizeNthLargestUtilizationSpec.dimension() = "cpu";
    minimizeNthLargestUtilizationSpec.n() = i;
    minimizeNthLargestUtilizationSpec.filter()->itemsBlacklist() = {"host0"};

    solver_->addGoal(minimizeNthLargestUtilizationSpec);
  }

  solver_->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver_->solve();

  map<string, int> taskCount;
  for (auto& [_, host] : *solution.assignment()) {
    ++taskCount[host];
  }

  // we expect host0 to have all the 63 tasks (since it is filtered out and
  // hence its utilization is not included)
  EXPECT_EQ(63, taskCount["host0"]);
}

TEST_P(MinimizeNthLargestUtilizationTest, TargetUtilization) {
  // create a problem with 2 hosts and 8 tasks

  // both hosts have 10 cpu
  const map<string, double> hostCpu = {{"host0", 10}, {"host1", 10}};

  // all tasks take 1 cpu
  const pair<map<string, double>, double> taskCpuWithDefault = {{}, 1};

  // initially, all 8 tasks are in host0
  map<string, vector<string>> initialAssignment = {
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
  };

  setUpTasksAndHosts(8, 2, taskCpuWithDefault, hostCpu, initialAssignment);

  // minimize the largest utilization, but only down to 0.5
  facebook::rebalancer::interface::MinimizeNthLargestUtilizationSpec
      minimizeNthLargestUtilizationSpec;
  minimizeNthLargestUtilizationSpec.scope() = "host";
  minimizeNthLargestUtilizationSpec.dimension() = "cpu";
  minimizeNthLargestUtilizationSpec.n() = 0;
  minimizeNthLargestUtilizationSpec.targetUtilization() = 0.5;

  solver_->addGoal(minimizeNthLargestUtilizationSpec);

  solver_->addSolver(makeDefaultLocalSearchSolver());

  // solve the problem
  auto solution = solver_->solve();

  // aggregate task count by host
  map<string, int> taskCount;
  for (auto& [_, host] : *solution.assignment()) {
    ++taskCount[host];
  }

  // expectation: exactly 3 tasks move from host0 to host1, and not 4 because
  // there's no incentive to reduce the largest relative utilization below 0.5
  EXPECT_EQ(5, taskCount["host0"]);
  EXPECT_EQ(3, taskCount["host1"]);

  // the penalty equals largest utilization minus target utilization
  EXPECT_NEAR(0.3, *solution.initialObjective()->value(), 1e-8);
  EXPECT_NEAR(0.0, *solution.finalObjective()->value(), 1e-8);
}

TEST_P(MinimizeNthLargestUtilizationTest, SomeZeroCapacity) {
  // create a problem with 2 hosts and 2 tasks, where one of the hosts has zero
  // capacity

  // one host has 10 cpu, the other one has zero
  const map<string, double> hostCpu = {{"host0", 10}, {"host1", 0}};

  // both tasks take 1 cpu
  const pair<map<string, double>, double> taskCpuWithDefault = {{}, 1};

  // initially, both tasks are in host0
  map<string, vector<string>> initialAssignment = {
      {"host0", {"task0", "task1"}},
      {"host1", {}},
  };

  setUpTasksAndHosts(2, 2, taskCpuWithDefault, hostCpu, initialAssignment);

  // minimize the largest utilization
  facebook::rebalancer::interface::MinimizeNthLargestUtilizationSpec
      minimizeNthLargestUtilizationSpec;
  minimizeNthLargestUtilizationSpec.scope() = "host";
  minimizeNthLargestUtilizationSpec.dimension() = "cpu";
  minimizeNthLargestUtilizationSpec.n() = 0;
  solver_->addGoal(minimizeNthLargestUtilizationSpec);

  // enforce capacity
  facebook::rebalancer::interface::CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "cpu";
  solver_->addConstraint(capacitySpec);

  // configure local search solver
  solver_->addSolver(makeDefaultLocalSearchSolver());

  // solve the problem
  auto solution = solver_->solve();

  // aggregate task count by host
  map<string, int> taskCount;
  for (auto& [_, host] : *solution.assignment()) {
    ++taskCount[host];
  }

  // nothing moves
  EXPECT_EQ(2, taskCount["host0"]);
  EXPECT_EQ(0, taskCount["host1"]);
}

TEST_P(MinimizeNthLargestUtilizationTest, AllZeroCapacity) {
  // create a degenerate problem with 2 hosts and 0 tasks, where both hosts
  // have zer capacity

  // both hosts have zero capacity
  const map<string, double> hostCpu = {{"host0", 0}, {"host1", 0}};

  // all tasks take 1 cpu
  const pair<map<string, double>, double> taskCpuWithDefault = {{}, 1};

  // initial assignment
  map<string, vector<string>> initialAssignment = {
      {"host0", {}},
      {"host1", {}},
  };

  setUpTasksAndHosts(0, 2, taskCpuWithDefault, hostCpu, initialAssignment);

  // minimize the largest utilization
  facebook::rebalancer::interface::MinimizeNthLargestUtilizationSpec
      minimizeNthLargestUtilizationSpec;
  minimizeNthLargestUtilizationSpec.scope() = "host";
  minimizeNthLargestUtilizationSpec.dimension() = "cpu";
  minimizeNthLargestUtilizationSpec.n() = 0;
  solver_->addGoal(minimizeNthLargestUtilizationSpec);

  // configure local search solver
  solver_->addSolver(makeDefaultLocalSearchSolver());

  // solve the problem, ensure no crashes
  auto solution = solver_->solve();
}
