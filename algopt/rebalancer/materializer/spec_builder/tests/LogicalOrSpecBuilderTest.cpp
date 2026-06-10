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
#include "algopt/rebalancer/materializer/spec_builder/LogicalOrSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class LogicalOrSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1", "task2", "task3"}},
        {"host1", {"task4", "task5"}},
        {"host2", {"task6"}},
    });

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }

  bool evaluateTruth(
      ExprPtr expression,
      const entities::Map<
          entities::ContainerId,
          std::vector<entities::ObjectId>>& containerToObjects) const {
    auto value = evaluate(expression, containerToObjects);
    return value <= 0;
  }

  void addCapacitySpec(const interface::CapacitySpec& capacitySpec) {
    interface::GenericSpec genericSpec;
    genericSpec.set_capacitySpec(capacitySpec);
    logicalOrSpec.genericSpecs()->push_back(genericSpec);
  }

  static interface::CapacitySpec makeCapacitySpec(
      interface::CapacitySpecBound bound,
      double limit,
      std::optional<std::string> hostName = std::nullopt) {
    interface::CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "task_count";
    capacitySpec.definition() = interface::CapacitySpecDefinition::AFTER;
    capacitySpec.limit()->type() = interface::LimitType::ABSOLUTE;
    capacitySpec.bound() = bound;
    capacitySpec.limit()->globalLimit() = limit;
    if (hostName) {
      capacitySpec.filter()->itemsWhitelist() = {*hostName};
    }
    return capacitySpec;
  }

  LogicalOrSpecBuilder getSpecBuilder() {
    LogicalOrSpecBuilder specBuilder(buildUniverse(), logicalOrSpec);
    return specBuilder;
  }

  folly::coro::Task<std::shared_ptr<Expression>> getSingleConstraintCoro() {
    const auto constraints =
        co_await getSpecBuilder().constraints(expressionBuilder());
    EXPECT_EQ(1, constraints.size());
    const auto& constraint = constraints[0].constraintExpr;
    co_return constraint;
  }

 protected:
  interface::LogicalOrSpec logicalOrSpec;
};

TEST_F(LogicalOrSpecBuilderTest, Description) {
  addCapacitySpec(
      makeCapacitySpec(interface::CapacitySpecBound::MIN, 4, "host0"));
  addCapacitySpec(
      makeCapacitySpec(interface::CapacitySpecBound::MAX, 2, "host1"));

  EXPECT_EQ(
      "ORed generic specs: after(task_count) >= 4 for scope host OR after(task_count) <= 2 for scope host",
      getSpecBuilder().description());
}

TEST_F(LogicalOrSpecBuilderTest, Goal) {
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(getSpecBuilder().goalCoro(expressionBuilder())),
      "LogicalOrSpec not supported as a goal");
}

CO_TEST_F(LogicalOrSpecBuilderTest, Min) {
  addCapacitySpec(makeCapacitySpec(interface::CapacitySpecBound::MIN, 2));

  const auto constraint = co_await getSingleConstraintCoro();
  entities::Map<std::string, std::string> changes;

  changes = {};
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));

  changes = {{"task0", "host1"}};
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));

  changes = {{"task0", "host2"}};
  EXPECT_TRUE(evaluateTruth(constraint, deltaFromInitial(changes)));

  changes = {{"task5", "host2"}};
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));
}

CO_TEST_F(LogicalOrSpecBuilderTest, Max) {
  addCapacitySpec(makeCapacitySpec(interface::CapacitySpecBound::MAX, 3));

  const auto constraint = co_await getSingleConstraintCoro();
  entities::Map<std::string, std::string> changes;

  changes = {};
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));

  changes = {{"task0", "host1"}};
  EXPECT_TRUE(evaluateTruth(constraint, deltaFromInitial(changes)));

  changes = {{"task5", "host2"}};
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));
}

CO_TEST_F(LogicalOrSpecBuilderTest, MinOrMax) {
  addCapacitySpec(
      makeCapacitySpec(interface::CapacitySpecBound::MIN, 4, "host0"));
  addCapacitySpec(
      makeCapacitySpec(interface::CapacitySpecBound::MAX, 2, "host1"));

  const auto constraint = co_await getSingleConstraintCoro();
  entities::Map<std::string, std::string> changes;

  changes = {};
  EXPECT_TRUE(evaluateTruth(constraint, deltaFromInitial(changes)));

  changes = {{"task0", "host1"}};
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));

  changes = {{"task0", "host2"}};
  EXPECT_TRUE(evaluateTruth(constraint, deltaFromInitial(changes)));

  changes = {{"task6", "host1"}};
  EXPECT_TRUE(evaluateTruth(constraint, deltaFromInitial(changes)));
}
} // namespace facebook::rebalancer::materializer::tests
