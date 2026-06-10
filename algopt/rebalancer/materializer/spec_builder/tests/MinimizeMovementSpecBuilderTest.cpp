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
#include "algopt/rebalancer/materializer/spec_builder/MinimizeMovementSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class MinimizeMovementSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1"}},
        {"host1", {"task2"}},
        {"host2", {"task3"}},
    });

    const entities::Map<entities::ObjectId, double> objectValues = {
        {objectId("task0"), 100},
        {objectId("task1"), 5},
        {objectId("task2"), 5}};
    co_await addObjectDimension("units", objectValues, 0);

    auto hostScopeId = scopeId(universeBuilder_.getContainerTypeName());
    const entities::Map<std::string, double> scopeValues = {
        {"host0", 200}, {"host1", 200}, {"host2", 200}};
    co_await addScopeDimension("units", hostScopeId, scopeValues, 0);
    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(MinimizeMovementSpecBuilderTest, Constraint) {
  interface::MinimizeMovementSpec spec;
  spec.scope() = "host";
  spec.dimension() = "units";

  const MinimizeMovementSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "Minimize movement on units for scope host", specBuilder.description());

  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  auto root = any_positive(constraints);

  // use non-default tolerances since to otherwise the xpress/gurobi is
  // encountering numerical issues
  auto lpAssertOptions = packer::tests::LpAssertOptions{
      .lpTolerances =
          algopt::lp::Tolerances{.constraint = 1.e-7, .integer = 1e-6}};

  // no movement hence value will be 0
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({}), lpAssertOptions), 1e-8);

  // still it will be 0 as task3 dim is 0 hence wont be braking constraint
  EXPECT_NEAR(
      0,
      evaluate(root, deltaFromInitial({{"task3", "host1"}}), lpAssertOptions),
      1e-8);

  // moving task2 will break the constraint hence value will be +ve
  EXPECT_NEAR(
      1,
      evaluate(root, deltaFromInitial({{"task2", "host0"}}), lpAssertOptions),
      1e-8);
}

CO_TEST_F(MinimizeMovementSpecBuilderTest, MagicScalingConstraint) {
  interface::MinimizeMovementSpec spec;
  spec.scope() = "host";
  spec.dimension() = "units";

  const MinimizeMovementSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraint = co_await specBuilder.constraints(expressionBuilder());

  // moving task2 will break the constraint hence value will be +ve
  // = (0.001/3 * 5/200) + (0.001/3 * 0) = 0.00000833333
  EXPECT_NEAR(
      8.33e-6,
      evaluate(constraint.at(0), deltaFromInitial({{"task2", "host0"}})),
      1e-8);
}

CO_TEST_F(MinimizeMovementSpecBuilderTest, NormalizeConstraint) {
  interface::MinimizeMovementSpec spec;
  spec.scope() = "host";
  spec.dimension() = "units";
  spec.doNotNormalize() = true;

  const MinimizeMovementSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraint = co_await specBuilder.constraints(expressionBuilder());

  // moving task2 will break the constraint hence value will be +ve
  // = 5 + 0 = 5
  EXPECT_NEAR(
      5,
      evaluate(constraint.at(0), deltaFromInitial({{"task2", "host0"}})),
      1e-8);
}

CO_TEST_F(MinimizeMovementSpecBuilderTest, NormalizeWithAllowanceConstraint) {
  interface::MinimizeMovementSpec spec;
  spec.scope() = "host";
  spec.dimension() = "units";
  spec.doNotNormalize() = true;
  spec.allowance() = 6;

  const MinimizeMovementSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraint = co_await specBuilder.constraints(expressionBuilder());

  // moving task2 will break the constraint hence value will be +ve
  // = 5 + 0 = 5; result = max(0, 5 - 6) = 0
  EXPECT_NEAR(
      0,
      evaluate(constraint.at(0), deltaFromInitial({{"task2", "host0"}})),
      1e-8);
}

TEST_F(MinimizeMovementSpecBuilderTest, SpecInfo) {
  interface::MinimizeMovementSpec spec;
  spec.scope() = "host";
  spec.dimension() = "units";

  const MinimizeMovementSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo =
      SpecParameters{.name = "", .scope = "host", .dimension = "units"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

} // namespace facebook::rebalancer::materializer::tests
