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
#include <folly/Random.h>
#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace facebook::rebalancer::interface;

static int runExperiment(int threadCount) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = threadCount});
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      map<string, vector<string>>(
          {{"h0", {"t0"}}, {"h1", {"t1"}}, {"h2", {}}}));
  solver->addScope(
      "region",
      map<string, string>({{"h0", "r0"}, {"h1", "r0"}, {"h2", "r1"}}));

  facebook::rebalancer::interface::BalanceSpec balanceSpec;
  balanceSpec.scope() = "region";
  balanceSpec.dimension() = "task_count";
  solver->addGoal(balanceSpec);

  // set a different random seed per experiment
  LocalSearchSolverSpec localSearchSolverSpec = makeDefaultLocalSearchSolver();
  localSearchSolverSpec.randomSeed() =
      static_cast<int>(folly::Random::rand32());
  solver->addSolver(localSearchSolverSpec);

  auto solution = solver->solve();
  const bool move0 = solution.assignment()->at("t0") == "h2";
  const bool move1 = solution.assignment()->at("t1") == "h2";

  // exactly one of t0 and t1 must move to h2 to achieve perfect balance
  EXPECT_TRUE(move0 || move1);
  EXPECT_TRUE(!move0 || !move1);

  // return which of t0 and t1 was the lucky one
  return move0 ? 0 : 1;
}

class MoveRandomizationTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(
    ThreadCounts,
    MoveRandomizationTest,
    testThreadCounts());

TEST_P(MoveRandomizationTest, StatisticalAnalysis) {
  int t1Moves = 0;
  for (const auto _ : folly::irange(100)) {
    t1Moves += runExperiment(GetParam());
  }
  // with 100 samples and a uniform distribution, the chances of t1 moving
  // less than 20 or more than 80 times are less than 1 in a million
  EXPECT_LT(20, t1Moves);
  EXPECT_GT(80, t1Moves);
}
