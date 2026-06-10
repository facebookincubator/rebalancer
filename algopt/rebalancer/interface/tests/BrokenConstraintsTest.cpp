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

#include <folly/container/irange.h>
#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

class BrokenConstraintsTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    solver_ = initializeTestProblemSolver({.executorThreadCount = GetParam()});
  }

  void setInitialAssignment() {
    solver_->setObjectName("shard");
    solver_->setContainerName("host");

    const std::map<std::string, std::vector<std::string>> initial_assignment = {
        {"h0", {"s1", "s2", "s3", "s4", "s5"}},
        {"h1", {"s6", "s7"}},
        {"h2", {}},
        {"h3", {}},
    };
    solver_->setAssignment(initial_assignment);
  }

  void makeSimpleProblem() {
    // Create a simple problem with 4 hosts and 7 shards, where host h0 has 5
    // shards, host h1 has 2 shards. There are 2 constraints. First is to drain
    // h0, and the second is to drain h1. However, the second constraint is of
    // lesser priority than the first one, and in particular, if broken, the
    // broken component of the second constraint is added to goal tuple index 2
    // (instead of 0, which is the default). There is also goal at tuple index 1
    // to maximize the number of shards assigned to h3. The fact that it is at
    // goal tuple index 1 implies that minimizing this goal is considered to be
    // more important than "fixing" the second constraint.

    setInitialAssignment();

    {
      // add a constraint to drain h0; this is a "default" type constraint,
      // which means it uses the default values for policy, invalidCost,
      // invalidState, and tuplePosIfBroken (or ones specified through
      // ConstraintParams)
      ToFreeSpec toFreeH0;
      toFreeH0.name() = "drain h0";
      toFreeH0.containers() = {"h0"};

      solver_->addConstraint(toFreeH0);
    }

    {
      // add a constraint to drain h1; this uses the default values for
      // policy, invalidCost, and invalidState, but if broken the broken
      // component is added to goal tuple index 2
      ToFreeSpec toFreeH1;
      toFreeH1.name() = "drain h1";
      toFreeH1.containers() = {"h1"};

      solver_->addConstraint(
          toFreeH1, std::nullopt, std::nullopt, std::nullopt, 2);
    }

    {
      // add a goal to maximize the number of shards in h3; this will
      // incentivize every object to move to h3
      MaximizeAllocationSpec maximizeAllocationSpec;
      maximizeAllocationSpec.scope() = "host";
      maximizeAllocationSpec.dimension() = "shard_count";

      Filter filter;
      filter.itemsWhitelist() = {std::vector<std::string>{"h3"}};

      maximizeAllocationSpec.filter() = filter;

      // add the goal to tuple index 1 with weight 1
      solver_->addGoal(maximizeAllocationSpec, 1, 1);
    }

    // add a local search solver
    const LocalSearchSolverSpec localSearchSolverSpec =
        makeDefaultLocalSearchSolver();
    solver_->addSolver(localSearchSolverSpec);
  }

  std::unique_ptr<ProblemSolver> solver_;
};

INSTANTIATE_TEST_CASE_P(NumThreads, BrokenConstraintsTest, testThreadCounts());

TEST_P(BrokenConstraintsTest, Basic) {
  makeSimpleProblem();

  auto solution = solver_->solve();

  auto initialObjectives = solution.initialGlobalObjective()->goals();
  auto finalObjectives = solution.finalGlobalObjective()->goals();

  EXPECT_EQ(3, initialObjectives->size());
  EXPECT_EQ(3, finalObjectives->size());

  // expect initial value for goal at tupleIndex 0 (corresponding to the broken
  // 'toFreeH0' constraint) to be 10000 + 5*100 (i.e., violation of capacity
  // w.r.t. h0)
  auto initialObj1Value = initialObjectives->at(0).value();
  EXPECT_EQ(*initialObj1Value, 10500);
  // expect final value for goal at tupleIndex 0 to be 0
  auto finalObj1Value = finalObjectives->at(0).value();
  EXPECT_EQ(*finalObj1Value, 0);

  // expect initial value for goal at tupleIndex 1 to be 0 (as number of tasks
  // in h3 is 0)
  auto initialObj2Value = initialObjectives->at(1).value();
  EXPECT_EQ(*initialObj2Value, 0);
  // expect final value for goal at tupleIndex 1 to be -1 (i.e., all shards
  // should have moved to h3)
  auto finalObj2Value = finalObjectives->at(1).value();
  EXPECT_EQ(*finalObj2Value, -1);

  // expect initial value for goal at tupleIndex 2 to be 10000 + 2*100 (due to
  // broken 'toFreeH1' constraint)
  auto initialObj3Value = initialObjectives->at(2).value();
  EXPECT_EQ(*initialObj3Value, 10200);
  // expect final value for goal at tupleIndex 2 to be 0
  auto finalObj3Value = finalObjectives->at(2).value();
  EXPECT_EQ(*finalObj3Value, 0);
}

