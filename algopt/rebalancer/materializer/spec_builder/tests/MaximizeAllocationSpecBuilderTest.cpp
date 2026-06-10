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
#include "algopt/rebalancer/materializer/spec_builder/MaximizeAllocationSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class MaximizeAllocationSpecBuilderTest : public SpecBuilderTestBase<> {
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

CO_TEST_F(MaximizeAllocationSpecBuilderTest, Goal) {
  interface::MaximizeAllocationSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "task_count";

  interface::Filter filter;
  filter.itemsBlacklist() = {std::vector<std::string>{"host2"}};

  spec.filter() = filter;
  const MaximizeAllocationSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "For task_count, maximize allocated task across hosts",
      specBuilder.description());

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // host2 is ignored hence value will be -0.75
  // coef = -0.25. goal = -0.5 (host0) + -0.25 (host1)
  EXPECT_NEAR(-0.75, evaluate(goal, deltaFromInitial({})), 1e-8);
}

TEST_F(MaximizeAllocationSpecBuilderTest, Constraint) {
  const interface::MaximizeAllocationSpec spec;
  MaximizeAllocationSpecBuilder specBuilder(buildUniverse(), spec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder())),
      "MaximizeAllocationSpec not supported as a constraint");
}

TEST_F(MaximizeAllocationSpecBuilderTest, SpecInfo) {
  interface::MaximizeAllocationSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "task_count";

  interface::Filter filter;
  filter.itemsBlacklist() = {std::vector<std::string>{"host2"}};
  spec.filter() = filter;

  const MaximizeAllocationSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "test",
      .scope = "host",
      .dimension = "task_count",
      .filterAllowListSize = 0,
      .filterBlockListSize = 1};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
