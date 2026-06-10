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
#include "algopt/rebalancer/materializer/spec_builder/MovesInProgressSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class MovesInProgressSpecBuilderTest : public SpecBuilderTestBase<> {
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

static interface::MovesInProgressSpec makeExampleSpec() {
  interface::MovesInProgressSpec movesInProgressSpec;
  movesInProgressSpec.name() = "testSpec";
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
  movesInProgressSpec.moves() = std::move(moves);
  return movesInProgressSpec;
}

CO_TEST_F(MovesInProgressSpecBuilderTest, Constraint) {
  const MovesInProgressSpecBuilder specBuilder(
      buildUniverse(), makeExampleSpec());
  EXPECT_EQ("2 objects are being moved", specBuilder.description());

  auto constraints = co_await specBuilder.constraints(expressionBuilder());

  auto root = any_positive(constraints);

  // no movement hence constraint will break
  EXPECT_NEAR(1, evaluate(root, deltaFromInitial({})), 1e-8);

  // all objects not in destination container hence constraint will break
  EXPECT_NEAR(1, evaluate(root, deltaFromInitial({{"task2", "host0"}})), 1e-8);

  // task2->host0, task0->host3; hence constraint is satisfied
  EXPECT_NEAR(
      0,
      evaluate(
          root, deltaFromInitial({{"task2", "host0"}, {"task0", "host3"}})),
      1e-8);

  // additional move which shouldn't be blocked by this constraint
  EXPECT_NEAR(
      0,
      evaluate(
          root,
          deltaFromInitial(
              {{"task2", "host0"}, {"task0", "host3"}, {"task1", "host0"}})),
      1e-8);
}

TEST_F(MovesInProgressSpecBuilderTest, Goal) {
  interface::MovesInProgressSpec MovesInProgressSpec;
  MovesInProgressSpec.name() = "testSpec";
  std::vector<interface::MoveInProgress> moves;
  interface::MoveInProgress move;
  move.objName() = "task2";
  move.toContainer() = "host0";
  moves.push_back(move);
  MovesInProgressSpec.moves() = std::move(moves);

  MovesInProgressSpecBuilder specBuilder(buildUniverse(), MovesInProgressSpec);
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "MovesInProgressSpec not supported as a goal");
}

TEST_F(MovesInProgressSpecBuilderTest, UpdatesInInitialAssignment) {
  auto universe = buildUniverse();
  const MovesInProgressSpecBuilder specBuilder(universe, makeExampleSpec());

  auto updates = specBuilder.getUpdatesInInitialAssignment();

  EXPECT_EQ(2, updates.size());
  EXPECT_EQ(
      universe->getContainerId("host3"),
      updates.at(universe->getObjectId("task0")));
  EXPECT_EQ(
      universe->getContainerId("host0"),
      updates.at(universe->getObjectId("task2")));
}

TEST_F(MovesInProgressSpecBuilderTest, FixedObjects) {
  auto universe = buildUniverse();
  const MovesInProgressSpecBuilder specBuilder(universe, makeExampleSpec());

  auto fixedObjects = specBuilder.fixedObjects();

  EXPECT_EQ(2, fixedObjects.size());
  EXPECT_EQ(1, fixedObjects.count(universe->getObjectId("task0")));
  EXPECT_EQ(1, fixedObjects.count(universe->getObjectId("task2")));
}

TEST_F(MovesInProgressSpecBuilderTest, SpecInfo) {
  interface::MovesInProgressSpec MovesInProgressSpec;
  MovesInProgressSpec.name() = "testSpec";
  std::vector<interface::MoveInProgress> moves;
  interface::MoveInProgress move;
  move.objName() = "task2";
  move.toContainer() = "host0";
  moves.push_back(move);
  MovesInProgressSpec.moves() = std::move(moves);

  const MovesInProgressSpecBuilder specBuilder(
      buildUniverse(), MovesInProgressSpec);

  auto expectedSpecInfo = SpecParameters{.name = "testSpec", .size = 1};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

} // namespace facebook::rebalancer::materializer::tests
