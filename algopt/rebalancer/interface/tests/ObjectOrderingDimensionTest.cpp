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

#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;

class ObjectOrderingDimensionTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    solver = initializeTestProblemSolver({.executorThreadCount = GetParam()});

    solver->setObjectName("task");
    solver->setContainerName("host");

    // initially, host0 has 7 tasks, host1 has 0 tasks.
    solver->setAssignment(
        std::vector<std::pair<std::string, std::vector<std::string>>>{
            {"host0",
             {"task0", "task1", "task2", "task3", "task4", "task5", "task6"}},
            {"host1", {}},
        });

    solver->addObjectDimension(
        "memory",
        folly::F14FastMap<std::string, double>{
            {"task0", 1},
            {"task1", 2},
            {"task2", 4},
            {"task3", 3},
            {"task4", 5},
            {"task5", 5},
            {"task6", 7},
        });

    // use local search solver with predictable behaviour; note that
    // minHotObjects = 1 and the moveType used is SINGLE_FAST. This means, any
    // time a beneficial move is found, it is immediately applied (as opposed to
    // trying several moves and picking the best)
    LocalSearchSolverSpec localSearchSolverSpec;
    localSearchSolverSpec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleFastMoveTypeSpec()));
    localSearchSolverSpec.minHotObjects() = 1;
    localSearchSolverSpec.objectOrderingDimension() = "potential";
    solver->addSolver(localSearchSolverSpec);
  }

  std::unique_ptr<ProblemSolver> solver;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    ObjectOrderingDimensionTest,
    testThreadCounts());

TEST_P(ObjectOrderingDimensionTest, ObjectOrderTestBasic) {
  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.scopeItemLimits() = {{"host0", 13}, {"host1", 14}};

  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  capacitySpec.limit() = limit;

  // add capacitySpec as a goal
  solver->addGoal(capacitySpec);

  // define object ordering dimension
  solver->addObjectDimension(
      "potential",
      folly::F14FastMap<std::string, double>{
          {"task0", 1},
          {"task1", 2},
          {"task2", 300},
          {"task3", 4},
          {"task4", 51},
          {"task5", 50},
          {"task6", 5},
      });

  // get a solution
  auto solution = solver->solve();

  // check initalGoalValue = (1 + 2 + 4 + 3 + 5 + 5 + 7) - 13 = 14.
  const double initialGoalValue =
      *solution.initialGlobalObjective()->goals()->at(0).value();
  EXPECT_NEAR(14, initialGoalValue, 1e-8);

  // Since objectOrderingDimension is set, the objects will be processed in
  // order given by the ordering dimension ("potential", define above). As a
  // result, it moves task2, task4, and task5 to host1. It does not make any
  // more moves since adding any more tasks would violate the limit of host1.
  const std::map<std::string, std::string> expected = {
      {"task0", "host0"},
      {"task1", "host0"},
      {"task2", "host1"},
      {"task3", "host0"},
      {"task4", "host1"},
      {"task5", "host1"},
      {"task6", "host0"},
  };
  EXPECT_EQ(expected, toOrderedMap(*solution.assignment()));

  //  check that goalValueWitObjectOrdering = 0, since w.r.t.
  // host0, we have, max(0, (1+2+3+7) - 13) = 0
  // host1, we have, max(0, (4+5+5) - 14)) = 0
  const double goalValueWithObjectOrdering =
      *solution.finalGlobalObjective()->goals()->at(0).value();
  EXPECT_NEAR(0, goalValueWithObjectOrdering, 1e-8);
}

TEST_P(ObjectOrderingDimensionTest, ObjectOrderTestWithDefaultValues) {
  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.scopeItemLimits() = {{"host0", 14}, {"host1", 15}};

  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  capacitySpec.limit() = limit;

  // add capacitySpec as a goal
  solver->addGoal(capacitySpec);

  // define object ordering dimension
  solver->addObjectDimension(
      "potential",
      folly::F14FastMap<std::string, double>{
          {"task0", 0},
          {"task1", 0},
          {"task2", 300},
          {"task3", 4},
          {"task4", 51},
          {"task5", 5},
      },
      10);

  // get a solution
  auto solution = solver->solve();

  // Since objectOrderingDimension is set, the objects will be processed in
  // order given by the ordering dimension ("potential", define above). As a
  // result, it moves task2, task4, and task6 to host1. Note that the default
  // value is 10, so task6 (with default value = 10) has the third highest value
  // among all the tasks.
  const std::map<std::string, std::string> expected = {
      {"task0", "host0"},
      {"task1", "host0"},
      {"task2", "host1"},
      {"task3", "host0"},
      {"task4", "host1"},
      {"task5", "host0"},
      {"task6", "host1"},
  };
  EXPECT_EQ(expected, toOrderedMap(*solution.assignment()));

  //  check that goalValueWitObjectOrdering = 0, since w.r.t.
  // host0, we have, max(0, (1+2+3+5) - 14) = 0
  // host1, we have, max(0, (4+5+7) - 15)) = 1
  const double goalValueWithObjectOrdering =
      *solution.finalGlobalObjective()->goals()->at(0).value();
  EXPECT_NEAR(1, goalValueWithObjectOrdering, 1e-8);
}

TEST_P(ObjectOrderingDimensionTest, ObjectOrderTestWithMultiValuedDimension) {
  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.scopeItemLimits() = {{"host0", 14}, {"host1", 15}};

  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  capacitySpec.limit() = limit;

  // add capacitySpec as a goal
  solver->addGoal(capacitySpec);

  // define object ordering dimension
  solver->addObjectDimension(
      "potential",
      folly::F14FastMap<std::string, std::vector<double>>{
          {"task0", {0, 0}},
          {"task1", {0, 0}},
          {"task2", {300, 0}},
          {"task3", {4, 0}},
          {"task4", {51, 0}},
          {"task5", {5, 0}},
      },
      10);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      solver->solve(),
      "objectOrderingDimension \"potential\" cannot be a dynamic dimension and should have only one value per object");
}
