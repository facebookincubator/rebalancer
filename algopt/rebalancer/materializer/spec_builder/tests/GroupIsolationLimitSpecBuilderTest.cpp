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

#include "algopt/rebalancer/materializer/spec_builder/GroupIsolationLimitSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class GroupIsolationLimitSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1", "task2"}},
        {"host1", {"task3", "task4", "task5"}},
    });

    co_await addPartition(
        "job",
        {{"job0", {"task0", "task1", "task2"}},
         {"job1", {"task3", "task4", "task5"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(GroupIsolationLimitSpecBuilderTest, constraintFullyIsolated) {
  interface::GroupIsolationLimitSpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.groupsAllowed() = 1;
  spec.limit()->globalLimit() = 0;

  // when groupsAllowed >= 1, do not evaluate lp expression when
  // minimizing=false as that is not supported yet
  packer::tests::LpAssertOptions assertOptions = {
      .exceptionOnlyForLpExprMax =
          "ObjectPartitionLookup: groupsAllowed_ > 0 when minimizing=false is not supported in LP"};

  const GroupIsolationLimitSpecBuilder specBuilder(buildUniverse(), spec);
  EXPECT_EQ(
      "Items of scope host can have at most 1 limit violating group(s) of partition job",
      specBuilder.description());

  // initially the constraint is satisfied, both groups are isolated
  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  // Constraint order depends on hash map iteration; identify by behavior.
  // host0 constraint: becomes 1 when task3 (from host1/job1) moves to host0
  std::optional<size_t> host0Ctr = -1;
  std::optional<size_t> host1Ctr = -1;
  for (const auto i : folly::irange(constraints.size())) {
    const double val = evaluate(
        constraints.at(i),
        deltaFromInitial({{"task3", "host0"}}),
        /*evaluateConstraintExpr=*/true,
        assertOptions);
    if (std::abs(val - 1) < 1e-8) {
      host0Ctr = i;
    } else {
      host1Ctr = i;
    }
  }
  EXPECT_TRUE(host0Ctr.has_value());
  EXPECT_TRUE(host1Ctr.has_value());

  EXPECT_NEAR(
      0,
      evaluate(
          constraints.at(*host0Ctr),
          deltaFromInitial({}),
          /*evaluateConstraintExpr=*/true,
          assertOptions),
      1e-8);
  EXPECT_NEAR(
      1,
      evaluate(
          constraints.at(*host0Ctr),
          deltaFromInitial({{"task3", "host0"}}),
          /*evaluateConstraintExpr=*/true,
          assertOptions),
      1e-8);
  EXPECT_NEAR(
      0,
      evaluate(
          constraints.at(*host1Ctr),
          deltaFromInitial({}),
          /*evaluateConstraintExpr=*/true,
          assertOptions),
      1e-8);
  EXPECT_NEAR(
      1,
      evaluate(
          constraints.at(*host1Ctr),
          deltaFromInitial({{"task2", "host1"}}),
          /*evaluateConstraintExpr=*/true,
          assertOptions),
      1e-8);
}

CO_TEST_F(GroupIsolationLimitSpecBuilderTest, constraintIsolationLimit) {
  interface::GroupIsolationLimitSpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.groupsAllowed() = 1;
  spec.limit()->globalLimit() = 0;
  // job1 will be violated in a scopeItem if its count exceeds 1
  spec.limit()->groupLimits() = {{"job1", 1}};

  const GroupIsolationLimitSpecBuilder specBuilder(buildUniverse(), spec);

  // initially the constraint is satisfied, both groups are isolated
  auto constraints = co_await specBuilder.constraints(expressionBuilder());

  // when groupsAllowed >= 1, do not evaluate lp expression when
  // minimizing=false as that is not supported yet
  packer::tests::LpAssertOptions assertOptions = {
      .exceptionOnlyForLpExprMax =
          "ObjectPartitionLookup: groupsAllowed_ > 0 when minimizing=false is not supported in LP"};

  // Constraint order depends on hash map iteration; identify by behavior.
  std::optional<int> host0Ctr;
  std::optional<int> host1Ctr;
  for (const auto i : folly::irange(constraints.size())) {
    const double val = evaluate(
        constraints.at(i),
        deltaFromInitial({{"task3", "host0"}, {"task4", "host0"}}),
        /*evaluateConstraintExpr=*/true,
        assertOptions);
    if (std::abs(val - 1) < 1e-8) {
      host0Ctr = i;
    } else {
      host1Ctr = i;
    }
  }
  EXPECT_TRUE(host0Ctr.has_value());
  EXPECT_TRUE(host1Ctr.has_value());

  // initially host0 consists of 3 tasks of job0, this is within limits
  EXPECT_NEAR(
      0,
      evaluate(
          constraints.at(*host0Ctr),
          deltaFromInitial({}),
          /*evaluateConstraintExpr=*/true,
          assertOptions),
      1e-8);
  // host0 consists of 3 tasks of job0 and 1 of job1, this is also within limits
  EXPECT_NEAR(
      0,
      evaluate(
          constraints.at(*host0Ctr),
          deltaFromInitial({{"task3", "host0"}}),
          /*evaluateConstraintExpr=*/true,
          assertOptions),
      1e-8);
  // host0 consists of 3 tasks of job0 and 2 of job1, this violates limit of
  // both job0 and job1 but at most one violation is allowed
  EXPECT_NEAR(
      1,
      evaluate(
          constraints.at(*host0Ctr),
          deltaFromInitial({{"task3", "host0"}, {"task4", "host0"}}),
          /*evaluateConstraintExpr=*/true,
          assertOptions),
      1e-8);
  // host1 consists of 3 tasks of job1 and 1 of job0, this violates limit of
  // both job0 and job1 but at most one violation is allowed
  EXPECT_NEAR(
      1,
      evaluate(
          constraints.at(*host1Ctr),
          deltaFromInitial({{"task2", "host1"}}),
          /*evaluateConstraintExpr=*/true,
          assertOptions),
      1e-8);
}

CO_TEST_F(GroupIsolationLimitSpecBuilderTest, goal) {
  interface::GroupIsolationLimitSpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.groupsAllowed() = 1;
  spec.limit()->globalLimit() = 0;
  // test limit overrides
  spec.limit()->scopeItemToGroupLimits() = {{"host0", {{"job1", 1}}}};

  const GroupIsolationLimitSpecBuilder specBuilder(buildUniverse(), spec);

  // initially the constraint is satisfied, both groups are isolated
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // when groupsAllowed >= 1, do not evaluate lp expression when
  // minimizing=false as that is not supported yet
  packer::tests::LpAssertOptions assertOptions = {
      .exceptionOnlyForLpExprMax =
          "ObjectPartitionLookup: groupsAllowed_ > 0 when minimizing=false is not supported in LP"};

  EXPECT_NEAR(0, evaluate(goal, deltaFromInitial({}), assertOptions), 1e-8);
  // job1 has violation of weight 1 on host0, which is within limit.
  // job0 has violation of weight 1 on host1, which is over limit.
  EXPECT_NEAR(
      1,
      evaluate(
          goal,
          deltaFromInitial({{"task2", "host1"}, {"task3", "host0"}}),
          assertOptions),
      1e-8);
}

TEST_F(GroupIsolationLimitSpecBuilderTest, SpecInfo) {
  interface::GroupIsolationLimitSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.groupsAllowed() = 1;
  spec.limit()->globalLimit() = 0;

  const GroupIsolationLimitSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "test",
      .scope = "host",
      .partition = "job",
      .limitType = "ABSOLUTE",
      .filterAllowListSize = 0,
      .filterBlockListSize = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
