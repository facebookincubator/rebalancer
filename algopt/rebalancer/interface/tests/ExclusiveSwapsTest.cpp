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

#include <fmt/core.h>
#include <gtest/gtest.h>

#include <map>

using namespace std;
using namespace facebook::rebalancer::interface;

using SubsetDefinition = ExclusiveSwapsSpecSubsetDefinition;

class ExclusiveSwapsTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, ExclusiveSwapsTest, testThreadCounts());

class BalanceRatiosTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, BalanceRatiosTest, testThreadCounts());

static void createProblem(
    ProblemSolver& solver,
    SubsetDefinition subsetDefinition) {
  solver.setObjectName("object");
  solver.setContainerName("container");

  const map<string, vector<string>> initial_assignment = {
      {"c1", {"o2", "o3"}}, {"c2", {"o1"}}, {"c3", {}}};
  solver.setAssignment(initial_assignment);

  // Object o1 prefers container c1
  // Object o2 prefers container c2
  // Object o3 prefers container c3
  AssignmentAffinitiesSpec assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.affinities() = {
      makeAssignmentAffinity("o1", "c1", 1.0),
      makeAssignmentAffinity("o2", "c2", 1.0),
      makeAssignmentAffinity("o3", "c3", 1.0),
  };
  assignmentAffinitiesSpec.scope() = "container";
  solver.addGoal(assignmentAffinitiesSpec);

  ExclusiveSwapsSpec exclusiveSwapsSpec;
  exclusiveSwapsSpec.subsetObjects() = {"o1", "o2"};
  exclusiveSwapsSpec.subsetDefinition() = subsetDefinition;
  solver.addConstraint(exclusiveSwapsSpec);
}

static void verifySolution(
    const AssignmentSolution& solution,
    SubsetDefinition subsetDefinition) {
  if (subsetDefinition == SubsetDefinition::AT_LEAST_ONE_IN_SUBSET) {
    EXPECT_EQ("c1", solution.assignment()->at("o1"));
    EXPECT_EQ("c2", solution.assignment()->at("o2"));
    EXPECT_EQ("c1", solution.assignment()->at("o3"));
  } else if (subsetDefinition == SubsetDefinition::EXACTLY_ONE_IN_SUBSET) {
    EXPECT_EQ("c1", solution.assignment()->at("o1"));
    EXPECT_EQ("c1", solution.assignment()->at("o2"));
    EXPECT_EQ("c2", solution.assignment()->at("o3"));
  } else if (subsetDefinition == SubsetDefinition::BOTH_SAME_SIDE_OF_SUBSET) {
    EXPECT_EQ("c1", solution.assignment()->at("o1"));
    EXPECT_EQ("c2", solution.assignment()->at("o2"));
    EXPECT_EQ("c1", solution.assignment()->at("o3"));
  } else {
    throw std::runtime_error(
        fmt::format(
            "unknown swaps subset definition {}",
            fmt::underlying(subsetDefinition)));
  }
}

static void testLocalSearch(
    SubsetDefinition subsetDefinition,
    int threadCount) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = threadCount});
  createProblem(*solver, subsetDefinition);
  const LocalSearchSolverSpec localSearchSpec = makeDefaultLocalSearchSolver();
  solver->addSolver(localSearchSpec);
  auto solution = solver->solve();
  verifySolution(solution, subsetDefinition);
}

static void testOptimal(SubsetDefinition subsetDefinition, int threadCount) {
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = threadCount});
  createProblem(*solver, subsetDefinition);
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());
  auto solution = solver->solve();
  verifySolution(solution, subsetDefinition);
}

TEST_P(ExclusiveSwapsTest, Basic) {
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("host");
  solver->setContainerName("rack");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"atn2.08.nc", {"cache0017.08.atn2"}},
          {"atn2.08.nd", {}},
          {"atn2.08.ne", {}},
      });
  solver->addConstraint(ExclusiveSwapsSpec());
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());
  solver->solve();
}

TEST_P(BalanceRatiosTest, LocalSearchAtLeastOne) {
  testLocalSearch(SubsetDefinition::AT_LEAST_ONE_IN_SUBSET, GetParam());
}

TEST_P(BalanceRatiosTest, LocalSearchExactlyOne) {
  testLocalSearch(SubsetDefinition::EXACTLY_ONE_IN_SUBSET, GetParam());
}

TEST_P(BalanceRatiosTest, LocalSearchBothSameSideOrSubset) {
  testLocalSearch(SubsetDefinition::BOTH_SAME_SIDE_OF_SUBSET, GetParam());
}

TEST_P(BalanceRatiosTest, OptimalAtLeastOne) {
  testOptimal(SubsetDefinition::AT_LEAST_ONE_IN_SUBSET, GetParam());
}

TEST_P(BalanceRatiosTest, OptimalExactlyOne) {
  testOptimal(SubsetDefinition::EXACTLY_ONE_IN_SUBSET, GetParam());
}

TEST_P(BalanceRatiosTest, OptimalBothSameSideOrSubset) {
  testOptimal(SubsetDefinition::BOTH_SAME_SIDE_OF_SUBSET, GetParam());
}

TEST_P(ExclusiveSwapsTest, Drain) {
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("host");
  solver->setContainerName("rack");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"c1", {"o2"}},
          {"c2", {"o1"}},
      });
  solver->addObjectDimension(
      "drain",
      std::vector<std::pair<std::string, double>>{{"o2", 1}, {"o1", 0}});

  ExclusiveSwapsSpec exclusiveSwapsSpec;
  exclusiveSwapsSpec.subsetObjects() = {{"o2"}};
  solver->addConstraint(exclusiveSwapsSpec);

  CapacitySpec drainSpec;
  drainSpec.scope() = "rack";
  drainSpec.dimension() = "drain";

  Limit limit;
  limit.globalLimit() = 0;

  Filter filter;
  filter.itemsWhitelist() = {"c1"};

  drainSpec.limit() = limit;
  drainSpec.filter() = filter;

  solver->addConstraint(drainSpec);
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());
  auto solution = solver->solve();
  EXPECT_EQ("c1", solution.assignment()->at("o1"));
  EXPECT_EQ("c2", solution.assignment()->at("o2"));
}