TEST_P(BrokenConstraintsTest, UseConstraintParams) {
  // same example as in Basic, but where we set ConstraintParams

  {
    // if a constraint is broken, then by default its broken component will be
    // added in tuple index 3
    ConstraintParams constraintParams;
    constraintParams.tuplePosIfBroken() = 3;

    solver_->setDefaultConstraintParams(constraintParams);
  }

  makeSimpleProblem();

  auto solution = solver_->solve();

  auto initialObjectives = solution.initialGlobalObjective()->goals();
  auto finalObjectives = solution.finalGlobalObjective()->goals();

  EXPECT_EQ(4, initialObjectives->size());
  EXPECT_EQ(4, finalObjectives->size());

  // expect initial value and final value for goal at tupleIndex 0 since there
  // is no broken constraint or goal at this position
  auto initialObj1Value = initialObjectives->at(0).value();
  EXPECT_EQ(*initialObj1Value, 0);
  auto finalObj1Value = finalObjectives->at(0).value();
  EXPECT_EQ(*finalObj1Value, 0);

  // expect initial value for goal at tupleIndex 1 to be 0 (as number of tasks
  // in h3 is 0)
  auto initialObj2Value = initialObjectives->at(1).value();
  EXPECT_EQ(*initialObj2Value, 0);
  // expect final value for goal at tupleIndex 1 to be -1 (i.e., all shards
  // should have moved to h3)
  auto finalObj2Value = finalObjectives->at(1).value();
  EXPECT_EQ(*finalObj2Value, -1);

  // expect initial value for goal at tupleIndex 2 to be 10000 + 2*100 (due to
  // broken 'toFreeH1' constraint)
  auto initialObj3Value = initialObjectives->at(2).value();
  EXPECT_EQ(*initialObj3Value, 10200);
  // expect final value for goal at tupleIndex 2 to be 0
  auto finalObj3Value = finalObjectives->at(2).value();
  EXPECT_EQ(*finalObj3Value, 0);

  // Note that by default borken constraints are added to tupleIndex 3. Since
  // 'toFreeH0' uses the default values, expect initial value for goal at
  // tupleIndex 3 (corresponding to the broken 'toFreeH0' constraint) to be
  // 10000 + 5*100 (i.e., violation of capacity w.r.t. h0)
  auto initialObj4Value = initialObjectives->at(3).value();
  EXPECT_EQ(*initialObj4Value, 10500);
  // expect final value for goal at tupleIndex 0 to be 0
  auto finalObj4Value = finalObjectives->at(3).value();
  EXPECT_EQ(*finalObj4Value, 0);
}

TEST_P(BrokenConstraintsTest, ErrorWhenTuplePosIsNegative) {
  makeSimpleProblem();
  {
    // if a constraint is broken, then by default its broken component will be
    // added in tuple index -1; this should raise an error
    ConstraintParams constraintParams;
    constraintParams.tuplePosIfBroken() = -1;

    REBALANCER_EXPECT_RUNTIME_ERROR(
        solver_->setDefaultConstraintParams(constraintParams),
        "'tuplePosIfBroken' should be non-negative");
  }

  {
    // add a dummy constraint to see that an error is raised if tuplePosIfBroken
    // is negative
    ToFreeSpec toFreeDummy;
    toFreeDummy.name() = "check error";
    toFreeDummy.containers() = {"h0"};

    REBALANCER_EXPECT_RUNTIME_ERROR(
        solver_->addConstraint(
            toFreeDummy, std::nullopt, std::nullopt, std::nullopt, -2),
        "'tuplePos'/'tuplePosIfBroken' should be non-negative");
  }
}

