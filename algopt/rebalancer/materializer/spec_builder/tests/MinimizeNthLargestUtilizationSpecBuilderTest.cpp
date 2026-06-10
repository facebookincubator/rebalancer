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
#include "algopt/rebalancer/materializer/spec_builder/MinimizeNthLargestUtilizationSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class MinimizeNthLargestUtilizationSpecBuilderTest
    : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1", "task2", "task7"}},
        {"host1", {}},
        {"host2", {"task6"}},
        {"host3", {"task3", "task4"}},
    });

    // all objects take 1 cpu (default value)
    co_await addObjectDimension(
        "cpu", entities::Map<entities::ObjectId, double>(), 1);

    // all hosts except host0 have 10 cpu; host0 has 14 cpu
    auto hostScopeId = scopeId(universeBuilder_.getContainerTypeName());
    co_await addScopeDimension("cpu", hostScopeId, {{"host0", 14}}, 10);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  double evaluate(
      ExprPtr expression,
      const entities::Map<
          entities::ContainerId,
          std::vector<entities::ObjectId>>& containerToObjects) const {
    // disable lpExpr evaluation because it is not supported for this spec
    packer::tests::LpAssertOptions assertOptions = {
        .exceptionForLpExpr =
            "lpIntent is not implemented for NthLargest expr"};
    return SpecBuilderTestBase::evaluate(
        expression, containerToObjects, assertOptions);
  }
};

TEST_F(MinimizeNthLargestUtilizationSpecBuilderTest, CheckDescription) {
  interface::MinimizeNthLargestUtilizationSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.n() = 4;

  const MinimizeNthLargestUtilizationSpecBuilder specBuilder(
      buildUniverse(), spec);
  EXPECT_EQ(
      "Minimize 4-th largest cpu utilization across host with target utilization 0",
      specBuilder.description());
}

CO_TEST_F(MinimizeNthLargestUtilizationSpecBuilderTest, Basic) {
  interface::MinimizeNthLargestUtilizationSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.n() = 1;

  const MinimizeNthLargestUtilizationSpecBuilder specBuilder(
      buildUniverse(), spec);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // the relative utilizations for the assignment are (host0 : 4/14,
  // host1: 0, host2: 1/10, host3: 2/10); hence the expected goal value
  // (corresponding to the second highest relative utilization) is 2/10
  // NOTE: n is a 0-based index
  EXPECT_NEAR(2.0 / 10, evaluate(goal, deltaFromInitial({})), 1e-8);

  // now if we move one object from host0 to host3, then the second highest
  // utilization corresponds to that of host0 (which is 3/14; host3 has
  // utilization 3/10)
  EXPECT_NEAR(
      3.0 / 14, evaluate(goal, deltaFromInitial({{"task0", "host3"}})), 1e-8);
}

CO_TEST_F(MinimizeNthLargestUtilizationSpecBuilderTest, BasicWithFilter) {
  // same example as the one above, but where host0 is filtered
  interface::MinimizeNthLargestUtilizationSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.n() = 1;
  spec.filter()->itemsBlacklist() = {"host0"};

  const MinimizeNthLargestUtilizationSpecBuilder specBuilder(
      buildUniverse(), spec);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // the relative utilizations for the assignment are (host0 : 4/14,
  // host1: 0, host2: 1/10, host3: 2/10).
  // However, host0 is filtered. So, the expected goal value
  // (corresponding to the second highest relative utilization) is 1/10
  EXPECT_NEAR(1.0 / 10, evaluate(goal, deltaFromInitial({})), 1e-8);

  // now if we move one object from host0 to host3, then the second highest
  // utilization still corresponds to that of host2 (which is 1/10), since host0
  // is filtered
  EXPECT_NEAR(
      1.0 / 10, evaluate(goal, deltaFromInitial({{"task0", "host3"}})), 1e-8);
}

CO_TEST_F(MinimizeNthLargestUtilizationSpecBuilderTest, WithTargetUtilization) {
  // same example as in 'Basic', but where targetUtilization is 0.1
  interface::MinimizeNthLargestUtilizationSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.n() = 1;
  spec.targetUtilization() = 0.1;

  const MinimizeNthLargestUtilizationSpecBuilder specBuilder(
      buildUniverse(), spec);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // the relative utilizations for the assignment are (host0 : 4/14,
  // host1: 0, host2: 1/10, host3: 2/10); hence the expected goal value
  // (corresponding to the second highest relative utilization) is 2/10 - 0.1
  EXPECT_NEAR(2.0 / 10 - 0.1, evaluate(goal, deltaFromInitial({})), 1e-8);

  // now if we move one object from host0 to host3, then the second highest
  // utilization corresponds to that of host0 (which is 3/14; host3 has
  // utilization 3/10)
  EXPECT_NEAR(
      3.0 / 14 - 0.1,
      evaluate(goal, deltaFromInitial({{"task0", "host3"}})),
      1e-8);
}

TEST_F(MinimizeNthLargestUtilizationSpecBuilderTest, SpecInfo) {
  // same example as in 'Basic', but where targetUtilization is 0.1
  interface::MinimizeNthLargestUtilizationSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.n() = 1;
  spec.targetUtilization() = 0.1;
  spec.filter()->itemsBlacklist() = {"host0"};

  const MinimizeNthLargestUtilizationSpecBuilder specBuilder(
      buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "",
      .scope = "host",
      .dimension = "cpu",
      .filterAllowListSize = 0,
      .filterBlockListSize = 1};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
