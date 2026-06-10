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
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/materializer/spec_builder/AvoidMovingSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class AvoidMovingSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host0", {"task0"}},
         {"host1", {"task1"}},
         {"host2", {"task2", "task3"}},
         {"host3", {"task4"}}});
    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(AvoidMovingSpecBuilderTest, Constraint) {
  interface::AvoidMovingSpec spec;
  spec.name() = "test";
  spec.objects() = {"task0", "task1", "task3", "task2"};

  const AvoidMovingSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ("Avoid moving 4 objects", specBuilder.description());

  const auto expectedSpecInfo = SpecParameters{.name = "test", .size = 4};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());

  const auto constraints =
      co_await specBuilder.constraints(expressionBuilder());
  const auto root = any_positive(constraints);

  // no movement hence constarint is satisfied
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({})), 1e-8);

  // task4 has no restriction hence can be moved
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({{"task4", "host1"}})), 1e-8);

  // task0 cannot be moved hence constraint breaks.
  EXPECT_NEAR(
      1,
      evaluate(
          root, deltaFromInitial({{"task0", "host1"}, {"task4", "host1"}})),
      1e-8);

  EXPECT_NEAR(1, evaluate(root, deltaFromInitial({{"task1", "host2"}})), 1e-8);

  // okay to move task to initial container
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({{"task1", "host1"}})), 1e-8);
}

TEST_F(AvoidMovingSpecBuilderTest, Goal) {
  interface::AvoidMovingSpec spec;
  spec.name() = "test";
  spec.objects() = {"task0"};

  AvoidMovingSpecBuilder specBuilder(buildUniverse(), spec);
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "AvoidMovingSpec not supported as a goal");
}

CO_TEST_F(AvoidMovingSpecBuilderTest, ConstraintWithMoveInProgress) {
  interface::AvoidMovingSpec spec;
  spec.name() = "test";
  spec.objects() = {"task0", "task1", "task3", "task2"};

  interface::MovesInProgressSpec moveInProgressSpec;
  moveInProgressSpec.name() = "testSpec";
  std::vector<interface::MoveInProgress> moves;
  {
    interface::MoveInProgress move;
    move.objName() = "task0";
    move.toContainer() = "host3";
    moves.push_back(move);
  }
  {
    interface::MoveInProgress move;
    move.objName() = "task2";
    move.toContainer() = "host0";
    moves.push_back(move);
  }
  moveInProgressSpec.moves() = std::move(moves);
  interface::ConstraintSpecs constraintSpecs;
  constraintSpecs.movesInProgressSpec() = std::move(moveInProgressSpec);

  co_await addConstraint(
      "moveInProgressSpec",
      std::move(constraintSpecs),
      interface::ConstraintPolicy::DEFAULT,
      10,
      10000,
      0);
  const entities::Map<entities::ContainerId, std::vector<entities::ObjectId>>
      updates;

  const AvoidMovingSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraints =
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder()));
  auto root = any_positive(constraints);

  // no movement hence constarint is satisfied
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({})), 1e-8);

  // task0 -> host3 move is already in progress hence constarint won't break
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({{"task0", "host3"}})), 1e-8);

  // same like above, move in progess hence this should be fine.
  EXPECT_NEAR(
      0,
      evaluate(
          root, deltaFromInitial({{"task0", "host3"}, {"task2", "host0"}})),
      1e-8);

  // this move is not allowed hence constraint should break.
  EXPECT_NEAR(1, evaluate(root, deltaFromInitial({{"task1", "host2"}})), 1e-8);
}

TEST_F(AvoidMovingSpecBuilderTest, FixedObjects) {
  interface::AvoidMovingSpec spec;
  spec.name() = "test";
  spec.objects() = {"task0", "task1", "task3", "task2"};

  const auto universe = buildUniverse();
  const AvoidMovingSpecBuilder specBuilder(universe, spec);

  const auto fixedObjects = specBuilder.fixedObjects();
  EXPECT_EQ(4, fixedObjects.size());
  EXPECT_EQ(1, fixedObjects.count(objectId("task0")));
}
} // namespace facebook::rebalancer::materializer::tests
