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
#include "algopt/rebalancer/materializer/spec_builder/ToFreeSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class ToFreeSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1", "task2"}},
        {"host1", {"task3"}},
    });
    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  entities::ObjectId task(int index) const {
    return objectId(fmt::format("task{}", index));
  }
};

INSTANTIATE_TEST_CASE_P(
    DiscreteExpressions,
    ToFreeSpecBuilderTest,
    ::testing::Values(false));

INSTANTIATE_TEST_CASE_P(
    ContinuousExpressions,
    ToFreeSpecBuilderTest,
    ::testing::Values(true));

CO_TEST_P(ToFreeSpecBuilderTest, Constraint) {
  interface::ToFreeSpec spec;
  spec.name() = "test";
  spec.containers() = {"host0"};

  const ToFreeSpecBuilder specBuilder(
      buildUniverse(), spec, /*continuousExpressions=*/GetParam());

  EXPECT_EQ(
      "To free 1 containers; 'task_count' dimension; MINIMIZE_TOTAL_UTILIZATION formula",
      specBuilder.description());
  EXPECT_EQ(
      (SpecParameters{.name = "test", .size = 1}), specBuilder.getSpecInfo());

  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  auto root = any_positive(constraints);

  // host 0 has task 0,1,2 hence it will break constraint.
  EXPECT_NEAR(1, evaluate(root, deltaFromInitial({})), 1e-8);

  // host 0 still has task 1, hence it will break the constraint
  EXPECT_NEAR(
      1,
      evaluate(
          root, deltaFromInitial({{"task0", "host1"}, {"task2", "host1"}})),
      1e-8);

  // host 0 still has no task left hence constraint will be satisfied.
  EXPECT_NEAR(
      0,
      evaluate(
          root,
          deltaFromInitial(
              {{"task0", "host1"}, {"task1", "host1"}, {"task2", "host1"}})),
      1e-8);
}

