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
#include "algopt/rebalancer/materializer/spec_builder/ExclusiveObjectsSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class ExclusiveObjectsSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  void SetUp() override {
    setUpUniverse(
        {{"host1", {"task0", "task1"}},
         {"host2", {}},
         {"host3", {"task2", "task3"}},
         {"host4", {"task4"}}});
  }
};

CO_TEST_F(ExclusiveObjectsSpecBuilderTest, Constraint) {
  interface::ObjectPair pair;
  pair.object1() = "task1";
  pair.object2() = "task4";

  interface::ExclusiveObjectsSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.pairs() = {pair};

  const ExclusiveObjectsSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ("Object separation across test", specBuilder.description());

  auto constraint = co_await specBuilder.constraints(expressionBuilder());

  // There should be 4 constraints (one per host)
  EXPECT_EQ(4, constraint.size());

  // Use any_positive to check if any constraint is violated
  auto satisfied = any_positive(constraint);

  // Initially task1 is on host1 and task4 is on host4 (different hosts)
  // So no constraint should be violated (constraint for "separation" is
  // satisfied)
  EXPECT_NEAR(0, evaluate(satisfied, deltaFromInitial({})), 1e-8);

  // When we move task4 to host1, both task1 and task4 will be on host1
  // This violates the separation constraint
  EXPECT_NEAR(
      1, evaluate(satisfied, deltaFromInitial({{"task4", "host1"}})), 1e-8);
}

CO_TEST_F(ExclusiveObjectsSpecBuilderTest, Goal) {
  interface::ObjectPair pair1;
  pair1.object1() = "task1";
  pair1.object2() = "task4";

  interface::ObjectPair pair2;
  pair2.object1() = "task0";
  pair2.object2() = "task2";

  interface::ExclusiveObjectsSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.pairs() = {pair1, pair2};

  const ExclusiveObjectsSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ("Object separation across test", specBuilder.description());

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());
  EXPECT_NEAR(0, evaluate(goal, deltaFromInitial({})), 1e-8);
  // value will be 2 as task1 and task4 will be on host1
  // and task0 and task2 will be on host2.
  EXPECT_NEAR(
      2,
      evaluate(
          goal, deltaFromInitial({{"task4", "host1"}, {"task0", "host3"}})),
      1e-8);
}

CO_TEST_F(ExclusiveObjectsSpecBuilderTest, NotSeparateConstraint) {
  interface::ObjectPair pair;
  pair.object1() = "task1";
  pair.object2() = "task4";

  interface::ExclusiveObjectsSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.pairs() = {pair};
  spec.separate() = false;

  const ExclusiveObjectsSpecBuilder specBuilder(buildUniverse(), spec);
  auto& exprBuilder = expressionBuilder();
  auto constraint = co_await specBuilder.constraints(exprBuilder);

  // we should have 4 constraints in total (one per host)
  EXPECT_EQ(4, constraint.size());

  // Use any_positive to check if any constraint is violated
  auto satisfied = any_positive(constraint);

  // When separate=false, constraint is violated if neither task is on a host.
  // Initially: task1 on host1, task4 on host4, so host2 and host3 violate.
  // So the "any positive" (at least one violation) should be 1.
  EXPECT_NEAR(1, evaluate(satisfied, deltaFromInitial({})), 1e-8);

  // The goal represents total violations (sum of positive constraint values)
  // host1: task1 there → 1 - 1 - 0 = 0
  // host2: neither → 1 - 0 - 0 = 1
  // host3: neither → 1 - 0 - 0 = 1
  // host4: task4 there → 1 - 0 - 1 = 0
  // Total = 2
  auto goal = co_await specBuilder.goalCoro(exprBuilder);
  EXPECT_NEAR(2, evaluate(goal, deltaFromInitial({})), 1e-8);
}

TEST_F(ExclusiveObjectsSpecBuilderTest, SpecInfo) {
  interface::ObjectPair pair;
  pair.object1() = "task1";
  pair.object2() = "task4";

  interface::ExclusiveObjectsSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.pairs() = {pair};
  spec.separate() = false;

  const ExclusiveObjectsSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "test",
      .scope = "host",
      .size = 1,
      .filterAllowListSize = 0,
      .filterBlockListSize = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
