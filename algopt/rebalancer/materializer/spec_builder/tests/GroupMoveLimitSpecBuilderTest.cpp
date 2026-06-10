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
#include "algopt/rebalancer/materializer/spec_builder/GroupMoveLimitSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class GroupMoveLimitSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task2", "task4"}},
        {"host1", {"task1", "task3"}},
        {"host2", {"task5"}},
    });

    co_await addPartition(
        "job",
        {{"job1", {"task0", "task1", "task2"}},
         {"job2", {"task3", "task4", "task5"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  entities::ObjectId task(int index) const {
    return objectId(fmt::format("task{}", index));
  }

  entities::ScopeId host() const {
    return scopeId("host");
  }
};

CO_TEST_F(GroupMoveLimitSpecBuilderTest, constraint) {
  interface::GroupMoveLimitSpec spec;
  spec.partitionName() = "job";
  spec.limit()->globalLimit() = 2;
  spec.limit()->groupLimits() = {{"job1", 1}};
  const GroupMoveLimitSpecBuilder specBuilder(buildUniverse(), spec);
  EXPECT_EQ(
      "Number of moving objects of partition job w.r.t. dimension task_count must be within limits",
      specBuilder.description());
  auto constraint = co_await specBuilder.constraints(expressionBuilder());
  // constraint is represented as a single ObjectPartitionMoveLimit expression
  EXPECT_EQ(1, constraint.size());
  auto& moveLimitExpr = constraint.at(0);
  // There are already similar unit tests in ExpressionBuilderTest, the test
  // here tries to verify that group limits have been correctly propagated end
  // to end

  // one object of job1 moves, this is within limit
  EXPECT_NEAR(
      0, evaluate(moveLimitExpr, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  // two objects of job2 move, this is also within limit
  EXPECT_NEAR(
      0,
      evaluate(
          moveLimitExpr,
          deltaFromInitial({{"task3", "host0"}, {"task4", "host0"}})),
      1e-8);

  // two object of job1 move, this exceeds limit by 1
  EXPECT_NEAR(
      1,
      evaluate(
          moveLimitExpr,
          deltaFromInitial({{"task0", "host1"}, {"task2", "host1"}})),
      1e-8);

  // three objects of job2 move, this exceeds limit by 1
  EXPECT_NEAR(
      1,
      evaluate(
          moveLimitExpr,
          deltaFromInitial(
              {{"task3", "host0"}, {"task4", "host2"}, {"task5", "host0"}})),
      1e-8);
}

CO_TEST_F(
    GroupMoveLimitSpecBuilderTest,
    constraintWithSourceAndDestinationFilters) {
  interface::GroupMoveLimitSpec spec;
  spec.partitionName() = "job";
  spec.limit()->globalLimit() = 1;
  spec.limit()->groupLimits() = {{"job1", 0}};
  spec.sourceScopeItemsAffectingLimitFilter()->itemsWhitelist() = {
      "host1", "host2"};
  spec.destinationScopeItemsAffectingLimitFilter()->itemsBlacklist() = {
      "host2"};

  const GroupMoveLimitSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraint = co_await specBuilder.constraints(expressionBuilder());

  // constraint is represented as a single ObjectPartitionMoveLimit expression
  EXPECT_EQ(1, constraint.size());
  auto& moveLimitExpr = constraint.at(0);

  // one object of job1 moves, this is within limit (of 0) because moves with
  // "host0" as source do not affect the limit
  EXPECT_NEAR(
      0, evaluate(moveLimitExpr, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  // one object of job1 moves, this is within limit (of 0) since the destination
  // is "host2"
  EXPECT_NEAR(
      0, evaluate(moveLimitExpr, deltaFromInitial({{"task1", "host2"}})), 1e-8);

  // one object of job1 moves, this is not within limit (of 0) as the source is
  // host1 and destination is "host0"
  EXPECT_NEAR(
      1, evaluate(moveLimitExpr, deltaFromInitial({{"task1", "host0"}})), 1e-8);

  // two objects of job2 move, this is within limit (of 1) because one of the
  // moves is from "host0" as source
  EXPECT_NEAR(
      0,
      evaluate(
          moveLimitExpr,
          deltaFromInitial({{"task3", "host2"}, {"task4", "host1"}})),
      1e-8);

  // two objects of job2 move, this is within limit (of 1) because one of the
  // moves is to "host2" as destination
  EXPECT_NEAR(
      0,
      evaluate(
          moveLimitExpr,
          deltaFromInitial({{"task4", "host1"}, {"task5", "host1"}})),
      1e-8);

  // two objects of job2 move, this exceeds limit by 1
  EXPECT_NEAR(
      1,
      evaluate(
          moveLimitExpr,
          deltaFromInitial({{"task3", "host0"}, {"task5", "host0"}})),
      1e-8);
}

TEST_F(GroupMoveLimitSpecBuilderTest, goal) {
  interface::GroupMoveLimitSpec spec;
  spec.partitionName() = "job";
  GroupMoveLimitSpecBuilder specBuilder(buildUniverse(), spec);
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "GroupMoveLimitSpec not supported as goal");
}

TEST_F(GroupMoveLimitSpecBuilderTest, FixedObjects) {
  // Store task IDs before building universe
  auto task0 = task(0);
  auto task1 = task(1);
  auto task2 = task(2);

  interface::GroupMoveLimitSpec spec;
  spec.name() = "test";
  spec.partitionName() = "job";
  spec.limit()->globalLimit() = 2;
  spec.limit()->groupLimits() = {{"job1", 0}};

  const GroupMoveLimitSpecBuilder specBuilder(buildUniverse(), spec);

  // all job1 objects are expected to be classified as 'fixed' since their move
  // limit is 0
  auto computedFixedObjects = specBuilder.fixedObjects();
  auto computedFixedObjectsSet = std::set<entities::ObjectId>(
      computedFixedObjects.begin(), computedFixedObjects.end());
  auto expectedFixedObjectsSet =
      std::set<entities::ObjectId>{task0, task1, task2};
  EXPECT_EQ(expectedFixedObjectsSet, computedFixedObjectsSet);
}

TEST_F(GroupMoveLimitSpecBuilderTest, SpecInfo) {
  interface::GroupMoveLimitSpec spec;
  spec.name() = "test";
  spec.partitionName() = "job";
  spec.limit()->globalLimit() = 2;
  spec.limit()->groupLimits() = {{"job1", 1}};

  const GroupMoveLimitSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "test",
      .partition = "job",
      .dimension = "task_count",
      .limitType = "ABSOLUTE"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

CO_TEST_F(GroupMoveLimitSpecBuilderTest, SpecInfoWithCustomDimension) {
  co_await addObjectDimension(
      "network", entities::Map<std::string, double>{}, 0);

  interface::GroupMoveLimitSpec spec;
  spec.name() = "test";
  spec.partitionName() = "job";
  spec.limit()->globalLimit() = 2;
  spec.limit()->groupLimits() = {{"job1", 1}};
  spec.dimension() = "network";
  const GroupMoveLimitSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "test",
      .partition = "job",
      .dimension = "network",
      .limitType = "ABSOLUTE"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

CO_TEST_F(GroupMoveLimitSpecBuilderTest, Dimension) {
  co_await addObjectDimension("weight", {{task(0), 2}, {task(3), 3}}, 1);

  interface::GroupMoveLimitSpec spec;
  spec.partitionName() = "job";
  spec.dimension() = "weight";
  spec.limit()->globalLimit() = 2;
  spec.limit()->groupLimits() = {{"job1", 1}};
  const GroupMoveLimitSpecBuilder specBuilder(buildUniverse(), spec);

  auto constraint = co_await specBuilder.constraints(expressionBuilder());
  // constraint is represented as a single ObjectPartitionMoveLimit expression
  EXPECT_EQ(1, constraint.size());
  auto& moveLimitExpr = constraint.at(0);

  // task1 has a weight of 1, which is equal to job1's limit of 1
  EXPECT_NEAR(
      0, evaluate(moveLimitExpr, deltaFromInitial({{"task1", "host1"}})), 1e-8);
  // task1 has a weight of 2, which is over job1's limit of 1
  EXPECT_NEAR(
      1, evaluate(moveLimitExpr, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  // Both task4 and task5 have a weight of 1 and the sum of weights is 2, which
  // is same as job2's limit of 2
  EXPECT_NEAR(
      0,
      evaluate(
          moveLimitExpr,
          deltaFromInitial({{"task4", "host0"}, {"task5", "host0"}})),
      1e-8);
  // task3 has a weight of 3, which is over job2's limit of 2
  EXPECT_NEAR(
      1, evaluate(moveLimitExpr, deltaFromInitial({{"task3", "host0"}})), 1e-8);
  // weight of (task3) + weight of (task5) - limit of job2 = 2
  EXPECT_NEAR(
      2,
      evaluate(
          moveLimitExpr,
          deltaFromInitial({{"task3", "host0"}, {"task5", "host0"}})),
      1e-8);
}

CO_TEST_F(GroupMoveLimitSpecBuilderTest, DynamicDimension) {
  co_await addDynamicObjectDimension(
      "weight",
      host(),
      {{"host0",
        makeSharedPtrEntityToValueMap<entities::ObjectId>(
            {{task(0), 1}, {task(1), 1}})},
       {"host1",
        makeSharedPtrEntityToValueMap<entities::ObjectId>(
            {{task(0), 3}, {task(1), 2}, {task(4), 2}})}},
      1);

  interface::GroupMoveLimitSpec spec;
  spec.partitionName() = "job";
  spec.dimension() = "weight";
  spec.limit()->globalLimit() = 2;
  spec.limit()->groupLimits() = {{"job1", 1}};
  const GroupMoveLimitSpecBuilder specBuilder(buildUniverse(), spec);

  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  // constraint is represented as a single ObjectPartitionMoveLimit expression
  EXPECT_EQ(1, constraints.size());
  auto& moveLimitExpr = constraints.at(0);

  // task0 has a weight of 3 for destination host host1
  EXPECT_NEAR(
      2, evaluate(moveLimitExpr, deltaFromInitial({{"task0", "host1"}})), 1e-8);
  // task1 has a weight of 2 for source host host1 and weight of 1 for
  // destination host host0. Since we use maximum of both weights, delta should
  // be 1
  EXPECT_NEAR(
      1, evaluate(moveLimitExpr, deltaFromInitial({{"task1", "host0"}})), 1e-8);
  EXPECT_NEAR(
      0, evaluate(moveLimitExpr, deltaFromInitial({{"task2", "host1"}})), 1e-8);

  // task4 has a weight of 2 for host1 and task5 has a default weight of 1.
  // job1's limit is 2, so we expect a delta of 1
  EXPECT_NEAR(
      1,
      evaluate(
          moveLimitExpr,
          deltaFromInitial({{"task4", "host1"}, {"task5", "host0"}})),
      1e-8);
}

} // namespace facebook::rebalancer::materializer::tests
