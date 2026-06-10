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
#include "algopt/rebalancer/entities/builders/ConstraintsBuilder.h"
#include "algopt/rebalancer/entities/builders/tests/BuilderTestBase.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/portability/GTest.h>

namespace facebook::rebalancer::entities::tests {

class ConstraintsBuilderTest : public BuilderTestBase {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    ConstraintsBuilderTest,
    ::testing::Values(1, 3, 20));

CO_TEST_P(ConstraintsBuilderTest, AddConstraints) {
  ConstraintsBuilder constraintsBuilder(idBuilder_);

  const auto constraint1Id = constraintsBuilder.makeConstraintId("constraint1");
  const auto constraint2Id = constraintsBuilder.makeConstraintId("constraint2");
  const auto constraint3Id = constraintsBuilder.makeConstraintId("constraint3");
  EXPECT_NE(constraint1Id, constraint2Id);
  EXPECT_NE(constraint2Id, constraint3Id);
  EXPECT_NE(constraint1Id, constraint3Id);

  co_await folly::coro::co_withExecutor(
      executor.get(),
      folly::coro::collectAll(
          constraintsBuilder.add(
              constraint1Id,
              ConstraintData{.constraint = makeConstraint(100.0, 0.0, 0)}),
          constraintsBuilder.add(
              constraint2Id,
              ConstraintData{.constraint = makeConstraint(200.0, 1.0, 0)}),
          constraintsBuilder.add(
              constraint3Id,
              ConstraintData{.constraint = makeConstraint(50.0, 0.5, 1)})));

  const auto result = co_await constraintsBuilder.build();

  EXPECT_EQ(3, result.constraintNameToId.size());
  EXPECT_EQ(constraint1Id, result.constraintNameToId.at("constraint1"));
  EXPECT_EQ(constraint2Id, result.constraintNameToId.at("constraint2"));
  EXPECT_EQ(constraint3Id, result.constraintNameToId.at("constraint3"));

  EXPECT_EQ(3, result.constraintIdToConstraint.size());
  EXPECT_TRUE(result.constraintIdToConstraint.contains(constraint1Id));
  EXPECT_TRUE(result.constraintIdToConstraint.contains(constraint2Id));
  EXPECT_TRUE(result.constraintIdToConstraint.contains(constraint3Id));

  EXPECT_DOUBLE_EQ(
      100.0,
      result.constraintIdToConstraint.at(constraint1Id)->getInvalidCost());
  EXPECT_DOUBLE_EQ(
      200.0,
      result.constraintIdToConstraint.at(constraint2Id)->getInvalidCost());
  EXPECT_DOUBLE_EQ(
      50.0,
      result.constraintIdToConstraint.at(constraint3Id)->getInvalidCost());

  EXPECT_DOUBLE_EQ(
      0.0,
      result.constraintIdToConstraint.at(constraint1Id)->getInvalidState());
  EXPECT_DOUBLE_EQ(
      1.0,
      result.constraintIdToConstraint.at(constraint2Id)->getInvalidState());
  EXPECT_DOUBLE_EQ(
      0.5,
      result.constraintIdToConstraint.at(constraint3Id)->getInvalidState());

  EXPECT_EQ(
      0, result.constraintIdToConstraint.at(constraint1Id)->getTupleIndex());
  EXPECT_EQ(
      0, result.constraintIdToConstraint.at(constraint2Id)->getTupleIndex());
  EXPECT_EQ(
      1, result.constraintIdToConstraint.at(constraint3Id)->getTupleIndex());
}

CO_TEST_P(ConstraintsBuilderTest, ConcurrentAdd) {
  ConstraintsBuilder constraintsBuilder(idBuilder_);

  constexpr int nConstraints = 500;
  std::vector<ConstraintId> constraintIds;
  constraintIds.reserve(nConstraints);
  for (const auto i : folly::irange(nConstraints)) {
    constraintIds.push_back(
        constraintsBuilder.makeConstraintId(fmt::format("constraint_{}", i)));
  }

  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve(nConstraints);
  for (const auto i : folly::irange(nConstraints)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&constraintsBuilder,
             constraintId = constraintIds[i],
             i]() -> folly::coro::Task<void> {
              executeRandomDelay();
              const double invalidCost = static_cast<double>(i) * 10.0;
              const double invalidState = static_cast<double>(i) / 1000.0;
              const int tupleIndex = i % 3;
              co_await constraintsBuilder.add(
                  constraintId,
                  ConstraintData{
                      .constraint = makeConstraint(
                          invalidCost, invalidState, tupleIndex)});
            }));
  }

  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  const auto result = co_await constraintsBuilder.build();

  EXPECT_EQ(nConstraints, result.constraintNameToId.size());
  EXPECT_EQ(nConstraints, result.constraintIdToConstraint.size());

  for (const auto i : folly::irange(nConstraints)) {
    const auto constraintName = fmt::format("constraint_{}", i);
    EXPECT_TRUE(result.constraintNameToId.contains(constraintName));
    const auto constraintId = result.constraintNameToId.at(constraintName);
    EXPECT_TRUE(result.constraintIdToConstraint.contains(constraintId));

    const double expectedInvalidCost = static_cast<double>(i) * 10.0;
    const double expectedInvalidState = static_cast<double>(i) / 1000.0;
    const int expectedTupleIndex = i % 3;
    EXPECT_DOUBLE_EQ(
        expectedInvalidCost,
        result.constraintIdToConstraint.at(constraintId)->getInvalidCost());
    EXPECT_DOUBLE_EQ(
        expectedInvalidState,
        result.constraintIdToConstraint.at(constraintId)->getInvalidState());
    EXPECT_EQ(
        expectedTupleIndex,
        result.constraintIdToConstraint.at(constraintId)->getTupleIndex());
  }
}

