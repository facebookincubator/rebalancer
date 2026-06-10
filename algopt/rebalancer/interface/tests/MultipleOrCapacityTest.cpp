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

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

struct CapacityRange {
  double lb;
  double ub;
};

class MultipleOrCapacityTest : public ::testing::TestWithParam<int> {
 protected:
  void addCapacityConstraintsForDisjointRanges(
      std::vector<CapacityRange> ranges,
      std::string host);
  static CapacitySpec
  getCapacitySpec(double bound, CapacitySpecBound type, std::string scopeItem);

  void SetUp() override;

  std::unique_ptr<ProblemSolver> solver_;
};

CapacitySpec MultipleOrCapacityTest::getCapacitySpec(
    double bound,
    CapacitySpecBound type,
    std::string scopeItem) {
  // if no dimension is specified, it is assumed to be "task_count"
  CapacitySpec spec;
  spec.scope() = "host";
  spec.bound() = type;
  spec.limit()->globalLimit() = bound;
  spec.filter()->itemsWhitelist() = {scopeItem};
  spec.name() = fmt::format(
      "capacity_{}_{}", type == CapacitySpecBound::MIN ? ">=" : "<=", bound);
  return spec;
}

void MultipleOrCapacityTest::addCapacityConstraintsForDisjointRanges(
    std::vector<CapacityRange> ranges,
    std::string host) {
  std::vector<double> breakpoints;
  for (auto r : ranges) {
    if (!breakpoints.empty() && (breakpoints.back() > r.lb || r.lb > r.ub)) {
      throw std::runtime_error("intervals are not well-formed");
    }
    breakpoints.push_back(r.lb);
    breakpoints.push_back(r.ub);
  }
  /*
  We want to model that util of @param host lies within one of the provided
  disjoint ranges: [a1, a2] [a3, a4] [a5, a6] where a_i are increasing

  This can be done as follows:
   1. Ensure that util >= a1  CapacitySpec with MIN bound
   2. Ensure that util <= a6  CapacitySpec with MAX bound
   3. Ensure that util NOT in (a2, a6) AND NOT in (a4, a5)
     Can be modeled as:
          [util <= a2 OR util >= a6]  => MultipleOrCapacitySpec
      AND [util <= a4 OR util >= a5]  => MultipleOrCapacitySpec
  */
  solver_->addConstraint(
      getCapacitySpec(breakpoints.at(0), CapacitySpecBound::MIN, host));
  auto lastIdx = breakpoints.size() - 1;
  for (size_t i = 1; i < lastIdx - 1; i += 2) {
    MultipleOrCapacitySpec spec;
    spec.name() = fmt::format(
        "capacity <= {} OR >= {}", breakpoints.at(i), breakpoints.at(i + 1));
    spec.capacitySpecs()->push_back(
        getCapacitySpec(breakpoints.at(i), CapacitySpecBound::MAX, host));
    spec.capacitySpecs()->push_back(
        getCapacitySpec(breakpoints.at(i + 1), CapacitySpecBound::MIN, host));
    solver_->addConstraint(spec);
  }
  solver_->addConstraint(
      getCapacitySpec(breakpoints.at(lastIdx), CapacitySpecBound::MAX, host));
}

void MultipleOrCapacityTest::SetUp() {
  solver_ = initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver_->setObjectName("task");
  solver_->setContainerName("host");

  // Total 12 tasks, hosts have 6 tasks each
  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2", "task3", "task4", "task5"}},
          {"host1", {"task6", "task7", "task8", "task9", "task10", "task11"}},
      });
}

INSTANTIATE_TEST_CASE_P(
    ThreadCounts,
    MultipleOrCapacityTest,
    testThreadCounts());

