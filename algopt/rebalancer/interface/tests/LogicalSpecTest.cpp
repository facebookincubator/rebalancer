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

using namespace facebook::rebalancer::interface;

class LogicalSpecTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    REBALANCER_SKIP_IF_NO_MIP_SOLVER();
    // Our universe is created with "taskX" objects and "hostY" containers.
    solver = initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver->addSolver(
        facebook::algopt::makeAvailableOptimalSolverSpec()); // Use the optimal
                                                             // solver.
    solver->setObjectName("task");
    solver->setContainerName("host");

    // host0 initially has 4 tasks in it, and host1 has none.
    solver->setAssignment(
        std::vector<std::pair<std::string, std::vector<std::string>>>{
            {"host0", {"task0", "task1", "task2", "task3"}},
            {"host1", {}},
        });
  }

  static CapacitySpec makeCapacitySpec(
      CapacitySpecBound bound,
      double limit,
      std::optional<std::string> hostName = std::nullopt) {
    // if no dimension is specified, it is assumed to be "task_count"
    CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.definition() = CapacitySpecDefinition::AFTER;
    capacitySpec.limit()->type() = LimitType::ABSOLUTE;
    capacitySpec.bound() = bound;
    capacitySpec.limit()->globalLimit() = limit;
    if (hostName) {
      capacitySpec.filter()->itemsWhitelist() = {*hostName};
    }
    return capacitySpec;
  }

  static void addCapSpecToAndSpec(
      LogicalAndSpec& logicalAndSpec,
      const CapacitySpec& capacitySpec) {
    GenericSpec genericSpec;
    genericSpec.set_capacitySpec(capacitySpec);
    logicalAndSpec.genericSpecs()->push_back(genericSpec);
  }

  static void addAndSpecToOrSpec(
      LogicalOrSpec& logicalOrSpec,
      const LogicalAndSpec& logicalAndSpec) {
    GenericSpec genericSpec;
    genericSpec.set_logicalAndSpec(logicalAndSpec);
    logicalOrSpec.genericSpecs()->push_back(genericSpec);
  }

  void solveAndSummarise() {
    solution = solver->solve();
    tasksPerHost.clear();
    for (auto& [_task, host] : *solution.assignment()) {
      ++tasksPerHost[host];
    }
  }

  void checkSatisfied(bool satisfiable) {
    auto fc = *solution.finalConstraint();
    EXPECT_TRUE(fc.solved().value());
    if (satisfiable) {
      EXPECT_EQ(fc.brokenCount().value(), 0);
    } else {
      EXPECT_GT(fc.brokenCount().value(), 0);
    }
  }

  void checkTasksPerHost(const std::map<std::string, unsigned>& expected) {
    EXPECT_GE(expected.size(), tasksPerHost.size());
    for (const auto& [host, count] : expected) {
      unsigned got_count = 0;
      const auto& it = tasksPerHost.find(host);
      if (it != tasksPerHost.end()) {
        got_count = it->second;
      }
      EXPECT_EQ(count, got_count);
    }
  }

 protected:
  std::unique_ptr<ProblemSolver> solver;
  AssignmentSolution solution;
  std::map<std::string, unsigned> tasksPerHost;
};

INSTANTIATE_TEST_CASE_P(NumThreads, LogicalSpecTest, testThreadCounts());

TEST_P(LogicalSpecTest, AndWithMin) {
  // We want at least:
  // 1 task  on host0
  // 3 tasks on host1
  LogicalAndSpec las;
  addCapSpecToAndSpec(
      las, makeCapacitySpec(CapacitySpecBound::MIN, 1, "host0"));
  addCapSpecToAndSpec(
      las, makeCapacitySpec(CapacitySpecBound::MIN, 3, "host1"));
  solver->addConstraint(las);

  solveAndSummarise();
  checkSatisfied(true);
  checkTasksPerHost({{"host0", 1}, {"host1", 3}});
}

TEST_P(LogicalSpecTest, OrOfAnds_FirstBranch) {
  // We want:
  // host0 to contain at least 1 task  and at most 4 tasks
  // host1 to contain at least 9 tasks and at most 9 tasks -- impossible
  LogicalOrSpec los;
  {
    LogicalAndSpec las;
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MIN, 1, "host0"));
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MAX, 4, "host0"));
    addAndSpecToOrSpec(los, las);
  }
  {
    LogicalAndSpec las;
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MIN, 9, "host1"));
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MAX, 9, "host1"));
    addAndSpecToOrSpec(los, las);
  }
  solver->addConstraint(los);

  solveAndSummarise();
  checkSatisfied(true);
  checkTasksPerHost({{"host0", 4}, {"host1", 0}});
}

TEST_P(LogicalSpecTest, OrOfAnds_SecondBranch) {
  // We want:
  // host0 to contain at least 9 tasks and at most 9 tasks -- impossible
  // host1 to contain at least 1 tasks and at most 4 tasks
  LogicalOrSpec los;
  {
    LogicalAndSpec las;
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MIN, 9, "host0"));
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MAX, 9, "host0"));
    addAndSpecToOrSpec(los, las);
  }
  {
    LogicalAndSpec las;
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MAX, 0, "host0"));
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MIN, 1, "host1"));
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MAX, 4, "host1"));
    addAndSpecToOrSpec(los, las);
  }
  solver->addConstraint(los);

  solveAndSummarise();
  checkSatisfied(true);
  checkTasksPerHost({{"host0", 0}, {"host1", 4}});
}

TEST_P(LogicalSpecTest, OrOfAnds_NoBranch) {
  // We want:
  // host0 to contain at least 9 tasks and at most 9 tasks -- impossible
  // host1 to contain at least 9 tasks and at most 9 tasks -- impossible
  LogicalOrSpec los;
  {
    LogicalAndSpec las;
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MIN, 9, "host0"));
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MAX, 9, "host0"));
    addAndSpecToOrSpec(los, las);
  }
  {
    LogicalAndSpec las;
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MIN, 9, "host1"));
    addCapSpecToAndSpec(
        las, makeCapacitySpec(CapacitySpecBound::MAX, 9, "host1"));
    addAndSpecToOrSpec(los, las);
  }
  solver->addConstraint(los);

  solveAndSummarise();
  checkSatisfied(false);
}
