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
#include "algopt/rebalancer/materializer/spec_builder/LogicalAndSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {
namespace entities = facebook::rebalancer::entities;
namespace interface = facebook::rebalancer::interface;

class LogicalAndSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  // Our universe is created with "taskX" objects and "hostY" containers.
  // It also has a single scope "host".
  folly::coro::Task<void> setUpCoro() {
    // This is the initial assignment of tasks to hosts.
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

  // Our convention for boolean / truth values is that the value of an
  // expression should be <= 0 to be considered true
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
    logicalAndSpec.genericSpecs()->push_back(genericSpec);
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

  LogicalAndSpecBuilder getSpecBuilder() {
    LogicalAndSpecBuilder specBuilder(buildUniverse(), logicalAndSpec);
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
  interface::LogicalAndSpec logicalAndSpec;
};

TEST_F(LogicalAndSpecBuilderTest, Description) {
  addCapacitySpec(
      makeCapacitySpec(interface::CapacitySpecBound::MIN, 4, "host0"));
  addCapacitySpec(
      makeCapacitySpec(interface::CapacitySpecBound::MAX, 2, "host1"));

  EXPECT_EQ(
      "ANDed generic specs: after(task_count) >= 4 for scope host AND after(task_count) <= 2 for scope host",
      getSpecBuilder().description());
}

TEST_F(LogicalAndSpecBuilderTest, Goal) {
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(getSpecBuilder().goalCoro(expressionBuilder())),
      "LogicalAndSpec not supported as a goal");
}

CO_TEST_F(LogicalAndSpecBuilderTest, Min) {
  // enforce that this is true:
  //   all hosts have >= 2 tasks
  addCapacitySpec(makeCapacitySpec(interface::CapacitySpecBound::MIN, 2));

  const auto constraint = co_await getSingleConstraintCoro();
  entities::Map<std::string, std::string> changes;

  // 1. Initially condition is NOT satisfied:
  // host0 has 4 tasks and is OK
  // host1 has 2 tasks and is OK
  // host2 has 1 tasks and is NOT OK
  // this fails (host0 >= 2 AND host1 >= 2 AND host2 >= 2)
  changes = {};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));

  // 2. After moving task0 to host1, condition is NOT satisfied:
  // host0 has 3 tasks and is OK
  // host1 has 3 tasks and is OK
  // host2 has 1 tasks and is NOT OK
  // this fails (host0 >= 2 AND host1 >= 2 AND host2 >= 2)
  changes = {{"task0", "host1"}};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));

  // 3. After moving task0 to host2, condition is satisfied:
  // host0 has 3 tasks and is OK
  // host1 has 2 tasks and is OK
  // host2 has 2 tasks and is OK
  // this satisfies (host0 >= 2 AND host1 >= 2 AND host2 >= 2)
  changes = {{"task0", "host2"}};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_TRUE(evaluateTruth(constraint, deltaFromInitial(changes)));

  // 4. After moving task5 (from host1) to host2, condition is NOT satisfied:
  // host0 has 4 tasks and is OK
  // host1 has 1 tasks and is NOT OK
  // host2 has 2 tasks and is OK
  // this fails (host0 >= 2 AND host1 >= 2 AND host2 >= 2)
  changes = {{"task5", "host2"}};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));
}

CO_TEST_F(LogicalAndSpecBuilderTest, Max) {
  // enforce that this is true:
  //   all hosts have <= 3 tasks
  addCapacitySpec(makeCapacitySpec(interface::CapacitySpecBound::MAX, 3));

  const auto constraint = co_await getSingleConstraintCoro();
  entities::Map<std::string, std::string> changes;

  // 1. Initially condition is NOT satisfied:
  // host0 has 4 tasks and is NOT OK
  // host1 has 2 tasks and is OK
  // host2 has 1 tasks and is OK
  // this fails (host0 <= 3 AND host1 <= 3 AND host2 <= 3)
  changes = {};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));

  // 2. After moving task0 to host1, condition is satisfied:
  // host0 has 3 tasks and is OK
  // host1 has 3 tasks and is OK
  // host2 has 1 tasks and is OK
  // this satisfies (host0 <= 3 AND host1 <= 3 AND host2 <= 3)
  changes = {{"task0", "host1"}};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_TRUE(evaluateTruth(constraint, deltaFromInitial(changes)));

  // 3. After moving task1 to host 1 and task6 (from host2) to host1,
  // condition is NOT satisfied:
  // host0 has 3 tasks and is OK
  // host1 has 4 tasks and is NOT OK
  // host2 has 0 tasks and is OK
  // this fails (host0 <= 3 AND host1 <= 3 AND host2 <= 3)
  changes = {{"task5", "host2"}};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));
}

CO_TEST_F(LogicalAndSpecBuilderTest, MinAndMax) {
  // enforce that ALL of these are true:
  //   host0 has >= 4 tasks
  //   host1 has <= 2 tasks
  addCapacitySpec(
      makeCapacitySpec(interface::CapacitySpecBound::MIN, 4, "host0"));
  addCapacitySpec(
      makeCapacitySpec(interface::CapacitySpecBound::MAX, 2, "host1"));

  const auto constraint = co_await getSingleConstraintCoro();
  entities::Map<std::string, std::string> changes;

  // 1. Initially condition is satisfied:
  // host0 has 4 tasks and is OK
  // host1 has 2 tasks and is OK
  // host2 has 1 tasks
  // this satisfies (host0 >= 4 AND host1 <= 2)
  changes = {};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_TRUE(evaluateTruth(constraint, deltaFromInitial(changes)));

  // 2. After moving task0 to host1, condition is NOT satisfied:
  // host0 has 3 tasks and is NOT OK
  // host1 has 3 tasks and is NOT OK
  // host2 has 1 tasks
  // this fails (host0 >= 4 AND host1 <= 2)
  changes = {{"task0", "host1"}};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));

  // 3. After moving task0 to host2, condition is NOT satisfied:
  // host0 has 3 tasks and is NOT OK
  // host1 has 2 tasks and is OK
  // host2 has 2 tasks
  // this fails (host0 >= 4 AND host1 <= 2)
  changes = {{"task0", "host2"}};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));

  // 4. After moving task6 (from host2) to host1, condition is NOT satisfied:
  // host0 has 4 tasks and is OK
  // host1 has 3 tasks and is NOT OK
  // host2 has 0 tasks
  // this fails (host0 >= 4 AND host1 <= 2)
  changes = {{"task6", "host1"}};
  //  XLOG(INFO) << digest(constraint, changes);
  EXPECT_FALSE(evaluateTruth(constraint, deltaFromInitial(changes)));
}
} // namespace facebook::rebalancer::materializer::tests
