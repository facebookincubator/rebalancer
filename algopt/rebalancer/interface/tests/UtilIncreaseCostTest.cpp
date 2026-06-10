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

#include <folly/container/irange.h>
#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

class UtilIncreaseCostTest : public ::testing::TestWithParam<int> {
 public:
  void SetUp() override {
    solver = initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver->setObjectName("task");
    solver->setContainerName("host");
  }

  void basicSetup() const {
    // host4 will be filtered out and never be used in the assignment
    solver->setAssignment(
        std::unordered_map<std::string, std::vector<std::string>>{
            {"host0", {"task0"}},
            {"host1", {"task1"}},
            {"host2", {"task2"}},
            {"host4", {}}});

    solver->addObjectDimension(
        "cpu",
        std::map<std::string, double>{
            {"task0", 2}, {"task1", 3}, {"task2", 5}});
    solver->addContainerDimension(
        "cpu",
        folly::F14FastMap<std::string, double>{
            {"host0", 10}, {"host1", 10}, {"host2", 10}, {"host4", 0}});

    solver->addSolver(makeDefaultLocalSearchSolver());

    ManifoldBackupParams params;
    params.uploadPolicy() = ManifoldUploadPolicy::NEVER;
    solver->setManifoldBackupParams(params);
  }
  std::unique_ptr<ProblemSolver> solver;
};

INSTANTIATE_TEST_CASE_P(NumThreads, UtilIncreaseCostTest, testThreadCounts());

TEST_P(UtilIncreaseCostTest, basic) {
  basicSetup();
  // If moves happen, balance utilization across scope by increasing the
  // utilization of scopeItems below the threshold
  UtilIncreaseCostSpec utilIncreaseCostSpec;
  utilIncreaseCostSpec.scope() = "host";
  utilIncreaseCostSpec.dimension() = "cpu";
  utilIncreaseCostSpec.lowerBound() = 0.4;
  utilIncreaseCostSpec.squares() = true;
  utilIncreaseCostSpec.filter()->itemsBlacklist() = {"host4"};
  solver->addGoal(utilIncreaseCostSpec);

  // free host0, keep host4 free
  ToFreeSpec toFreeSpec;
  toFreeSpec.containers() = {"host0", "host4"};
  solver->addConstraint(toFreeSpec);

  // initially: host0 and host 1 have utilization below 0.4
  // and host0 needs to be freed, so naturally task0 should move to host1
  auto solution = solver->solve();
  const std::map<std::string, std::string> expected_assignment = {
      {"task0", "host1"}, {"task1", "host1"}, {"task2", "host2"}};
  EXPECT_EQ(expected_assignment, toOrderedMap(*solution.assignment()));
  // objective = (sum(0) + sum(power(0.5) - power(0.4)) + sum(0)) * 0.33
  // each sum term corresponds to items host0, host1, host2
  // host4 should not show up as it should have been filtered
  EXPECT_NEAR(0.0338464, *solution.finalObjective()->value(), 1e-4);
}

TEST_P(UtilIncreaseCostTest, unchanged) {
  basicSetup();
  UtilIncreaseCostSpec utilIncreaseCostSpec;
  utilIncreaseCostSpec.scope() = "host";
  utilIncreaseCostSpec.dimension() = "cpu";
  utilIncreaseCostSpec.lowerBound() = 0.5;
  utilIncreaseCostSpec.squares() = true;
  utilIncreaseCostSpec.filter()->itemsBlacklist() = {"host4"};
  solver->addGoal(utilIncreaseCostSpec);

  // initially: host0 and host1 have utilization below 0.4
  // no moves needed, so the initial and final solutions must be same
  auto solution = solver->solve();
  EXPECT_EQ(
      toOrderedMap(*solution.initialAssignment()),
      toOrderedMap(*solution.assignment()));
  EXPECT_NEAR(0, *solution.finalObjective()->value(), 1e-4);
}

TEST_P(UtilIncreaseCostTest, randomPlacement) {
  // 3 hosts, 10 tasks, all tasks are in host0
  folly::F14FastMap<std::string, std::vector<std::string>> initial_assignment;
  for (const auto i : folly::irange(10)) {
    initial_assignment["host0"].push_back(fmt::format("task{}", i));
  }
  initial_assignment["host1"] = {};
  initial_assignment["host2"] = {};
  solver->setAssignment(initial_assignment);

  // all tasks have cpu = 1
  folly::F14FastMap<std::string, double> taskCPU;
  for (const auto i : folly::irange(10)) {
    taskCPU[fmt::format("task{}", i)] = 1;
  }
  solver->addObjectDimension("cpu", taskCPU);

  solver->addContainerDimension(
      "cpu",
      folly::F14FastMap<std::string, double>{
          {"host0", 100}, {"host1", 100}, {"host2", 100}});

  // Free host 0
  ToFreeSpec toFreeSpec;
  toFreeSpec.containers() = {"host0"};
  solver->addConstraint(toFreeSpec);

  UtilIncreaseCostSpec utilIncreaseCostSpec;
  utilIncreaseCostSpec.scope() = "host";
  utilIncreaseCostSpec.dimension() = "cpu";
  utilIncreaseCostSpec.lowerBound() = 0.5;
  utilIncreaseCostSpec.squares() = true;
  solver->addGoal(utilIncreaseCostSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  std::map<std::string, int> hostToTaskCount;
  for (auto& [_, hostName] : assignment) {
    ++hostToTaskCount[hostName];
  }

  EXPECT_EQ(0, hostToTaskCount["host0"]);

  // host1 and host2 have utilization below 0.5
  // because of this, the tasks should be randomly placed in host1 and host2
  EXPECT_EQ(10, hostToTaskCount["host1"] + hostToTaskCount["host2"]);
}

TEST_P(UtilIncreaseCostTest, AllAboveLowerbound) {
  // 3 hosts, 10 tasks, all tasks are in host0
  folly::F14FastMap<std::string, std::vector<std::string>> initial_assignment;
  for (const auto i : folly::irange(10)) {
    initial_assignment["host0"].push_back(fmt::format("task{}", i));
  }
  initial_assignment["host1"] = {};
  initial_assignment["host2"] = {};
  solver->setAssignment(initial_assignment);

  // all tasks have cpu = 1
  folly::F14FastMap<std::string, double> taskCPU;
  for (const auto i : folly::irange(10)) {
    taskCPU[fmt::format("task{}", i)] = 1;
  }
  solver->addObjectDimension("cpu", taskCPU);

  solver->addContainerDimension(
      "cpu",
      folly::F14FastMap<std::string, double>{
          {"host0", 100}, {"host1", 100000000}, {"host2", 100}});

  // Free host 0
  ToFreeSpec toFreeSpec;
  toFreeSpec.containers() = {"host0"};
  solver->addConstraint(toFreeSpec);

  UtilIncreaseCostSpec utilIncreaseCostSpec;
  utilIncreaseCostSpec.scope() = "host";
  utilIncreaseCostSpec.dimension() = "cpu";
  utilIncreaseCostSpec.lowerBound() = 0;
  utilIncreaseCostSpec.squares() = true;
  solver->addGoal(utilIncreaseCostSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  std::map<std::string, int> hostToTaskCount;
  for (auto& [_, hostName] : assignment) {
    ++hostToTaskCount[hostName];
  }

  EXPECT_EQ(0, hostToTaskCount["host0"]);
  EXPECT_EQ(10, hostToTaskCount["host1"]);
  EXPECT_EQ(0, hostToTaskCount["host2"]);
}