TEST_P(ConstraintsBuilderTest, DuplicateMakeConstraintIdThrows) {
  ConstraintsBuilder constraintsBuilder(idBuilder_);
  std::ignore = constraintsBuilder.makeConstraintId("constraint1");
  REBALANCER_EXPECT_RUNTIME_ERROR(
      constraintsBuilder.makeConstraintId("constraint1"),
      "Unexpected call to makeConstraintId with a previously added name 'constraint1'");
}

CO_TEST_P(ConstraintsBuilderTest, Summarize) {
  ConstraintsBuilder builder(idBuilder_);
  builder.makeConstraintId("capacity_constraint");
  builder.makeConstraintId("affinity_constraint");

  const auto result = co_await builder.summarize();

  const std::string expected =
      "Constraints:\n"
      "  affinity_constraint\n"
      "  capacity_constraint\n";
  EXPECT_EQ(expected, result);
}

CO_TEST_P(ConstraintsBuilderTest, BuildAddConstraintAndRebuild) {
  ConstraintsBuilder constraintsBuilder(idBuilder_);

  const auto constraint1Id = constraintsBuilder.makeConstraintId("constraint1");
  co_await constraintsBuilder.add(
      constraint1Id,
      ConstraintData{
          .constraint = makeConstraint(
              /*invalidCost=*/100.0,
              /*invalidState=*/0.0,
              /*tupleIndex=*/0)});

  const auto result1 = co_await constraintsBuilder.build();
  EXPECT_EQ(1, result1.constraintNameToId.size());
  EXPECT_EQ(constraint1Id, result1.constraintNameToId.at("constraint1"));
  EXPECT_EQ(1, result1.constraintIdToConstraint.size());
  EXPECT_DOUBLE_EQ(
      100.0,
      result1.constraintIdToConstraint.at(constraint1Id)->getInvalidCost());

  const auto constraint2Id = constraintsBuilder.makeConstraintId("constraint2");
  co_await constraintsBuilder.add(
      constraint2Id,
      ConstraintData{
          .constraint = makeConstraint(
              /*invalidCost=*/200.0,
              /*invalidState=*/1.0,
              /*tupleIndex=*/1)});

  const auto result2 = co_await constraintsBuilder.build();

  // Verify that result1 was not mutated by the second build
  EXPECT_EQ(1, result1.constraintNameToId.size());
  EXPECT_EQ(constraint1Id, result1.constraintNameToId.at("constraint1"));
  EXPECT_EQ(1, result1.constraintIdToConstraint.size());
  EXPECT_DOUBLE_EQ(
      100.0,
      result1.constraintIdToConstraint.at(constraint1Id)->getInvalidCost());

  EXPECT_EQ(2, result2.constraintNameToId.size());
  EXPECT_EQ(constraint2Id, result2.constraintNameToId.at("constraint2"));
  EXPECT_EQ(2, result2.constraintIdToConstraint.size());
  EXPECT_DOUBLE_EQ(
      200.0,
      result2.constraintIdToConstraint.at(constraint2Id)->getInvalidCost());
  EXPECT_EQ(
      0, result2.constraintIdToConstraint.at(constraint1Id)->getTupleIndex());
  EXPECT_EQ(
      1, result2.constraintIdToConstraint.at(constraint2Id)->getTupleIndex());
}

} // namespace facebook::rebalancer::entities::tests
