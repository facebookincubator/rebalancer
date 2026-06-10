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
#include "algopt/rebalancer/materializer/spec_builder/MultipleOrCapacitySpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class MultipleOrCapacitySpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host0", {"task0", "task1", "task2", "task3"}},
         {"host1", {"task4", "task5", "task6", "task7"}}});

    co_await addObjectDimension(
        "cpu", entities::Map<std::string, double>{}, 10);

    co_await addScopeDimension("cpu", scopeId("host"), {}, 100);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

TEST_F(MultipleOrCapacitySpecBuilderTest, Basic) {
  interface::MultipleOrCapacitySpec multipleSpec;

  {
    interface::CapacitySpec spec;
    spec.scope() = "host";
    spec.dimension() = "cpu";
    spec.bound() = interface::CapacitySpecBound::MAX;
    spec.limit()->globalLimit() = 0.1;
    spec.filter()->itemsWhitelist() = {"host0"};
    multipleSpec.capacitySpecs()->push_back(spec);
  }

  {
    interface::CapacitySpec spec;
    spec.scope() = "host";
    spec.dimension() = "cpu";
    spec.bound() = interface::CapacitySpecBound::MIN;
    spec.limit()->globalLimit() = 0.3;
    spec.filter()->itemsWhitelist() = {"host0"};
    multipleSpec.capacitySpecs()->push_back(spec);
  }

  const MultipleOrCapacitySpecBuilder specBuilder(
      buildUniverse(), multipleSpec);

  auto constraints =
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder()));
  EXPECT_EQ(1, constraints.size());

  const auto& constraint = constraints.at(0);

  EXPECT_NEAR(0.0, evaluate(constraint, deltaFromInitial({})), 1e-8);
  EXPECT_NEAR(
      0.0, evaluate(constraint, deltaFromInitial({{"task0", "host1"}})), 1e-8);
  EXPECT_NEAR(
      0.1,
      evaluate(
          constraint,
          deltaFromInitial({{"task0", "host1"}, {"task1", "host1"}})),
      1e-8);
  EXPECT_NEAR(
      0.0,
      evaluate(
          constraint,
          deltaFromInitial(
              {{"task0", "host1"}, {"task1", "host1"}, {"task2", "host1"}})),
      1e-8);

  auto description = specBuilder.description();
  EXPECT_EQ(
      "Multiple capacity specs: after(cpu) <= 0.1 for scope host OR after(cpu) >= 0.3 for scope host",
      description);

  auto expectedSpecInfo = SpecParameters{.name = "", .size = 2};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

TEST_F(MultipleOrCapacitySpecBuilderTest, Goal) {
  const interface::MultipleOrCapacitySpec spec;
  MultipleOrCapacitySpecBuilder specBuilder(buildUniverse(), spec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "MultipleOrCapacitySpec not supported as a goal");
}

} // namespace facebook::rebalancer::materializer::tests