CO_TEST_P(ToFreeSpecBuilderTest, ToFreeAsGoalDefault) {
  interface::ToFreeSpec spec;
  spec.name() = "test to free goal";
  spec.containers() = {"host0"};

  const ToFreeSpecBuilder specBuilder(
      buildUniverse(), spec, /*continuousExpressions=*/GetParam());
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  EXPECT_EQ(
      "To free 1 containers; 'task_count' dimension; MINIMIZE_TOTAL_UTILIZATION formula",
      specBuilder.description());

  EXPECT_EQ(
      (SpecParameters{.name = "test to free goal", .size = 1}),
      specBuilder.getSpecInfo());

  // In the assignment defined in SetUp(), "host0" has three objects. Hence,
  // the goal value is 3
  EXPECT_NEAR(3.0, evaluate(goal, deltaFromInitial({})), 1e-8);

  // if we move "task0" to "host1", then goal value is 2.0
  EXPECT_NEAR(
      2.0, evaluate(goal, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  // If we move all the objects from "host0" to "host1", then goal value will be
  // zero
  EXPECT_NEAR(
      0.0,
      evaluate(
          goal,
          deltaFromInitial({
              {"task0", "host1"},
              {"task1", "host1"},
              {"task2", "host1"},
          })),
      1e-8);

  // If we move "task0", "task1", "task2" to "host1", and move "task3" to
  // "host0", then the value of the goal is 1
  EXPECT_NEAR(
      1.0,
      evaluate(
          goal,
          deltaFromInitial({
              {"task0", "host1"},
              {"task1", "host1"},
              {"task2", "host1"},
              {"task3", "host0"},
          })),
      1e-8);
}

CO_TEST_P(ToFreeSpecBuilderTest, ToFreeWithMinimizeOccupiedContainers) {
  // all objects except task0 have a value of 1.5; task0 has a value of 0
  co_await addObjectDimension("important_tasks", {{task(0), 0.0}}, 1.5);

  interface::ToFreeSpec spec;
  spec.name() = "test to free goal with minimizeOccupiedContainers";
  spec.containers() = {"host0"};
  spec.dimension() = "important_tasks";
  spec.formula() = interface::ToFreeSpecFormula::MINIMIZE_OCCUPIED_CONTAINERS;

  const bool isContinuousExpressions = GetParam();

  const ToFreeSpecBuilder specBuilder(
      buildUniverse(), spec, isContinuousExpressions);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  EXPECT_EQ(
      "To free 1 containers; 'important_tasks' dimension; MINIMIZE_OCCUPIED_CONTAINERS formula",
      specBuilder.description());

  {
    // In the assignment defined in SetUp(), "host0" has three objects.
    auto eval = evaluate(goal, deltaFromInitial({}));
    if (isContinuousExpressions) {
      // when using continuous expressions (e.g., with local search) goal value
      // is 1.1/1.5 * 3  - 1/4.5^2 * 3^2 = 1.7555
      EXPECT_NEAR(1.1 / 1.5 * 3 - pow(3.0 / 4.5, 2), eval, 1e-8);
    } else {
      // when using discrete expressions (e.g., optimal solver) goal value is 1
      // since only one container is occupied
      EXPECT_NEAR(1.0, eval, 1e-8);
    }
  }

  { // If we move task3 to host0, objective gets worse for local search
    auto eval = evaluate(goal, deltaFromInitial({{"task3", "host0"}}));
    if (isContinuousExpressions) {
      // goal value is (1.1 / 1.5 * 4.5) - (1 / 4.5^2 * 4.5^2) ~= 2.3
      EXPECT_NEAR(2.3, eval, 1e-8);
    } else {
      EXPECT_NEAR(1.0, eval, 1e-8);
    }
  }

  {
    // move "task1" to "host1"
    auto eval = evaluate(goal, deltaFromInitial({{"task1", "host1"}}));
    if (isContinuousExpressions) {
      // when using continuous expressions goal value is 1.1/1.5 * 1.5 -
      // (1.5/4.5)^2
      EXPECT_NEAR(1.1 / 1.5 * 1.5 - pow(1.5 / 4.5, 2), eval, 1e-8);
    } else {
      // when using discrete expressions goal value is still 1
      EXPECT_NEAR(1.0, eval, 1e-8);
    }
  }

  {
    // If we move "task1" and "task2"" from "host0" to "host1", then goal value
    // will be zero
    EXPECT_NEAR(
        0.0,
        evaluate(
            goal,
            deltaFromInitial({
                {"task1", "host1"},
                {"task2", "host1"},
            })),
        1e-8);
  }

  {
    // If we move all the objects from "host0" to "host1", then goal value will
    // still be zero
    EXPECT_NEAR(
        0.0,
        evaluate(
            goal,
            deltaFromInitial({
                {"task0", "host1"},
                {"task1", "host1"},
                {"task2", "host1"},
            })),
        1e-8);
  }
}

CO_TEST_P(
    ToFreeSpecBuilderTest,
    ToFreeWithMinimizeOccupiedContainersOneRelevantNonZeroObject) {
  // this test is to ensure that even when there is only one object in
  // ToFreeContainer that has non-zero value, there is an incentive to move that
  // object outside.

  // only one object in containerToFree has a value greater than 0.0; all others
  // objects have value of 0.0
  co_await addObjectDimension("zero_tasks", {{task(2), 2.0}}, 0.0);

  interface::ToFreeSpec spec;
  spec.containers() = {"host0"};
  spec.dimension() = "zero_tasks";
  spec.formula() = interface::ToFreeSpecFormula::MINIMIZE_OCCUPIED_CONTAINERS;

  const bool isContinuousExpressions = GetParam();

  const ToFreeSpecBuilder specBuilder(
      buildUniverse(), spec, isContinuousExpressions);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  {
    auto eval = evaluate(goal, deltaFromInitial({}));
    if (isContinuousExpressions) {
      //  When using continuous expressions goal value
      //  is 1.1/2.0 * 2.0  - 1.0/2.0^2 * 2.0^2 = 0.1
      EXPECT_NEAR(0.1, eval, 1e-8);
    } else {
      // using discrete expressions (e.g., optimal solver) goal value is 1 since
      // one container is occupied
      EXPECT_NEAR(1.0, eval, 1e-8);
    }
  }

  {
    // move "task2" to "host1"; objective value should be zero
    EXPECT_NEAR(
        0.0, evaluate(goal, deltaFromInitial({{"task2", "host1"}})), 1e-8);
  }
}

CO_TEST_P(
    ToFreeSpecBuilderTest,
    ToFreeWithMinimizeOccupiedContainersAllRelevantObjectsZero) {
  co_await addObjectDimension("zero_tasks", {{task(3), 2.0}}, 0.0);

  interface::ToFreeSpec spec;
  spec.containers() = {"host0"};
  spec.dimension() = "zero_tasks";
  spec.formula() = interface::ToFreeSpecFormula::MINIMIZE_OCCUPIED_CONTAINERS;

  const bool isContinuousExpressions = GetParam();

  const ToFreeSpecBuilder specBuilder(
      buildUniverse(), spec, isContinuousExpressions);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // In the assignment defined in SetUp(), "host0" has three objects, but all
  // those objects have zero value. When using continuous expressions (e.g.,
  // with local search) goal value is 1.1/2.0 * 0  - 1.0/2.0^2 * 0^2; when using
  // discrete expressions (e.g., optimal solver) goal value is 0 since only one
  // container that is occupied has zero util
  EXPECT_NEAR(0.0, evaluate(goal, deltaFromInitial({})), 1e-8);
}

CO_TEST_P(
    ToFreeSpecBuilderTest,
    ToFreeWithMinimizeOccupiedContainersAllObjectsZero) {
  // all objects have a value of 0.0
  co_await addObjectDimension(
      "zero_tasks", entities::Map<std::string, double>{}, 0.0);

  interface::ToFreeSpec spec;
  spec.containers() = {"host0"};
  spec.dimension() = "zero_tasks";
  spec.formula() = interface::ToFreeSpecFormula::MINIMIZE_OCCUPIED_CONTAINERS;

  const bool isContinuousExpressions = GetParam();

  const ToFreeSpecBuilder specBuilder(
      buildUniverse(), spec, isContinuousExpressions);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // In the assignment defined in SetUp(), "host0" has three objects, but all
  // objects have zero value. so expect zero goal value
  EXPECT_NEAR(0.0, evaluate(goal, deltaFromInitial({})), 1e-8);
}

TEST_P(
    ToFreeSpecBuilderTest,
    ToFreeErrorWhenNonDefaultFormulaUsedWithConstraint) {
  interface::ToFreeSpec spec;
  spec.containers() = {"host0"};
  spec.formula() = interface::ToFreeSpecFormula::MINIMIZE_OCCUPIED_CONTAINERS;

  ToFreeSpecBuilder specBuilder(
      buildUniverse(), spec, /*continuousExpressions=*/GetParam());
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder())),
      "Choosing a formula is not supported when using ToFreeSpec as a constraint;"
      "only the default formula ToFreeSpecFormula::MINIMIZE_TOTAL_UTILIZATION is supported");
}

