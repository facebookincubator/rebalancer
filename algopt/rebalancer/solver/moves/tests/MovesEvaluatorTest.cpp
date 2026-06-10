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

#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"
#include "algopt/rebalancer/solver/moves/tests/MoveTestBase.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

struct AllowedWorsening {
  double percent;
  double absolute;
  algopt::common::thrift::Intent intent;
};

class MockMovesEvaluator : public MovesEvaluator {
 public:
  static DoNotWorsenGoalConfig makeDoNotWorsenGoalConfig(
      const Problem& problem,
      int objTupleEnd,
      const std::string& stageName,
      const std::optional<
          algopt::common::thrift::HigherPriorityObjectivesConfig>&
          higherPriorityObjConfig) {
    return DoNotWorsenGoalConfig(
        problem, objTupleEnd, stageName, higherPriorityObjConfig);
  }

  static algopt::common::thrift::HigherPriorityObjectivesConfig
  makeHigherPriorityObjConfig(
      const std::map<int, AllowedWorsening>& tuplePosToAllowedWorsening) {
    algopt::common::thrift::HigherPriorityObjectivesConfig
        higherPriorityObjConfig;
    for (auto& [tuplePos, deviation] : tuplePosToAllowedWorsening) {
      auto& [percent, absolute, intent] = deviation;
      algopt::common::thrift::AllowedWorsening allowedWorsening;
      allowedWorsening.percent() = percent;
      allowedWorsening.absolute() = absolute;
      allowedWorsening.intent() = intent;

      higherPriorityObjConfig.tuplePosToAllowedWorsening()->emplace(
          tuplePos, std::move(allowedWorsening));
    }

    return higherPriorityObjConfig;
  }
};

class MovesEvaluatorTest : public MoveTestBase {
 protected:
  MovesEvaluatorTest() : MoveTestBase("object", "container") {}
};

TEST_F(MovesEvaluatorTest, DoNotWorsenGoalConfigNoHigherPriorityObjConfig) {
  setInitialAssignment(
      {{"container1", {"object1", "object2", "object3", "object4"}},
       {"container2", {"object5", "object6"}},
       {"container3", {"object7", "object8"}},
       {"container4", {}}});

  const auto universe = buildUniverse();

  auto objectVector =
      makeObjectVector(PackerMap<entities::ObjectId, double>{}, 1, 8, universe);
  auto container1Util = makeObjectLookup(objectVector, {container(1)});
  auto container2Util = makeObjectLookup(objectVector, {container(2)});
  auto container3Util = makeObjectLookup(objectVector, {container(3)});

  createProblem(
      /*objectiveTuple=*/{container1Util, container2Util, container3Util},
      /*constraint=*/const_expr(0, universe));

  auto& problem = getProblem();
  auto doNotWorsenGoalConfig = MockMovesEvaluator::makeDoNotWorsenGoalConfig(
      problem,
      /*objTupleEnd=*/2,
      "stage 1",
      /*higherPriorityObjConfig=*/std::nullopt);

  EXPECT_FALSE(doNotWorsenGoalConfig.worseningAllowed());
  EXPECT_EQ(2, doNotWorsenGoalConfig.goal.size());
  EXPECT_TRUE(doNotWorsenGoalConfig.tuplePosToAllowedWorsenUntilValue.empty());
  EXPECT_EQ(2, doNotWorsenGoalConfig.doNotWorsenLabels.size());
  EXPECT_EQ(
      "doNotWorsenGoal: Objective at position 0 cannot worsen in stage 1",
      doNotWorsenGoalConfig.doNotWorsenLabels.at(0)->name);
  EXPECT_EQ(
      "doNotWorsenGoal: Objective at position 1 cannot worsen in stage 1",
      doNotWorsenGoalConfig.doNotWorsenLabels.at(1)->name);

  {
    Context context;
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(1), container(1), container(4)));
    // moves improves objective at tuple pos 0; makes none worse
    context.changes() = moveSet.getChangeSet();
    EXPECT_EQ(
        std::nullopt,
        doNotWorsenGoalConfig.getFirstWorseTuplePos(
            context, problem.getOrchestrator()));
  }

  {
    Context context;
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(5), container(2), container(1)));
    // move worsens objective at tuple pos 0
    context.changes() = moveSet.getChangeSet();
    EXPECT_EQ(
        0,
        doNotWorsenGoalConfig.getFirstWorseTuplePos(
            context, problem.getOrchestrator()));
  }

  {
    Context context;
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(7), container(3), container(2)));
    context.changes() = moveSet.getChangeSet();
    // move worsens objective at tuple pos 1
    EXPECT_EQ(
        1,
        doNotWorsenGoalConfig.getFirstWorseTuplePos(
            context, problem.getOrchestrator()));
  }
}

