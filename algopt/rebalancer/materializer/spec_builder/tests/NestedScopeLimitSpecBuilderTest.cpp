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
#include "algopt/rebalancer/materializer/spec_builder/NestedScopeLimitSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class NestedScopeLimitSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host0", {"task0", "task1", "task2", "task3"}},
         {"host1", {}},
         {"host2", {}},
         {"host3", {}}});

    co_await addScope(
        "rack", {{"rack0", {"host0", "host1"}}, {"rack1", {"host2", "host3"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(NestedScopeLimitSpecBuilderTest, Constraint) {
  // ensures that tasks assigned to hosts are balanced across racks
  // Specifically, tasks assigned on each host is at most 50% of task assigned
  // on each rack.
  interface::NestedScopeLimitSpec spec;
  spec.scope() = "host";
  spec.outerScope() = "rack";
  spec.dimension() = "task_count";
  spec.limit()->globalLimit() = 0.5;

  const NestedScopeLimitSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  // one constraint per scopeItem
  EXPECT_EQ(4, constraints.size());

  auto satisfied = any_positive(constraints);
  // initially host0 has all the tasks, that is 100% of rack0 tasks are assigned
  // to host0, so for host0 this constraint is violated.
  // 4 - 0.5 * 4 = -2 for host0, rest 0
  EXPECT_NEAR(1, evaluate(satisfied, deltaFromInitial({})), 1e-8);

  EXPECT_NEAR(
      1,
      evaluate(
          satisfied,
          deltaFromInitial({{"task2", "host2"}, {"task3", "host2"}})),
      1e-8);

  // Now allocations are balanced across the parent scope : rack
  EXPECT_NEAR(
      0,
      evaluate(
          satisfied,
          deltaFromInitial(
              {{"task1", "host1"}, {"task2", "host2"}, {"task3", "host3"}})),
      1e-8);
}

CO_TEST_F(NestedScopeLimitSpecBuilderTest, ConstraintWithFilter) {
  // ensures that tasks assigned to hosts are balanced across racks
  // Specifically, tasks assigned on each host is at most 50% of task assigned
  // on each rack.
  interface::NestedScopeLimitSpec spec;
  spec.scope() = "host";
  spec.outerScope() = "rack";
  spec.dimension() = "task_count";
  spec.limit()->globalLimit() = 0.5;
  spec.limit()->scopeItemLimits() = {{"host0", 1}};
  spec.filter()->itemsWhitelist() = {"host0", "host1"};

  const NestedScopeLimitSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  // one constraint per scopeItem ( 2 were filtered)
  EXPECT_EQ(2, constraints.size());

  auto satisfied = any_positive(constraints);

  // use non-default tolerances since to otherwise the xpress/gurobi is
  // encountering numerical issues
  auto lpAssertOptions = packer::tests::LpAssertOptions{
      .lpTolerances =
          algopt::lp::Tolerances{.constraint = 1.e-7, .integer = 1e-6}};

  // initially host0 has all the tasks, that is 100% of rack0 tasks are assigned
  // to host0, but it is allowed to have that, so initially no constrainst were
  // violated
  EXPECT_NEAR(
      0, evaluate(satisfied, deltaFromInitial({}), lpAssertOptions), 1e-8);

  // This assignment is also ok because the host3 is filtered
  EXPECT_NEAR(
      0,
      evaluate(
          satisfied,
          deltaFromInitial(
              {{"task0", "host3"},
               {"task1", "host3"},
               {"task2", "host3"},
               {"task3", "host3"}}),
          lpAssertOptions),
      1e-8);
}

TEST_F(NestedScopeLimitSpecBuilderTest, SpecInfo) {
  interface::NestedScopeLimitSpec spec;
  spec.scope() = "host";
  spec.outerScope() = "rack";
  spec.dimension() = "task_count";
  spec.limit()->globalLimit() = 0.5;

  const NestedScopeLimitSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "",
      .scope = "host",
      .dimension = "task_count",
      .limitType = "ABSOLUTE",
      .filterAllowListSize = 0,
      .filterBlockListSize = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

} // namespace facebook::rebalancer::materializer::tests