CO_TEST_F(ToFreeSpecBuilderTest, ToFreeErrorNegativeDimensionValues) {
  // all objects except task1 have a value of 1.0; task1 has a value of -2.0
  co_await addObjectDimension("unimportant_tasks", {{task(1), -2.0}}, 1.0);

  interface::ToFreeSpec spec;
  spec.containers() = {"host0"};
  spec.dimension() = "unimportant_tasks";
  spec.formula() = interface::ToFreeSpecFormula::MINIMIZE_OCCUPIED_CONTAINERS;

  ToFreeSpecBuilder specBuilder(
      buildUniverse(), spec, /*continuousExpressions=*/true);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "Negative dimension values are not supported when using ToFreeSpec with MINIMIZE_OCCUPIED_CONTAINERS formula");
}

TEST_P(ToFreeSpecBuilderTest, ToFreeWithEmptyContainersList) {
  // Test that when the ToFreeSpec has no containers to free, the goal
  // evaluates to 0 without throwing an exception
  interface::ToFreeSpec spec;
  spec.name() = "test with no containers";
  spec.containers() = {};
  spec.formula() = interface::ToFreeSpecFormula::MINIMIZE_OCCUPIED_CONTAINERS;

  const ToFreeSpecBuilder specBuilder(
      buildUniverse(), spec, /*continuousExpressions=*/GetParam());

  EXPECT_EQ(
      "To free 0 containers; 'task_count' dimension; MINIMIZE_OCCUPIED_CONTAINERS formula",
      specBuilder.description());

  auto goal =
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder()));
  // When there are no containers to free, the goal should evaluate to 0
  EXPECT_NEAR(0.0, evaluate(goal, deltaFromInitial({})), 1e-8);

  // Goal should remain 0 even if we move objects around
  EXPECT_NEAR(
      0.0, evaluate(goal, deltaFromInitial({{"task0", "host1"}})), 1e-8);
}

} // namespace facebook::rebalancer::materializer::tests