TEST_F(MovesEvaluatorTest, DoNotWorsenGoalConfigHigherPriorityObjConfig1) {
  setInitialAssignment(
      {{"container1", {"object1", "object2", "object3", "object4"}},
       {"container2", {"object5", "object6"}},
       {"container3", {"object7", "object8"}},
       {"container4", {}}});

  const auto universe = buildUniverse();

  auto objectVector =
      makeObjectVector(PackerMap<entities::ObjectId, double>{}, 1, 8, universe);
  auto container1Util = makeObjectLookup(objectVector, {container(1)});
  auto container2Util = makeObjectLookup(objectVector, {container(2)});
  auto container3Util = makeObjectLookup(objectVector, {container(3)});

  createProblem(
      /*objectiveTuple=*/{container1Util, container2Util, container3Util},
      /*constraint=*/const_expr(0, universe));

  auto& problem = getProblem();
  auto doNotWorsenGoalConfig = MockMovesEvaluator::makeDoNotWorsenGoalConfig(
      problem,
      /*objTupleEnd=*/2,
      "Stage: Load balancing",
      MockMovesEvaluator::makeHigherPriorityObjConfig(
          {{0,
            AllowedWorsening{
                .percent = 0,
                .absolute = 1,
                .intent = algopt::common::thrift::Intent::MAX}}}));

  EXPECT_TRUE(doNotWorsenGoalConfig.worseningAllowed());
  EXPECT_EQ(2, doNotWorsenGoalConfig.goal.size());
  EXPECT_EQ(1, doNotWorsenGoalConfig.tuplePosToAllowedWorsenUntilValue.size());
  EXPECT_EQ(2, doNotWorsenGoalConfig.doNotWorsenLabels.size());
  EXPECT_EQ(
      "doNotWorsenGoal: Objective at position 0 cannot become worse than 5 in Stage: Load balancing", // 4 is the initial value of container1Util, allowed worsening is 1
      doNotWorsenGoalConfig.doNotWorsenLabels.at(0)->name);
  EXPECT_EQ(
      "doNotWorsenGoal: Objective at position 1 cannot worsen in Stage: Load balancing",
      doNotWorsenGoalConfig.doNotWorsenLabels.at(1)->name);

  {
    Context context;
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(1), container(1), container(4)));
    // moves improves objective at tuple pos 0; makes none worse
    context.changes() = moveSet.getChangeSet();
    EXPECT_EQ(
        std::nullopt,
        doNotWorsenGoalConfig.getFirstWorseTuplePos(
            context, problem.getOrchestrator()));
  }

  {
    Context context;
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(5), container(2), container(1)));
    // move worsens objective at tuple pos 0 by 1, which is allowed
    context.changes() = moveSet.getChangeSet();
    EXPECT_EQ(
        std::nullopt,
        doNotWorsenGoalConfig.getFirstWorseTuplePos(
            context, problem.getOrchestrator()));
  }

  {
    Context context;
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(7), container(3), container(2)));
    // move worsens objective at tuple pos 1 by 1, which is not allowed
    context.changes() = moveSet.getChangeSet();
    EXPECT_EQ(
        1,
        doNotWorsenGoalConfig.getFirstWorseTuplePos(
            context, problem.getOrchestrator()));
  }
}

