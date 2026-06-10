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

#include "algopt/rebalancer/interface/tests/GroupCountCustomDimensionBase.h"
#include "algopt/rebalancer/interface/tests/utils.h"

#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;

class GroupCountCustomDimensionTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    GroupCountCustomDimensionTest,
    testThreadCounts());

TEST_P(GroupCountCustomDimensionTest, SmallAbsolute) {
  // Create a small non-trivial problem that's likely to hit all codepaths and
  // cover most corner cases.
  auto solver = buildProblem(30, 10, 10, true, GetParam());
  LocalSearchSolverSpec solverSpec = makeDefaultLocalSearchSolver();
  solverSpec.stopAfterMoves() = 0;
  solver->addSolver(solverSpec);
  auto solution = solver->solve();

  // Assert that the only goal evaluates to the already known pre-computed value
  // when evaluated on the initial assignment. If the internal implementation
  // changes the value should remain unchanged. If the value changes, that's
  // considered a bug and this test will catch it.
  EXPECT_NEAR(
      149.0, *solution.initialGlobalObjective()->goals()->at(0).value(), 1e-8);
}

TEST_P(GroupCountCustomDimensionTest, LargeAbsolute) {
  // Create a large non-trivial problem that's likely to hit all codepaths,
  // cover most corner cases and catch precision errors.
  auto solver = buildProblem(500, 100, 100, true, GetParam());
  auto solverSpec = makeDefaultLocalSearchSolver();
  solverSpec.stopAfterMoves() = 0;
  solver->addSolver(solverSpec);
  auto solution = solver->solve();

  // Assert that the only goal evaluates to the already known pre-computed value
  // when evaluated on the initial assignment. If the internal implementation
  // changes the value should remain unchanged. If the value changes, that's
  // considered a bug and this test will catch it.
  EXPECT_NEAR(
      2476.7398797595188,
      *solution.initialGlobalObjective()->goals()->at(0).value(),
      1e-8);
}

TEST_P(GroupCountCustomDimensionTest, SmallRelative) {
  auto solver = buildProblem(30, 10, 10, false, GetParam());
  auto solverSpec = makeDefaultLocalSearchSolver();
  solverSpec.stopAfterMoves() = 0;
  solver->addSolver(solverSpec);
  auto solution = solver->solve();

  // Explanation: https://fburl.com/eeeq30z2
  EXPECT_NEAR(
      135.0, *solution.initialGlobalObjective()->goals()->at(0).value(), 1e-8);
}

TEST_P(GroupCountCustomDimensionTest, LargeRelative) {
  auto solver = buildProblem(500, 100, 100, false, GetParam());
  auto solverSpec = makeDefaultLocalSearchSolver();
  solverSpec.stopAfterMoves() = 0;
  solver->addSolver(solverSpec);
  auto solution = solver->solve();

  EXPECT_NEAR(
      1918.3266533066133,
      *solution.initialGlobalObjective()->goals()->at(0).value(),
      1e-8);
}

TEST_P(GroupCountCustomDimensionTest, FixedObjects) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}}, {"host1", {"task2"}}});
  solver->addPartition(
      "job",
      std::map<std::string, std::string>{
          {"task0", "job0"}, {"task1", "job1"}, {"task2", "job0"}});
  solver->addObjectDimension(
      "cpu",
      std::vector<std::pair<std::string, double>>{
          {"task0", 10}, {"task1", 20}, {"task2", 30}});

  {
    auto spec = GroupCountSpec();
    spec.scope() = "host";
    spec.partitionName() = "job";
    spec.dimension() = "cpu";
    spec.limit()->type() = LimitType::ABSOLUTE;
    spec.limit()->globalLimit() = 7;

    solver->addGoal(spec);
  }

  {
    auto spec = AvoidMovingSpec();
    spec.objects() = {"task0"};

    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();

  EXPECT_NEAR(
      39.0, *solution.initialGlobalObjective()->goals()->at(0).value(), 1e-8);
}
