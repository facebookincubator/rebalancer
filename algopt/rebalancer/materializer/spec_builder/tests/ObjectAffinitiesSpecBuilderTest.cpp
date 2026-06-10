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
#include "algopt/rebalancer/materializer/spec_builder/ObjectAffinitiesSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class ObjectAffinitiesSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0"}},
        {"host1", {"task1"}},
        {"host2", {"task2", "task3"}},
        {"host3", {}},
    });

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(ObjectAffinitiesSpecBuilderTest, ObjectBasedConstraint) {
  interface::ObjectAffinitiesSpec spec;
  spec.scope() = "host";
  spec.filter()->itemsBlacklist() = {"host3"};
  {
    interface::ObjectAffinity affinity;
    affinity.object0() = "task0";
    affinity.object1() = "task1";
    affinity.objectsN() = {"task2", "task3"};
    spec.affinities()->push_back(std::move(affinity));
  }

  auto universe = buildUniverse();
  const ObjectAffinitiesSpecBuilder specBuilder(universe, spec);
  auto constraintInfos = co_await specBuilder.constraints(expressionBuilder());
  std::vector<ExprPtr> constraintExprs;
  constraintExprs.reserve(constraintInfos.size());
  for (auto& constraint : constraintInfos) {
    constraintExprs.push_back(constraint.constraintExpr);
  }
  constraintExprs.push_back(const_expr(0, universe));
  auto constraint = max(constraintExprs, universe);

  // Constraint satisfied because both object0 and object1 are assigned to the
  // same scope item.
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host0"},
               {"task1", "host0"},
               {"task2", "host2"},
               {"task3", "host2"}})),
      1e-8);
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host0"},
               {"task1", "host0"},
               {"task2", "host0"},
               {"task3", "host0"}})),
      1e-8);

  // Constraint satisfied because object1 and all of objectsN are assigned to
  // the same scope item.
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host0"},
               {"task1", "host1"},
               {"task2", "host1"},
               {"task3", "host1"}})),
      1e-8);

  // Constraint not satisfied because neither of object0 or all of objectsN are
  // assigned to the same scope item as object1.
  EXPECT_NEAR(
      1.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host0"},
               {"task1", "host1"},
               {"task2", "host2"},
               {"task3", "host2"}})),
      1e-8);
  EXPECT_NEAR(
      1.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host0"},
               {"task1", "host1"},
               {"task2", "host1"},
               {"task3", "host2"}})),
      1e-8);
}

CO_TEST_F(ObjectAffinitiesSpecBuilderTest, ScopeItemBasedConstraint) {
  interface::ObjectAffinitiesSpec spec;
  spec.scope() = "host";
  {
    interface::ObjectAffinity affinity;
    affinity.object0() = "task0";
    affinity.object1() = "host1";
    affinity.objectsN() = {"task2", "task3"};
    spec.affinities()->push_back(std::move(affinity));
  }

  auto universe = buildUniverse();
  const ObjectAffinitiesSpecBuilder specBuilder(universe, spec);
  auto constraintInfos = co_await specBuilder.constraints(expressionBuilder());
  std::vector<ExprPtr> constraintExprs;
  constraintExprs.reserve(constraintInfos.size());
  for (auto& constraint : constraintInfos) {
    constraintExprs.push_back(constraint.constraintExpr);
  }
  constraintExprs.push_back(const_expr(0, universe));
  auto constraint = max(constraintExprs, universe);

  // Constraint satisfied because object0 is assigned to the scope item object1.
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host1"},
               {"task1", "host1"},
               {"task2", "host2"},
               {"task3", "host2"}})),
      1e-8);
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host1"},
               {"task1", "host2"},
               {"task2", "host1"},
               {"task3", "host1"}})),
      1e-8);

  // Constraint satisfied because all of objectsN are assigned to the scope item
  // object1.
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host0"},
               {"task1", "host2"},
               {"task2", "host1"},
               {"task3", "host1"}})),
      1e-8);

  // Constraint not satisfied because neither object0 nor all of objectsN are
  // assigned to the scope item object1.
  EXPECT_NEAR(
      1.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host0"},
               {"task1", "host1"},
               {"task2", "host2"},
               {"task3", "host2"}})),
      1e-8);
  EXPECT_NEAR(
      1.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host0"},
               {"task1", "host1"},
               {"task2", "host1"},
               {"task3", "host2"}})),
      1e-8);
}

CO_TEST_F(
    ObjectAffinitiesSpecBuilderTest,
    ScopeItemBasedFilteredOutConstraint) {
  interface::ObjectAffinitiesSpec spec;
  spec.scope() = "host";
  spec.filter()->itemsBlacklist() = {"host1"};
  {
    interface::ObjectAffinity affinity;
    affinity.object0() = "task0";
    affinity.object1() = "host1";
    affinity.objectsN() = {"task2", "task3"};
    spec.affinities()->push_back(std::move(affinity));
  }

  auto universe = buildUniverse();
  const ObjectAffinitiesSpecBuilder specBuilder(universe, spec);
  auto constraintInfos = co_await specBuilder.constraints(expressionBuilder());
  std::vector<ExprPtr> constraintExprs;
  constraintExprs.reserve(constraintInfos.size());
  for (auto& constraint : constraintInfos) {
    constraintExprs.push_back(constraint.constraintExpr);
  }
  constraintExprs.push_back(const_expr(0, universe));
  auto constraint = max(constraintExprs, universe);

  // Constraint satisfied because scope item object1 is excluded by the filter.
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host0"},
               {"task1", "host1"},
               {"task2", "host2"},
               {"task3", "host2"}})),
      1e-8);

  auto expectedSpecInfo = SpecParameters{
      .name = "",
      .scope = "host",
      .size = 1,
      .filterAllowListSize = 0,
      .filterBlockListSize = 1};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

TEST_F(ObjectAffinitiesSpecBuilderTest, Goal) {
  const interface::ObjectAffinitiesSpec spec;
  ObjectAffinitiesSpecBuilder specBuilder(buildUniverse(), spec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "ObjectAffinitiesSpec not supported as a goal");
}

TEST_F(ObjectAffinitiesSpecBuilderTest, Description) {
  interface::ObjectAffinitiesSpec spec;
  spec.scope() = "host";
  const ObjectAffinitiesSpecBuilder specBuilder(buildUniverse(), spec);
  EXPECT_EQ("Object affinity across host", specBuilder.description());
}

} // namespace facebook::rebalancer::materializer::tests