TEST_F(MovesEvaluatorTest, DoNotWorsenGoalConfigHigherPriorityObjConfig2) {
  setInitialAssignment(
      {{"container1", {"object1", "object2", "object3", "object4"}},
       {"container2", {"object5", "object6"}},
       {"container3", {"object7", "object8"}},
       {"container4", {}}});

  const auto universe = buildUniverse();

  auto objectVector =
      makeObjectVector(PackerMap<entities::ObjectId, double>{}, 1, 8, universe);
  auto container1Util = makeObjectLookup(objectVector, {container(1)});
  auto container2Util = makeObjectLookup(objectVector, {container(2)});
  auto container3Util = makeObjectLookup(objectVector, {container(3)});

  createProblem(
      /*objectiveTuple=*/{container1Util, container2Util, container3Util},
      /*constraint=*/const_expr(0, universe));

  auto& problem = getProblem();
  auto doNotWorsenGoalConfig = MockMovesEvaluator::makeDoNotWorsenGoalConfig(
      problem,
      /*objTupleEnd=*/2,
      "Stage: Load balancing",
      MockMovesEvaluator::makeHigherPriorityObjConfig(
          {{0,
            AllowedWorsening{
                .percent = 0,
                .absolute = 0,
                .intent = algopt::common::thrift::Intent::MAX}}}));

  EXPECT_TRUE(doNotWorsenGoalConfig.worseningAllowed());
  EXPECT_EQ(2, doNotWorsenGoalConfig.goal.size());
  EXPECT_EQ(1, doNotWorsenGoalConfig.tuplePosToAllowedWorsenUntilValue.size());
  EXPECT_EQ(2, doNotWorsenGoalConfig.doNotWorsenLabels.size());
  EXPECT_EQ(
      "doNotWorsenGoal: Objective at position 0 cannot become worse than 4 in Stage: Load balancing", // 4 is the initial value of container1Util
      doNotWorsenGoalConfig.doNotWorsenLabels.at(0)->name);
  EXPECT_EQ(
      "doNotWorsenGoal: Objective at position 1 cannot worsen in Stage: Load balancing",
      doNotWorsenGoalConfig.doNotWorsenLabels.at(1)->name);

  {
    Context context;
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(1), container(1), container(4)));
    // moves improves objective at tuple pos 0; makes none worse
    context.changes() = moveSet.getChangeSet();
    EXPECT_EQ(
        std::nullopt,
        doNotWorsenGoalConfig.getFirstWorseTuplePos(
            context, problem.getOrchestrator()));

    // apply the move above
    problem.apply(moveSet.getChangeSet());
  }

  {
    Context context;
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(5), container(2), container(1)));
    context.changes() = moveSet.getChangeSet();
    // move worsens objective at tuple pos 0 by 1. Note that although allowed
    // deviation for tuple pos 0 is 0, this move is allowed because it still
    // does not make tuple pos 0 worse than at the start (note that the previous
    // move was applied)
    EXPECT_EQ(
        std::nullopt,
        doNotWorsenGoalConfig.getFirstWorseTuplePos(
            context, problem.getOrchestrator()));
  }

  {
    Context context;
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(7), container(3), container(2)));
    context.changes() = moveSet.getChangeSet();
    // move worsens objective at tuple pos 1 by 1, which is not allowed
    EXPECT_EQ(
        1,
        doNotWorsenGoalConfig.getFirstWorseTuplePos(
            context, problem.getOrchestrator()));
  }
}

TEST_F(MovesEvaluatorTest, TestConstraintViolations) {
  setInitialAssignment(
      {{"container1", {"object1"}},
       {"container2", {"object2", "object3", "object4"}}});

  const auto universe = buildUniverse();

  auto objectVector =
      makeObjectVector(PackerMap<entities::ObjectId, double>{}, 1, 4, universe);
  // Set up constraint: container(1) can hold max 2 objects
  auto container1Objects = makeObjectLookup(objectVector, {container(1)});
  auto constraint = container1Objects - const_expr(2.0, universe);

  createProblem(
      /*objectiveTuple=*/{const_expr(0, universe)},
      /*constraint=*/constraint);

  auto& problem = getProblem();
  const MovesEvaluator evaluator(problem, 0, 1, "Stage: Load balancing");

  // Test move that violates constraint (would put 3 objects in container(1))
  {
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(2), container(2), container(1)));
    moveSet.insert(Move(object(3), container(2), container(1)));
    auto result = evaluator.evaluate(std::move(moveSet));
    EXPECT_FALSE(result.isValid());
  }

  // Test move that respects constraint
  {
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(2), container(2), container(1)));
    auto result = evaluator.evaluate(std::move(moveSet));
    EXPECT_TRUE(result.isValid());
  }
}

TEST_F(MovesEvaluatorTest, TestSatisfiesConstraints) {
  setInitialAssignment(
      {{"container1", {"object1"}},
       {"container2", {"object2", "object3", "object4"}}});

  const auto universe = buildUniverse();

  auto objectVector =
      makeObjectVector(PackerMap<entities::ObjectId, double>{}, 1, 4, universe);
  // Set up constraint: container(1) can hold max 2 objects
  auto container1Objects = makeObjectLookup(objectVector, {container(1)});
  auto constraint = container1Objects - const_expr(2.0, universe);

  createProblem(
      /*objectiveTuple=*/{const_expr(0, universe)},
      /*constraint=*/constraint);

  auto& problem = getProblem();
  const MovesEvaluator evaluator(problem, 0, 1, "Stage: Load balancing");

  // Test move that violates constraint (would put 3 objects in container(1))
  {
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(2), container(2), container(1)));
    moveSet.insert(Move(object(3), container(2), container(1)));
    EXPECT_FALSE(evaluator.satisfiesConstraints(moveSet));
  }

  // Test move that respects constraint
  {
    auto moveSet = MoveSet();
    moveSet.insert(Move(object(2), container(2), container(1)));
    EXPECT_TRUE(evaluator.satisfiesConstraints(moveSet));
  }
}

} // namespace facebook::rebalancer::packer::tests
