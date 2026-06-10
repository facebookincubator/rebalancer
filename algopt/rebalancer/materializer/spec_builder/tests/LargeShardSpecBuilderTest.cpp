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
#include "algopt/rebalancer/materializer/spec_builder/LargeShardSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {
class LargeShardSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  entities::ScopeId host() const {
    return scopeId("host");
  }

  folly::coro::Task<void> addDefaultMemoryDimension() {
    const entities::Map<entities::ObjectId, double> objectValues = {
        {objectId("task0"), 0.5},
        {objectId("task1"), 1},
        {objectId("task2"), 2},
        {objectId("task3"), 4},
        {objectId("task4"), 2},
        {objectId("task5"), 6},
        {objectId("task6"), 3},
        {objectId("task7"), 12}};
    co_await addObjectDimension("memory", objectValues, 0);

    co_await addScopeDimension(
        "memory",
        host(),
        {{"host0", 10000}, {"host1", 14}, {"host2", 12}, {"host3", 10}},
        0);

    co_return;
  }

  folly::coro::Task<void> setUpProblem(
      const entities::Map<std::string, std::vector<std::string>>&
          initialAssignment = defaultInitialAssignment_) {
    setUpUniverse(initialAssignment);
    co_await addDefaultMemoryDimension();

    co_return;
  }

  inline static const entities::Map<std::string, std::vector<std::string>>
      defaultInitialAssignment_ = {
          {"host0", {"task0", "task1", "task6", "task7"}},
          {"host1", {"task2"}},
          {"host2", {}},
          {"host3", {"task3", "task4", "task5"}},
      };
};

CO_TEST_F(LargeShardSpecBuilderTest, BasicGoalNoDrainRequired) {
  co_await setUpProblem();

  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.unassignedScopeItemName() = "host0";

  const LargeShardSpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // Expect goal expression to be 0, since host1 and host2 have enough capacity
  // to directly fit the unassigned largeShard (task7)
  EXPECT_NEAR(0.0, evaluate(goal, deltaFromInitial({})), 1e-8);

  // Expect goal expression to increase to (12 - (14 - 2.5)) = 0.5 if for some
  // reason task0 moves into host1 (which is the candidateContainer because
  // of tie-breaking based on containerName)
  EXPECT_NEAR(
      0.5, evaluate(goal, deltaFromInitial({{"task0", "host1"}})), 1e-8);
}

CO_TEST_F(LargeShardSpecBuilderTest, BasicGoalDrainRequired) {
  const entities::Map<std::string, std::vector<std::string>> initialAssignment =
      {
          {"host0", {"task6", "task7"}},
          {"host1", {"task1", "task2"}},
          {"host2", {"task0"}},
          {"host3", {"task3", "task4", "task5"}},
      };

  co_await setUpProblem(initialAssignment);

  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.unassignedScopeItemName() = "host0";

  const LargeShardSpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // We expect host2 to be the candidateContainer/candidateScopeItem, and the
  // goal expression to evaluate to 0.5 as that's the amount of capacity needed
  // to be drained from host2 to fit the unassigned candidateLargeShard (task7)
  EXPECT_NEAR(0.5, evaluate(goal, deltaFromInitial({})), 1e-8);

  // We expect the goalValue to go to zero once the candidateLargeShard (task7)
  // has been allocated to one of containers {host1, host2, host3}
  EXPECT_NEAR(
      0.0, evaluate(goal, deltaFromInitial({{"task7", "host1"}})), 1e-8);
}

