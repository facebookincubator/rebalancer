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
#include <algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h>

#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;

class SingleRandomStratifiedTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    SingleRandomStratifiedTest,
    testThreadCounts());

TEST_P(SingleRandomStratifiedTest, EnableRestrictMovingObjectOnlyOnce) {
  // This test evaluates that when enableRestrictMovingObjectOnlyOnce is set, an
  // object is only moves once. It does so by forcing the move type to move the
  // object to host0 in the first stage. The second stage then is incentivized
  // to move from host0 to host1 for improving balance, but should not be able
  // to do so because of enableRestrictMovingObjectOnlyOnce.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"unassigned", {"task0"}}, {"host0", {"task1"}}, {"host1", {}}});

  solver->addObjectDimension(
      "cpu", std::map<std::string, double>{{"task0", 10}, {"task1", 10}});
  solver->addContainerDimension(
      "cpu", std::map<std::string, double>{{"host0", 100}, {"host1", 100}});
  solver->enableRestrictMovingObjectOnlyOnce();

  // Stage 0: ToFreeSpec
  {
    ToFreeSpec spec;
    spec.containers() = {"unassigned"};
    solver->addConstraint(spec);
  }

  {
    AvoidMovingSpec spec;
    spec.objects() = {"task1"};
    solver->addConstraint(spec);
  }
  solver->addGoalBoundary();

  // Stage 1: BalanceSpec
  {
    BalanceSpec spec;
    spec.scope() = "host";
    spec.dimension() = "cpu";
    spec.formula() = BalanceSpecFormula::SQUARES;
    spec.filter()->itemsBlacklist() = {"unassigned"};
    solver->addGoal(spec);
  }

  {
    LocalSearchStageSolverSpec spec;
    {
      ScopeItemList list;
      list.scopeName() = "host";
      list.scopeItems() = {"host0"};

      MoveToScopeItemsSpec moveToScopeItemsSpec;
      moveToScopeItemsSpec.defaultScopeItems() = list;

      DestinationsToExploreOptions destinationsToExplore;
      destinationsToExplore.set_moveToScopeItems() = moveToScopeItemsSpec;

      SingleRandomStratifiedMoveTypeSpec moveSpec;
      moveSpec.destinationsToExplore() = destinationsToExplore;
      moveSpec.stratifiedSampleSize()->defaultSampleSize() = 2;

      LocalSearchSolverSpec allocSpec;
      allocSpec.moveTypeList()->push_back(
          ProblemSolver::makeMoveTypeSpec(moveSpec));

      LocalSearchStageSpec allocStageSpec;
      allocStageSpec.begin() = 0;
      allocStageSpec.end() = *allocStageSpec.begin() + 1;
      allocStageSpec.solverSpec() = allocSpec;
      spec.stageSpecs()->push_back(allocStageSpec);
    }

    {
      ScopeItemList list;
      list.scopeName() = "host";
      list.scopeItems() = {"host0", "host1"};

      MoveToScopeItemsSpec moveToScopeItemsSpec;
      moveToScopeItemsSpec.defaultScopeItems() = list;

      DestinationsToExploreOptions destinationsToExplore;
      destinationsToExplore.set_moveToScopeItems() = moveToScopeItemsSpec;

      SingleRandomStratifiedMoveTypeSpec moveSpec;
      moveSpec.destinationsToExplore() = destinationsToExplore;
      moveSpec.stratifiedSampleSize()->defaultSampleSize() = 2;

      LocalSearchSolverSpec balanceSpec;
      balanceSpec.moveTypeList()->push_back(
          ProblemSolver::makeMoveTypeSpec(moveSpec));

      LocalSearchStageSpec balanceStageSpec;
      balanceStageSpec.begin() = 1;
      balanceStageSpec.end() = *balanceStageSpec.begin() + 1;
      balanceStageSpec.solverSpec() = balanceSpec;
      spec.stageSpecs()->push_back(balanceStageSpec);
    }

    solver->addSolver(spec);
  }

  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  std::map<std::string, int> hostToTaskCount;
  for (auto& [task, host] : assignment) {
    ++hostToTaskCount[host];
  }

  EXPECT_EQ(0, hostToTaskCount["unassigned"]);
  EXPECT_EQ(2, hostToTaskCount["host0"]);
  EXPECT_EQ(0, hostToTaskCount["host1"]);
}