TEST_P(BrokenConstraintsTest, NoGoalAndNoBrokenConstraint) {
  setInitialAssignment();

  {
    // add a constraint to see at tuple position 5 which is not broken
    ToFreeSpec satisfied;
    satisfied.name() = "satisfied constraint";
    satisfied.containers() = {"h2"};
    solver_->addConstraint(
        satisfied, std::nullopt, std::nullopt, std::nullopt, 5);
  }

  LocalSearchSolverSpec spec;
  spec.stopAfterMoves() = 100;
  spec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  solver_->addSolver(spec);
  auto solution = solver_->solve();

  auto initialObjectives = solution.initialGlobalObjective()->goals();
  auto finalObjectives = solution.finalGlobalObjective()->goals();

  // we expect the goal to have only one tuple position since there is not goal
  // added and there is no broken constraint
  EXPECT_EQ(1, initialObjectives->size());
  EXPECT_EQ(1, finalObjectives->size());

  for (const auto i : folly::irange(initialObjectives->size())) {
    EXPECT_EQ(0, *initialObjectives->at(i).value());
    EXPECT_EQ(0, *finalObjectives->at(i).value());
  }
}

TEST_P(BrokenConstraintsTest, NoGoalAndBrokenConstraint) {
  setInitialAssignment();

  {
    // add a satisfied constraint to see at tuple position 5
    ToFreeSpec satisified;
    satisified.name() = "satisfied constraint";
    satisified.containers() = {"h1"};
    solver_->addConstraint(
        satisified, std::nullopt, 1 /*invalidCost*/, 0 /*invalidState*/, 5);
  }

  LocalSearchSolverSpec localSearchSpec;
  localSearchSpec.stopAfterMoves() = 100;
  localSearchSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  solver_->addSolver(localSearchSpec);

  auto solution = solver_->solve();

  auto initialObjectives = solution.initialGlobalObjective()->goals();
  auto finalObjectives = solution.finalGlobalObjective()->goals();

  // we expect the goal to have tuple position 0 through 5 since the broken
  // constraint will be added at tuple position 5
  EXPECT_EQ(6, initialObjectives->size());
  EXPECT_EQ(6, finalObjectives->size());

  for (const auto i : folly::irange(initialObjectives->size() - 1)) {
    EXPECT_EQ(0, *initialObjectives->at(i).value());
    EXPECT_EQ(0, *finalObjectives->at(i).value());
  }

  EXPECT_EQ(2, *initialObjectives->at(5).value());
  EXPECT_EQ(0, *finalObjectives->at(5).value());
}

TEST_P(BrokenConstraintsTest, GoalAndABrokenConstraint) {
  setInitialAssignment();

  {
    // add a satisfied constraint to see at tuple position 5
    ToFreeSpec satisfied;
    satisfied.name() = "satisfied constraint";
    satisfied.containers() = {"h3"};
    solver_->addConstraint(
        satisfied, std::nullopt, 1 /*invalidCost*/, 0 /*invalidState*/, 5);
  }

  {
    // add a broken constraint to see at tuple position 1
    ToFreeSpec toFreeBroken;
    toFreeBroken.name() = "broken constraint";
    toFreeBroken.containers() = {"h1"};
    solver_->addConstraint(
        toFreeBroken, std::nullopt, 1 /*invalidCost*/, 0 /*invalidState*/, 1);
  }

  {
    ToFreeSpec toFreeGoal;
    toFreeGoal.name() = "goal";
    toFreeGoal.containers() = {"h0"};
    solver_->addGoal(toFreeGoal, 1, 3);
  }

  LocalSearchSolverSpec localSearchSpec;
  localSearchSpec.stopAfterMoves() = 100;
  localSearchSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  solver_->addSolver(localSearchSpec);

  auto solution = solver_->solve();

  auto initialObjectives = solution.initialGlobalObjective()->goals();
  auto finalObjectives = solution.finalGlobalObjective()->goals();

  // we expect the goal to have tuple position 0 through 3 since goal is at
  // tuple position 3 and the constraint is not broken
  EXPECT_EQ(4, initialObjectives->size());
  EXPECT_EQ(4, finalObjectives->size());

  // no goal or constraint at this position
  EXPECT_EQ(0, *initialObjectives->at(0).value());
  EXPECT_EQ(0, *finalObjectives->at(0).value());

  // broken constraint at this position
  EXPECT_EQ(2, *initialObjectives->at(1).value());
  EXPECT_EQ(0, *finalObjectives->at(1).value());

  // no goal or constraint at this position
  EXPECT_EQ(0, *initialObjectives->at(2).value());
  EXPECT_EQ(0, *finalObjectives->at(2).value());

  EXPECT_EQ(5, *initialObjectives->at(3).value());
  EXPECT_EQ(0, *finalObjectives->at(3).value());
}
