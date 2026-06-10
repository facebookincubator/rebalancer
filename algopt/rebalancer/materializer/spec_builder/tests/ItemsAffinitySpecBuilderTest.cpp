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
#include "algopt/rebalancer/materializer/spec_builder/ItemsAffinitySpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include <algopt/rebalancer/entities/Map.h>

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class ItemsAffinitySpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host0", {"task0", "task1", "task2", "task3"}},
         {"host1", {"task4", "task5"}},
         {"host2", {}},
         {"host3", {}}});

    co_await addPartition(
        "job",
        {{"job0", {"task0", "task1"}},
         {"job1", {"task2", "task3"}},
         {"job2", {"task4", "task5"}}});

    co_await addObjectDimension(
        "job_count", entities::Map<std::string, double>{}, 0.5);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(ItemsAffinitySpecBuilderTest, Basic) {
  interface::ItemsAffinitySpec spec;
  spec.partitionName() = "job";
  spec.dimension() = "job_count";
  spec.scopeItemsOfType1() = {"host0", "host1"};
  spec.scopeItemsOfType2() = {"host2", "host3"};
  spec.scope() = "host";

  const ItemsAffinitySpecBuilder specBuilder(buildUniverse(), spec);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());
  // 1 expression per group
  EXPECT_EQ(3, goal->children().size());

  // initial assignment : max(2 * 0.5, 0) + max(2 * 0.5, 0) + max(2 * 0.5, 0)
  // all tasks are assigned to only one item type
  EXPECT_EQ(3, evaluate(goal, deltaFromInitial({})));
  // with task1 moving to host2, task(0) has one part assigned to each item type
  //  max(0.5, 0.5) + max(1, 0) + max(1, 0)
  EXPECT_EQ(2.5, evaluate(goal, deltaFromInitial({{"task0", "host2"}})));
  // with task3 moving to host2, task(0) and task(1) has one part assigned to
  // each item type:  max(0.5, 0.5) + max(0.5, 0.5) + max(1, 0)
  EXPECT_EQ(
      2,
      evaluate(
          goal, deltaFromInitial({{"task0", "host2"}, {"task3", "host2"}})));
  // with task5 moving to host3, all tasks have one part assigned to
  // each item type:  max(0.5, 0.5) + max(0.5, 0.5) + max(0.5, 0.5)
  // this is also the optimal value for this goal
  EXPECT_EQ(
      1.5,
      evaluate(
          goal,
          deltaFromInitial(
              {{"task0", "host2"}, {"task3", "host2"}, {"task5", "host3"}})));

  // following arrangements also achieve the optimal value
  EXPECT_EQ(
      1.5,
      evaluate(
          goal,
          deltaFromInitial(
              {{"task0", "host2"}, {"task3", "host2"}, {"task5", "host2"}})));

  EXPECT_EQ(
      1.5,
      evaluate(
          goal,
          deltaFromInitial(
              {{"task0", "host3"}, {"task3", "host3"}, {"task5", "host3"}})));
}

TEST_F(ItemsAffinitySpecBuilderTest, SpecInfo) {
  interface::ItemsAffinitySpec spec;
  spec.name() = "abc";
  spec.partitionName() = "job";
  spec.dimension() = "job_count";
  spec.scopeItemsOfType1() = {"host0", "host1"};
  spec.scopeItemsOfType2() = {"host2", "host3"};
  spec.scope() = "host";

  const ItemsAffinitySpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "abc",
      .scope = "host",
      .partition = "job",
      .dimension = "job_count"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