TEST_P(MultipleOrCapacityTest, SingleInterval) {
  MultipleOrCapacitySpec spec;
  spec.name() = fmt::format("capacity <= {} OR >= {}", 5, 7);
  spec.capacitySpecs()->push_back(
      getCapacitySpec(5, CapacitySpecBound::MAX, "host0"));
  spec.capacitySpecs()->push_back(
      getCapacitySpec(7, CapacitySpecBound::MIN, "host0"));
  solver_->addConstraint(spec);
  solver_->addSolver(makeDefaultLocalSearchSolver());
  auto solution = solver_->solve();
  std::map<std::string, int> tasksInHost;
  for (auto& [_task, host] : *solution.assignment()) {
    ++tasksInHost[host];
  }
  EXPECT_NE(6, tasksInHost["host0"]);
  EXPECT_EQ(12, tasksInHost["host0"] + tasksInHost["host1"]);
}

TEST_P(MultipleOrCapacityTest, MultipleRangesOne) {
  // however, host0 are only allowed to have following range of tasks:
  //  [2, 4] U [7 8] U [10 11]
  addCapacityConstraintsForDisjointRanges(
      {CapacityRange{.lb = 2, .ub = 4},
       CapacityRange{.lb = 7, .ub = 8},
       CapacityRange{.lb = 10, .ub = 12}},
      "host0");
  // Moreover we want to force the solver to choose the first interval [2, 4]
  ToFreeSpec freeHost0;
  freeHost0.containers() = {"host0"};
  solver_->addGoal(freeHost0);
  // add additional capacity constraint forcing min allocation to be

  solver_->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();

  // We expect each host to contain exactly 2 tasks in the final assignment.
  std::map<std::string, int> tasksInHost;
  for (auto& [_task, host] : *solution.assignment()) {
    ++tasksInHost[host];
  }
  EXPECT_EQ(2, tasksInHost["host0"]);
  EXPECT_EQ(10, tasksInHost["host1"]);
}

TEST_P(MultipleOrCapacityTest, MultipleRangesTwo) {
  // however, host0 are only allowed to have following range of tasks:
  //  [0, 4] U [7 8] U [10 11]
  addCapacityConstraintsForDisjointRanges(
      {CapacityRange{.lb = 0, .ub = 4},
       CapacityRange{.lb = 7, .ub = 8},
       CapacityRange{.lb = 10, .ub = 12}},
      "host0");
  // Moreover we want to force the solver to choose the second interval [7, 8]
  MinimizeMovementSpec spec;
  spec.scope() = "host";
  spec.dimension() = "task_count";
  solver_->addGoal(spec);
  solver_->addSolver(makeDefaultLocalSearchSolver());
  // Get a solution from Rebalancer.
  auto solution = solver_->solve();

  // We expect each host to contain exactly 2 tasks in the final assignment.
  std::map<std::string, int> tasksInHost;
  for (auto& [_task, host] : *solution.assignment()) {
    ++tasksInHost[host];
  }
  EXPECT_EQ(7, tasksInHost["host0"]);
  EXPECT_EQ(5, tasksInHost["host1"]);
}

TEST_P(MultipleOrCapacityTest, MultipleRangesThree) {
  // however, host0 are only allowed to have following range of tasks:
  //  [0, 4] U [7 8] U [10 11]
  addCapacityConstraintsForDisjointRanges(
      {CapacityRange{.lb = 0, .ub = 4},
       CapacityRange{.lb = 7, .ub = 8},
       CapacityRange{.lb = 10, .ub = 12}},
      "host0");
  // Moreover we want to force the solver to choose the third interval [10, 11]
  solver_->addConstraint(getCapacitySpec(9, CapacitySpecBound::MIN, "host0"));
  // Use optimal solver as local search with default move types can't find an
  // optimal solution
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  solver_->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());
  // Get a solution from Rebalancer.
  auto solution = solver_->solve();

  // We expect each host to contain exactly 2 tasks in the final assignment.
  std::map<std::string, int> tasksInHost;
  for (auto& [_task, host] : *solution.assignment()) {
    ++tasksInHost[host];
  }

  EXPECT_EQ(0, *solution.finalObjective()->value());
  EXPECT_GE(tasksInHost["host0"], 10);
  EXPECT_LE(tasksInHost["host1"], 2);
}
