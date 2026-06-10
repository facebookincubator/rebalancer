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
#include "algopt/rebalancer/materializer/spec_builder/MinimizeSquaresSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class MinimizeSquaresSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({{"h1", {"t1", "t2", "t3"}}, {"h2", {"t4"}}, {"h3", {}}});

    const entities::Map<entities::ObjectId, double> objectValues = {
        {objectId("t1"), 100},
        {objectId("t2"), 5},
        {objectId("t3"), 5},
        {objectId("t4"), 10}};
    co_await addObjectDimension("units", objectValues, 0);

    auto hostScopeId = scopeId(universeBuilder_.getContainerTypeName());
    const entities::Map<std::string, double> scopeValues = {
        {"h1", 200}, {"h2", 200}, {"h3", 200}};
    co_await addScopeDimension("units", hostScopeId, scopeValues, 0);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(MinimizeSquaresSpecBuilderTest, Goal) {
  interface::MinimizeSquaresSpec minimizeSquaresSpec;
  minimizeSquaresSpec.scope() = "host";
  minimizeSquaresSpec.dimension() = "units";

  const MinimizeSquaresSpecBuilder specBuilder(
      buildUniverse(), minimizeSquaresSpec);

  EXPECT_EQ(
      "Minimize sum of squared units across hosts", specBuilder.description());

  // 1/3 * (110/200) + 1/3 * (10/200) = 0.10083333 + 0.000833325 =
  // 0.1016666582325
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());
  EXPECT_NEAR(0.1016666582325, evaluate(goal, deltaFromInitial({})), 1e-8);
}

TEST_F(MinimizeSquaresSpecBuilderTest, Constraint) {
  interface::MinimizeSquaresSpec spec;
  spec.scope() = "host";
  spec.dimension() = "units";
  MinimizeSquaresSpecBuilder specBuilder(buildUniverse(), spec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder())),
      "MinimizeSquaresSpec not supported as a constraint");
}

TEST_F(MinimizeSquaresSpecBuilderTest, SpecInfo) {
  interface::MinimizeSquaresSpec minimizeSquaresSpec;
  minimizeSquaresSpec.scope() = "host";
  minimizeSquaresSpec.dimension() = "units";

  const MinimizeSquaresSpecBuilder specBuilder(
      buildUniverse(), minimizeSquaresSpec);

  auto expectedSpecInfo = SpecParameters{
      .name = "",
      .scope = "host",
      .dimension = "units",
      .filterAllowListSize = 0,
      .filterBlockListSize = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

} // namespace facebook::rebalancer::materializer::tests
