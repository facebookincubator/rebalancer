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
#include "algopt/rebalancer/materializer/spec_builder/UtilIncreaseCostSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class UtilIncreaseCostSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1"}},
        {"host1", {}},
        {"host2", {"task2"}},
        {"host3", {"task3"}},
    });

    const entities::Map<entities::ObjectId, double> cpuValues = {
        {objectId("task0"), 10},
        {objectId("task1"), 10},
        {objectId("task2"), 10},
        {objectId("task3"), 10}};
    co_await addObjectDimension("cpu", cpuValues, 0);

    const entities::Map<std::string, double> hostCapacity = {
        {"host0", 20}, {"host1", 20}, {"host2", 20}, {"host3", 20}};
    co_await addScopeDimension("cpu", host(), hostCapacity, 0);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  entities::ScopeId host() const {
    return scopeId("host");
  }
};

CO_TEST_F(UtilIncreaseCostSpecBuilderTest, GoalWithoutSquaresPenalty) {
  interface::UtilIncreaseCostSpec spec;
  spec.dimension() = "cpu";
  spec.scope() = "host";
  spec.lowerBound() = 0.4;
  spec.squares() = false;
  const UtilIncreaseCostSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "Util increase cost over 0.4 on cpu of host, squares: false",
      specBuilder.description());

  auto expectedSpecInfo = SpecParameters{
      .name = "",
      .scope = "host",
      .dimension = "cpu",
      .squares = "no",
      .filterAllowListSize = 0,
      .filterBlockListSize = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());
  // the initial assignment will always evaluate to zero
  EXPECT_NEAR(0, evaluate(goal, deltaFromInitial({})), 1e-8);
  // only host1 will have a non-zero contribution
  //  0.25 * max(0, util_after - max(util_before, lb)) = 0.25 * (0.5 - 0.4)
  EXPECT_NEAR(
      0.025, evaluate(goal, deltaFromInitial({{"task1", "host1"}})), 1e-8);
}

CO_TEST_F(UtilIncreaseCostSpecBuilderTest, GoalWithSquaresPenalty) {
  interface::UtilIncreaseCostSpec spec;
  spec.dimension() = "cpu";
  spec.scope() = "host";
  spec.lowerBound() = 0.4;
  spec.squares() = true;
  const UtilIncreaseCostSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "Util increase cost over 0.4 on cpu of host, squares: true",
      specBuilder.description());

  auto expectedSpecInfo = SpecParameters{
      .name = "",
      .scope = "host",
      .dimension = "cpu",
      .squares = "yes",
      .filterAllowListSize = 0,
      .filterBlockListSize = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());
  // the initial assignment will always evaluate to zero
  EXPECT_NEAR(0, evaluate(goal, deltaFromInitial({})), 1e-8);
  // only host1 will have a non-zero contribution
  //  0.25 * max(0, util_after - max(util_before, lb))
  // = 0.25 * (0.5^1.1 - 0.4^1.1)
  EXPECT_NEAR(
      0.02538477028,
      evaluate(goal, deltaFromInitial({{"task1", "host1"}})),
      1e-8);
}

CO_TEST_F(UtilIncreaseCostSpecBuilderTest, GoalWithinLowerbound) {
  interface::UtilIncreaseCostSpec spec;
  spec.dimension() = "cpu";
  spec.scope() = "host";
  spec.lowerBound() = 0.5;
  spec.squares() = false;
  const UtilIncreaseCostSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "Util increase cost over 0.5 on cpu of host, squares: false",
      specBuilder.description());

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());
  // the initial assignment will always evaluate to zero
  EXPECT_NEAR(0, evaluate(goal, deltaFromInitial({})), 1e-8);
  // tasks are moving to host1, so only host1 may have a non-zero contribution
  // case 1: util after move is 0.5 which is <= lb, so penalty = 0
  EXPECT_NEAR(0, evaluate(goal, deltaFromInitial({{"task1", "host1"}})), 1e-8);
  // case 2: util after move is 1, so penalty = 0.5 * avg = 0.5*0.25
  EXPECT_NEAR(
      0.125,
      evaluate(
          goal, deltaFromInitial({{"task1", "host1"}, {"task2", "host1"}})),
      1e-8);
}

TEST_F(UtilIncreaseCostSpecBuilderTest, Constraint) {
  const interface::UtilIncreaseCostSpec spec;
  UtilIncreaseCostSpecBuilder specBuilder(buildUniverse(), spec);
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder())),
      "UtilIncreaseCostSpec not supported as a constraint");
}

} // namespace facebook::rebalancer::materializer::tests