CO_TEST_F(LargeShardSpecBuilderTest, NoDrainRequiredToDrainRequired) {
  const entities::Map<std::string, std::vector<std::string>> initialAssignment =
      {
          {"host0", {"task0", "task6", "task7"}},
          {"host1", {"task1", "task2"}},
          {"host2", {}},
          {"host3", {"task3", "task4", "task5"}},
      };

  co_await setUpProblem(initialAssignment);
  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.unassignedScopeItemName() = "host0";

  const LargeShardSpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // We expect host2 to be the candidateContainer/candidateScopeItem, and the
  // goal expression to evaluate to 0.0 as initially it has enough capacity to
  // accommodate the unassigned candidateLargeShard (task7)
  EXPECT_NEAR(0.0, evaluate(goal, deltaFromInitial({})), 1e-8);

  // If for some reason task6 moves to host2 (which is the
  // candidateContainer/candidateScopeItem), then we expect the goal value to
  // increase to 3, since that's the amount of drain required to fit task7
  EXPECT_NEAR(
      3.0, evaluate(goal, deltaFromInitial({{"task6", "host2"}})), 1e-8);

  // If task6 moves to host2 (which is the
  // candidateContainer/candidateScopeItem), but task7 got assigned to host1,
  // then we expect the goal value to be back to 0
  EXPECT_NEAR(
      0.0,
      evaluate(
          goal, deltaFromInitial({{"task6", "host2"}, {"task7", "host1"}})),
      1e-8);
}

CO_TEST_F(LargeShardSpecBuilderTest, PositiveGoalOnlyIfUnassigned) {
  // This test is to check that even if candidateContainer allows exceeding its
  // capacity for whatever reason, the goalValue will only be positive if the
  // candiateLargeShard is unassigned
  const entities::Map<std::string, std::vector<std::string>> initialAssignment =
      {
          {"host0", {"task7"}},
          {"host1", {}},
          {"host2", {"task0", "task1", "task2", "task3", "task4", "task5"}},
          {"host3", {"task6"}},
      };

  setUpUniverse(initialAssignment);

  const entities::Map<entities::ObjectId, double> fakeMemoryObjectValues = {
      {objectId("task6"), 10}, {objectId("task7"), 10}};
  co_await addObjectDimension("fakeMemory", fakeMemoryObjectValues, 2);

  co_await addScopeDimension(
      "fakeMemory", host(), {{"host0", 10000}, {"host1", 0}}, 10);

  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "fakeMemory";
  spec.unassignedScopeItemName() = "host0";

  const LargeShardSpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // First, note that host2 will be the candidateContainer since host1 has zero
  // capacity, and host3 has a large shard. 'availableCapacity' w.r.t. host2 =
  // max(0, (10 -12)) = 0, where 10 is the total capacity of host2 and 12 is its
  // current utilization. Given this, we expect goal expression to be of value
  // (10 - max(0, ())) = 10.
  EXPECT_NEAR(10.0, evaluate(goal, deltaFromInitial({})), 1e-8);

  // If task7 (i.e., the candidateLargeShard) is assigned to host3 for some
  // reason, then we expect the goal value to become 0
  EXPECT_NEAR(
      0.0, evaluate(goal, deltaFromInitial({{"task7", "host3"}})), 1e-8);
}

// Tests for findCandidateScopeItemToDrain()

CO_TEST_F(LargeShardSpecBuilderTest, TestTieBreakingWhenSameDrainMetric) {
  const entities::Map<std::string, std::vector<std::string>> initialAssignment =
      {
          {"host0", {"task7"}},
          {"host1", {"task3", "task4", "task5"}},
          {"host2", {"task0", "task1", "task2"}},
          {"host3", {"task6"}},
      };

  setUpUniverse(initialAssignment);

  const entities::Map<entities::ObjectId, double> memoryObjectValues = {
      {objectId("task0"), 3},
      {objectId("task3"), 3},
      {objectId("task4"), 1},
      {objectId("task7"), 10}};
  co_await addObjectDimension("memory", memoryObjectValues, 0);

  co_await addScopeDimension(
      "memory", host(), {{"host0", 10000}, {"host3", 0}}, 10);

  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.unassignedScopeItemName() = "host0";

  const LargeShardSpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // Both host1 and host2 are potential candidateContainers. Moreoever, they
  // both have the same DrainMetric. So we expect tie to be broken in favor of
  // host1 since we break ties based on containerName and "host1" < "host2".
  // Hence, goal value should be (10 - (10 - 4)) = 4. Note that if
  // candidateContainer were host2, then the value would be (10- (10- 3)) = 3.
  EXPECT_NEAR(4.0, evaluate(goal, deltaFromInitial({})), 1e-8);
}

