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
#include "algopt/rebalancer/materializer/spec_builder/MoveGroupSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class MoveGroupSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host0", {"task0", "task1"}},
         {"host1", {"task2", "task3", "task4", "task5"}},
         {"host2", {}}});

    co_await addPartition(
        "unit",
        {{"unit0", {"task0", "task1"}},
         {"unit1", {"task2", "task3", "task4"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  MoveGroupSpecBuilder makeSpecBuilder() {
    interface::MoveGroupSpec spec;
    spec.partitionName() = "unit";
    return MoveGroupSpecBuilder(buildUniverse(), spec);
  }
};

CO_TEST_F(MoveGroupSpecBuilderTest, Constraint) {
  auto specBuilder = makeSpecBuilder();
  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  auto root = any_positive(constraints);

  // Initially not broken.
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({})), 1e-8);

  // Moving task0 alone breaks unit0.
  EXPECT_NEAR(1, evaluate(root, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  // Moving the entire unit0 is valid.
  EXPECT_NEAR(
      0,
      evaluate(
          root,
          deltaFromInitial({
              {"task0", "host1"},
              {"task1", "host1"},
          })),
      1e-8);

  // Moving task0 and task1 to different destinations breaks unit0.
  EXPECT_NEAR(
      1,
      evaluate(
          root,
          deltaFromInitial({
              {"task0", "host1"},
              {"task1", "host2"},
          })),
      1e-8);

  // Moving task2 alone breaks unit1.
  EXPECT_NEAR(1, evaluate(root, deltaFromInitial({{"task2", "host2"}})), 1e-8);

  // Moving task2 and task3 together still breaks unit1.
  EXPECT_NEAR(
      1,
      evaluate(
          root, deltaFromInitial({{"task2", "host2"}, {"task3", "host2"}})),
      1e-8);

  // Moving the entire unit1 is valid.
  EXPECT_NEAR(
      0,
      evaluate(
          root,
          deltaFromInitial(
              {{"task2", "host2"}, {"task3", "host2"}, {"task4", "host2"}})),
      1e-8);

  // task5 is free to move independently.
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({{"task5", "host0"}})), 1e-8);
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({{"task5", "host2"}})), 1e-8);
}

TEST_F(MoveGroupSpecBuilderTest, GoalNotSupported) {
  auto specBuilder = makeSpecBuilder();
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "MoveGroupSpec not supported as a goal");
}

TEST_F(MoveGroupSpecBuilderTest, SpecInfo) {
  auto specBuilder = makeSpecBuilder();
  auto expectedSpecInfo = SpecParameters{.name = "", .partition = "unit"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

} // namespace facebook::rebalancer::materializer::tests
