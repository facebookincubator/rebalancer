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
#include "algopt/rebalancer/materializer/spec_builder/GroupAssignmentAffinitiesSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class GroupAssignmentAffinitiesSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1", "task2", "task3"}},
        {"host1", {}},
        {"host2", {}},
    });

    co_await addPartition(
        "job", {{"job0", {"task0", "task1"}}, {"job1", {"task2", "task3"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  static interface::GroupScopeItemAffinity makeAffinity(
      const std::string& group,
      const std::string& scopeItem,
      int targetDimensionValue,
      double affinity) {
    interface::GroupScopeItemAffinity result;
    result.group() = group;
    result.scopeItem() = scopeItem;
    result.targetDimensionValue() = targetDimensionValue;
    result.affinity() = affinity;
    return result;
  }
};

CO_TEST_F(GroupAssignmentAffinitiesSpecBuilderTest, Goal) {
  interface::GroupAssignmentAffinitiesSpec spec;
  spec.scope() = "host";
  spec.partition() = "job";
  spec.dimension() = "task_count";
  spec.affinities() = {
      makeAffinity("job0", "host1", 2, 10),
      makeAffinity("job1", "host1", 1, 1),
      makeAffinity("job1", "host2", 1, 1),
  };

  const GroupAssignmentAffinitiesSpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  EXPECT_NEAR(11.0, evaluate(goal, deltaFromInitial({})), 1e-8);

  EXPECT_NEAR(
      6.0, evaluate(goal, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  EXPECT_NEAR(
      10.5,
      evaluate(
          goal,
          deltaFromInitial({
              {"task2", "host1"},
              {"task3", "host1"},
          })),
      1e-8);

  EXPECT_NEAR(
      0.0,
      evaluate(
          goal,
          deltaFromInitial({
              {"task0", "host1"},
              {"task1", "host1"},
              {"task2", "host1"},
              {"task3", "host2"},
          })),
      1e-8);
}

TEST_F(GroupAssignmentAffinitiesSpecBuilderTest, Constraint) {
  const interface::GroupAssignmentAffinitiesSpec spec;
  GroupAssignmentAffinitiesSpecBuilder specBuilder(buildUniverse(), spec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder())),
      "not supported as a constraint");
}

TEST_F(GroupAssignmentAffinitiesSpecBuilderTest, Description) {
  interface::GroupAssignmentAffinitiesSpec spec;
  spec.scope() = "host";
  spec.partition() = "job";
  spec.dimension() = "task_count";
  const GroupAssignmentAffinitiesSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "Group assignment affinities of job to host on task_count",
      specBuilder.description());
}

TEST_F(GroupAssignmentAffinitiesSpecBuilderTest, SpecInfo) {
  interface::GroupAssignmentAffinitiesSpec spec;
  spec.scope() = "host";
  spec.partition() = "job";
  spec.dimension() = "task_count";
  spec.name() = "test";
  spec.affinities() = {
      makeAffinity("task0", "host1", 2, 10),
      makeAffinity("task1", "host1", 1, 1),
      makeAffinity("task1", "host2", 1, 1),
  };

  const GroupAssignmentAffinitiesSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "test",
      .scope = "host",
      .partition = "job",
      .dimension = "task_count",
      .size = 3};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
