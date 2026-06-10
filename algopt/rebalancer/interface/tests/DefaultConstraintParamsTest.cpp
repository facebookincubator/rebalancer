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

#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::rebalancer::interface;

class DefaultConstraintParamsTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    DefaultConstraintParamsTest,
    testThreadCounts());

static AssignmentSolution runSolver(ConstraintPolicy policy, int threadCount) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = threadCount});
  solver->setConstraintPolicy(policy);
  {
    ConstraintParams params;
    params.invalidCost() = 10;
    params.invalidState() = 100;
    solver->setDefaultConstraintParams(params);
  }
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0"}}});
  {
    CapacitySpec spec;
    spec.scope() = "host";
    spec.dimension() = "task_count";
    spec.limit()->globalLimit() = 0;
    solver->addConstraint(spec);
  }
  solver->addSolver(makeDefaultLocalSearchSolver());
  return solver->solve();
}

TEST_P(DefaultConstraintParamsTest, DefaultPolicy) {
  auto solution = runSolver(ConstraintPolicy::DEFAULT, GetParam());
  EXPECT_NEAR(110.0, *solution.finalObjective()->value(), 1e-8);
  EXPECT_EQ(1, solution.finalObjective()->objs()->size());
  EXPECT_NEAR(1.0, *solution.finalConstraint()->brokenVal(), 1e-8);
  EXPECT_EQ(1, solution.finalConstraint()->constraints()->size());
  EXPECT_NEAR(
      1.0, *solution.finalConstraint()->constraints()->at(0).value(), 1e-8);
}

TEST_P(DefaultConstraintParamsTest, HardPolicy) {
  auto solution = runSolver(ConstraintPolicy::HARD, GetParam());
  EXPECT_NEAR(0.0, *solution.finalObjective()->value(), 1e-8);
  EXPECT_EQ(0, solution.finalObjective()->objs()->size());
  EXPECT_NEAR(1.0, *solution.finalConstraint()->brokenVal(), 1e-8);
  EXPECT_EQ(1, solution.finalConstraint()->constraints()->size());
  EXPECT_NEAR(
      1.0, *solution.finalConstraint()->constraints()->at(0).value(), 1e-8);
}
