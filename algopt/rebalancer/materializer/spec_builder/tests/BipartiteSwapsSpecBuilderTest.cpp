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
#include "algopt/rebalancer/materializer/spec_builder/BipartiteSwapsSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class BipartiteSwapsSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1"}},
        {"host1", {"task2"}},
        {"host2", {"task3"}},
        {"host3", {"task4"}},
    });
    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(BipartiteSwapsSpecBuilderTest, Constraint) {
  interface::BipartiteSwapsSpec spec;
  spec.name() = "test_spec";
  spec.subsetContainers() = {"host0", "host1"};

  const BipartiteSwapsSpecBuilder specBuilder(buildUniverse(), spec);

  EXPECT_EQ(
      "Enforce swaps between left and right side of bipartite graph test_spec",
      specBuilder.description());

  EXPECT_EQ(SpecParameters{.name = "test_spec"}, specBuilder.getSpecInfo());

  const auto constraint = co_await specBuilder.constraints(expressionBuilder());

  {
    // If task1 moves to host1, (same side move), then constraint stays
    // satisfied.
    std::vector<double> componentValues;
    componentValues.reserve(constraint.size());
    for (const auto& component : constraint) {
      componentValues.push_back(
          evaluate(component, deltaFromInitial({{"task1", "host1"}})));
    }
    EXPECT_NEAR(0, componentValues.at(0), 1e-8);
    EXPECT_NEAR(0, componentValues.at(1), 1e-8);
  }
  {
    // If task1 moves to host3, (opposite side move) then constraint breaks.
    std::vector<double> componentValues;
    componentValues.reserve(constraint.size());
    for (const auto& component : constraint) {
      componentValues.push_back(
          evaluate(component, deltaFromInitial({{"task1", "host3"}})));
    }
    EXPECT_NEAR(-1, componentValues.at(0), 1e-8);
    EXPECT_NEAR(1, componentValues.at(1), 1e-8);
  }
  {
    // If task1 swaps with task4, (opposite side swap) then constraint stays
    // satisfied.
    std::vector<double> componentValues;
    componentValues.reserve(constraint.size());
    for (const auto& component : constraint) {
      componentValues.push_back(evaluate(
          component,
          deltaFromInitial({{"task1", "host3"}, {"task4", "host0"}})));
    }
    EXPECT_NEAR(0, componentValues.at(0), 1e-8);
    EXPECT_NEAR(0, componentValues.at(1), 1e-8);
  }
}

TEST_F(BipartiteSwapsSpecBuilderTest, Goal) {
  const interface::BipartiteSwapsSpec spec;
  const BipartiteSwapsSpecBuilder specBuilder(buildUniverse(), spec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "not supported as a goal");
}
} // namespace facebook::rebalancer::materializer::tests
