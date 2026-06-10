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
#include "algopt/rebalancer/materializer/spec_builder/FlowSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class FlowSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1"}},
        {"host1", {}},
        {"host2", {}},
    });

    co_await addObjectDimension("cpu", {{task(0), 20}, {task(1), 40}}, 0);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  entities::ObjectId task(int index) const {
    return objectId(fmt::format("task{}", index));
  }
};

TEST_F(FlowSpecBuilderTest, Description) {
  interface::FlowSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";

  const FlowSpecBuilder specBuilder(buildUniverse(), spec);
  EXPECT_EQ(
      "limit flow capacity (cpu) on scope host", specBuilder.description());
}

TEST_F(FlowSpecBuilderTest, SpecInfo) {
  interface::FlowSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";

  const FlowSpecBuilder specBuilder(buildUniverse(), spec);
  auto expectedSpecInfo = SpecParameters{
      .name = "",
      .scope = "host",
      .dimension = "cpu",
      .boundType = "UPPER",
      .limitType = "ABSOLUTE"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

CO_TEST_F(FlowSpecBuilderTest, Constraint) {
  interface::FlowSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  {
    interface::ObjectPair pair;
    pair.object1() = "task0";
    pair.object2() = "task1";
    spec.pairs()->push_back(pair);
  }
  {
    auto& limits = *spec.limit()->scopeItemToGroupLimits();
    limits["host0"]["host1"] = 5;
  }
  spec.sourceFilter()->itemsWhitelist() = {"host0"};
  {
    auto& destinationFilters = *spec.destinationFilter();
    destinationFilters["host0"].itemsWhitelist() = {"host1"};
  }
  {
    auto& coefficients = *spec.coefficients()->scopeItemToGroupLimits();
    coefficients["host0"]["host1"] = 10;
  }

  const auto universe = buildUniverse();
  const FlowSpecBuilder specBuilder(universe, spec);
  auto constraint = co_await specBuilder.constraints(expressionBuilder());

  EXPECT_EQ(1, constraint.size());

  auto normalized = max(0, constraint.at(0).constraintExpr, universe);

  EXPECT_NEAR(0, evaluate(normalized, deltaFromInitial({})), 1e-8);
  EXPECT_NEAR(
      0, evaluate(normalized, deltaFromInitial({{"task0", "host1"}})), 1e-8);
  EXPECT_NEAR(
      150, evaluate(normalized, deltaFromInitial({{"task1", "host1"}})), 1e-8);
  EXPECT_NEAR(
      0, evaluate(normalized, deltaFromInitial({{"task1", "host2"}})), 1e-8);
  EXPECT_NEAR(
      0,
      evaluate(
          normalized,
          deltaFromInitial({{"task0", "host1"}, {"task1", "host1"}})),
      1e-8);
}
} // namespace facebook::rebalancer::materializer::tests
