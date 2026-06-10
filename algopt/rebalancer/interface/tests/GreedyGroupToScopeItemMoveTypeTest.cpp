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
#include <algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h>

#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;

class GreedyGroupToScopeItemTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    solver_ = initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver_->setObjectName("task");
    solver_->setContainerName("host");
    solver_->setAssignment(
        folly::F14FastMap<std::string, std::vector<std::string>>{
            {"unassigned", {"task0", "task1", "task2", "task3"}},
            {"host1", {"task4"}},
            {"host2", {}},
            {"host3", {}}});

    solver_->addPartition(
        "job",
        std::map<std::string, std::vector<std::string>>({
            {"job0", {"task0", "task1", "task3"}},
        }));

    solver_->addScope(
        "assignable",
        std::map<std::string, std::string>({
            {"host1", "assignable1"},
            {"host2", "assignable1"},
            {"host3", "assignable1"},
        }));
  }

  std::unique_ptr<ProblemSolver> solver_;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    GreedyGroupToScopeItemTest,
    testThreadCounts());

TEST_P(GreedyGroupToScopeItemTest, Basic) {
  // problem is setup in SetUp()
  {
    ToFreeSpec spec;
    spec.containers() = {"unassigned"};
    solver_->addConstraint(spec);
  }

  {
    // NOTE: GreedyGroupToScopeItem forces every object in the given group to a
    // *unique* container in the scopeItem.
    LocalSearchSolverSpec spec;
    GreedyGroupToScopeItemMoveTypeSpec moveTypeSpec;
    moveTypeSpec.scopeItemMovesScope() = "assignable";
    moveTypeSpec.groupMovesPartition() = "job";
    spec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(std::move(moveTypeSpec)));

    solver_->addSolver(spec);
  }

  auto solution = solver_->solve();
  auto& assignment = *solution.assignment();

  // Expect tasks 0, 1, and 3 to move to one of host1/host2/host3, so they
  // cannot be in 'unassigned'
  EXPECT_NE(assignment["task0"], "unassigned");
  EXPECT_NE(assignment["task1"], "unassigned");
  EXPECT_NE(assignment["task3"], "unassigned");
}

TEST_P(GreedyGroupToScopeItemTest, ThrowIfMoveParamsNotSet) {
  // problem is setup in SetUp()
  {
    LocalSearchSolverSpec spec;
    spec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(GreedyGroupToScopeItemMoveTypeSpec()));
    solver_->addSolver(spec);
  }

  REBALANCER_EXPECT_RUNTIME_ERROR(
      solver_->solve(),
      "GreedyGroupToScopeItemMoveType requires the parameter 'groupMovesPartition' and 'scopeItemMovesScope' to be set");
}