CO_TEST_F(
    LargeShardSpecBuilderTest,
    TestCandidateContainersWithSameBucketIndex) {
  const entities::Map<std::string, std::vector<std::string>> initialAssignment =
      {
          {"host0", {"task7"}},
          {"host1", {"task3", "task4", "task5", "task6"}},
          {"host2", {"task0", "task1", "task2"}},
          {"host3", {}},
      };

  setUpUniverse(initialAssignment);

  const entities::Map<entities::ObjectId, double> memoryObjectValues = {
      {objectId("task3"), 4},
      {objectId("task0"), 4},
      {objectId("task1"), 4},
      {objectId("task7"), 10}};
  co_await addObjectDimension("memory", memoryObjectValues, 1);

  co_await addScopeDimension(
      "memory", host(), {{"host0", 10000}, {"host3", 0}}, 12);

  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.unassignedScopeItemName() = "host0";

  const LargeShardSpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // Both host1 and host2 are potential candidateContainers. Moreoever, they
  // both have the same maxShardSizeToBeRemoved (4). But choosing host1 would
  // mean having to remove 3 shards, whereas choosing host2 results in removing
  // only 2 shards. So host2 should be the candidateContainer, and we expect
  // the goal value to be (10 - (12 - 9)) = 7. Note that if
  // candidateContainer were host1, then the value would be (10- (12- 7)) = 5.
  EXPECT_NEAR(7.0, evaluate(goal, deltaFromInitial({})), 1e-8);
}

// Test errors/warnings, description(), specInfo()

CO_TEST_F(LargeShardSpecBuilderTest, NotEnoughContainerCapacity) {
  setUpUniverse(defaultInitialAssignment_);
  co_await addDefaultMemoryDimension();

  const entities::Map<entities::ObjectId, double> veryLargeMemoryObjectValues =
      {{objectId("task7"), 16}};
  co_await addObjectDimension(
      "veryLargeMemory", veryLargeMemoryObjectValues, 0);

  co_await addScopeDimension("veryLargeMemory", host(), {{"host0", 10000}}, 10);

  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "veryLargeMemory";
  spec.unassignedScopeItemName() = "host0";

  const LargeShardSpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // Expect goal expression to be 16, since no container other than the
  // unassignedScopeItem (host0) has capacity to accommodate the unassigned
  // largeShard (task7).
  EXPECT_NEAR(16.0, evaluate(goal, deltaFromInitial({})), 1e-8);

  // Also, notice Warning in the logs
}

CO_TEST_F(LargeShardSpecBuilderTest, Description) {
  co_await setUpProblem();

  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.unassignedScopeItemName() = "host0";

  const LargeShardSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "Accommodate a large shard w.r.t. dimension 'memory'",
      specBuilder.description());
}

CO_TEST_F(LargeShardSpecBuilderTest, SpecInfo) {
  co_await setUpProblem();

  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.unassignedScopeItemName() = "host0";

  const LargeShardSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "test",
      .scope = "host",
      .dimension = "memory",
      .filterAllowListSize = 0,
      .filterBlockListSize = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

CO_TEST_F(
    LargeShardSpecBuilderTest,
    ErrorWithObjectDimensionSizeGreaterThanOne) {
  co_await setUpProblem();

  // Create a multi-valued dimension (size > 1) with empty maps
  co_await addObjectDimension("incorrectMemory", {{}, {}}, {0, 0});

  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "incorrectMemory";
  spec.unassignedScopeItemName() = "host0";

  auto universe = buildUniverse();
  REBALANCER_EXPECT_RUNTIME_ERROR(
      const LargeShardSpecBuilder specBuilder(universe, spec),
      "LargeShardSpec is not currently supported when size of given dimension "
      "> 1");
}

CO_TEST_F(LargeShardSpecBuilderTest, ErrorWhenUsedAsConstraint) {
  setUpUniverse(defaultInitialAssignment_);
  co_await addDefaultMemoryDimension();

  interface::LargeShardSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.unassignedScopeItemName() = "host0";
  LargeShardSpecBuilder specBuilder(buildUniverse(), spec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder())),
      "LargeShardSpec is not supported as a constraint");
}
} // namespace facebook::rebalancer::materializer::tests
