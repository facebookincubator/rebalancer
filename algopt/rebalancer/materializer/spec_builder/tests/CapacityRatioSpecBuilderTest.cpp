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

#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/materializer/spec_builder/CapacityRatioSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class CapacityRatioSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"h1", {"t1"}}, {"h2", {}}, {"h3", {"t2", "t3"}}, {"h4", {"t4"}}});

    const entities::Map<entities::ObjectId, double> values = {
        {objectId("t1"), 1},
        {objectId("t2"), 2},
        {objectId("t3"), 2},
        {objectId("t4"), 4}};
    co_await addObjectDimension("dim", values, 0);
    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(CapacityRatioSpecBuilderTest, Goal) {
  interface::CapacityRatioSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "dim";
  spec.ratios() = {{"h1", {{"h2", 0.5}}}};

  const CapacityRatioSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "Limit capacity ratio (dim) on scope host", specBuilder.description());

  const auto expectedSpecInfo =
      SpecParameters{.name = "test", .scope = "host", .dimension = "dim"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());

  const auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // h1/h2 = 0.5
  // h1 - 0.5 * h2 = 1
  EXPECT_NEAR(1, evaluate(goal, deltaFromInitial({})), 1e-8);

  //  1 - 0.5 * 4 = -1
  // but goal will not be -ve hence we will still get 0
  EXPECT_NEAR(0, evaluate(goal, deltaFromInitial({{"t4", "h2"}})), 1e-8);

  EXPECT_NEAR(5, evaluate(goal, deltaFromInitial({{"t4", "h1"}})), 1e-8);
}

CO_TEST_F(CapacityRatioSpecBuilderTest, Constraint) {
  interface::CapacityRatioSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "dim";
  spec.ratios() = {{"h1", {{"h2", 0.5}}}};

  const CapacityRatioSpecBuilder specBuilder(buildUniverse(), spec);

  const auto constraints =
      co_await specBuilder.constraints(expressionBuilder());

  // we just have 1 constarint
  EXPECT_EQ(1, constraints.size());

  // h1/h2 = 0.5
  // h1 - 0.5 * h2 = 1
  EXPECT_NEAR(1, evaluate(constraints.at(0), deltaFromInitial({})), 1e-8);

  //  1 - 0.5 * 4 = -1
  EXPECT_NEAR(
      -1, evaluate(constraints.at(0), deltaFromInitial({{"t4", "h2"}})), 1e-8);

  const auto expectedSpecInfo =
      SpecParameters{.name = "test", .scope = "host", .dimension = "dim"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
