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
#include "algopt/rebalancer/materializer/spec_builder/ExclusiveSwapsSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {
namespace interface = facebook::rebalancer::interface;

class ExclusiveSwapsSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host0", {"task0", "task1"}},
         {"host1", {"task2"}},
         {"host2", {"task3"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

static interface::ExclusiveSwapsSpec makeSpec(
    std::optional<interface::ExclusiveSwapsSpecSubsetDefinition> definition) {
  interface::ExclusiveSwapsSpec spec;
  if (definition) {
    spec.subsetObjects() = {"task0", "task2"};
    spec.subsetDefinition() = *definition;
  }
  return spec;
}

TEST_F(ExclusiveSwapsSpecBuilderTest, Goal) {
  const interface::ExclusiveSwapsSpec spec;
  ExclusiveSwapsSpecBuilder specBuilder(buildUniverse(), spec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "ExclusiveSwapsSpec not supported as a goal");
}

CO_TEST_F(ExclusiveSwapsSpecBuilderTest, AtLeastOneInSubset) {
  auto spec = makeSpec(
      interface::ExclusiveSwapsSpecSubsetDefinition::AT_LEAST_ONE_IN_SUBSET);

  const ExclusiveSwapsSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraints = co_await specBuilder.constraints(expressionBuilder());

  auto constraint = any_positive(constraints);

  // No moves: valid.
  EXPECT_NEAR(0.0, evaluate(constraint, deltaFromInitial({})), 1e-8);

  // Not a swap: invalid
  EXPECT_NEAR(
      1.0, evaluate(constraint, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  // Swap with both sides outside of subset: invalid
  EXPECT_NEAR(
      1.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task1", "host2"}, {"task3", "host0"}})),
      1e-8);

  // Swap with both sides inside of subset: valid
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task0", "host1"}, {"task2", "host0"}})),
      1e-8);

  // Swap with exactly one side inside of subset: valid
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task1", "host1"}, {"task2", "host0"}})),
      1e-8);
}

CO_TEST_F(ExclusiveSwapsSpecBuilderTest, ExactlyOneInSubset) {
  auto spec = makeSpec(
      interface::ExclusiveSwapsSpecSubsetDefinition::EXACTLY_ONE_IN_SUBSET);

  const ExclusiveSwapsSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraints = co_await specBuilder.constraints(expressionBuilder());

  auto constraint = any_positive(constraints);

  // No moves: valid.
  EXPECT_NEAR(0.0, evaluate(constraint, deltaFromInitial({})), 1e-8);

  // Not a swap: invalid
  EXPECT_NEAR(
      1.0, evaluate(constraint, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  // Swap with both sides outside of subset: invalid
  EXPECT_NEAR(
      1.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task1", "host2"}, {"task3", "host0"}})),
      1e-8);

  // Swap with both sides inside of subset: invalid
  EXPECT_NEAR(
      1.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task0", "host1"}, {"task2", "host0"}})),
      1e-8);

  // Swap with exactly one side inside of subset: valid
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task1", "host1"}, {"task2", "host0"}})),
      1e-8);
}

CO_TEST_F(ExclusiveSwapsSpecBuilderTest, BothSameSideOfSubset) {
  auto spec = makeSpec(
      interface::ExclusiveSwapsSpecSubsetDefinition::BOTH_SAME_SIDE_OF_SUBSET);

  const ExclusiveSwapsSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  auto constraint = any_positive(constraints);

  // No moves: valid.
  EXPECT_NEAR(0.0, evaluate(constraint, deltaFromInitial({})), 1e-8);

  // Not a swap: invalid
  EXPECT_NEAR(
      1.0, evaluate(constraint, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  // Swap with both sides outside of subset: valid
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task1", "host2"}, {"task3", "host0"}})),
      1e-8);

  // Swap with both sides inside of subset: valid
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task0", "host1"}, {"task2", "host0"}})),
      1e-8);

  // Swap with exactly one side inside of subset: invalid
  EXPECT_NEAR(
      1.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task1", "host1"}, {"task2", "host0"}})),
      1e-8);
}

CO_TEST_F(ExclusiveSwapsSpecBuilderTest, NoSubset) {
  auto spec = makeSpec(std::nullopt);

  const ExclusiveSwapsSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  auto constraint = any_positive(constraints);

  // No moves: valid.
  EXPECT_NEAR(0.0, evaluate(constraint, deltaFromInitial({})), 1e-8);

  // Not a swap: invalid
  EXPECT_NEAR(
      1.0, evaluate(constraint, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  // Swap with both sides outside of subset: valid
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task1", "host2"}, {"task3", "host0"}})),
      1e-8);

  // Swap with both sides inside of subset: valid
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task0", "host1"}, {"task2", "host0"}})),
      1e-8);

  // Swap with exactly one side inside of subset: valid
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial({{"task1", "host1"}, {"task2", "host0"}})),
      1e-8);
}

TEST_F(ExclusiveSwapsSpecBuilderTest, Description) {
  auto spec = makeSpec(
      interface::ExclusiveSwapsSpecSubsetDefinition::AT_LEAST_ONE_IN_SUBSET);
  const ExclusiveSwapsSpecBuilder specBuilder(buildUniverse(), spec);
  EXPECT_EQ(
      "Exclusive swaps (at least one in subset)", specBuilder.description());
}

TEST_F(ExclusiveSwapsSpecBuilderTest, SpecInfo) {
  auto spec = makeSpec(
      interface::ExclusiveSwapsSpecSubsetDefinition::AT_LEAST_ONE_IN_SUBSET);
  spec.name() = "test";

  const ExclusiveSwapsSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo =
      SpecParameters{.name = "test", .definition = "AT_LEAST_ONE_IN_SUBSET"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
