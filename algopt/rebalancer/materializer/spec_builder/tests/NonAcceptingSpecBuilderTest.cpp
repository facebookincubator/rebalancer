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
#include "algopt/rebalancer/materializer/spec_builder/NonAcceptingSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class NonAcceptingSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({{"host0", {"task0"}}, {"host1", {"task1"}}});
    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

TEST_F(NonAcceptingSpecBuilderTest, Description) {
  interface::NonAcceptingSpec spec;
  spec.scope() = "host";
  spec.items() = {"host0", "host1"};

  const NonAcceptingSpecBuilder specBuilder(buildUniverse(), spec);
  EXPECT_EQ("Non-accepting 2 hosts", specBuilder.description());
}

CO_TEST_F(NonAcceptingSpecBuilderTest, Constraint) {
  interface::NonAcceptingSpec spec;
  spec.scope() = "host";
  spec.items() = {"host0"};

  const NonAcceptingSpecBuilder specBuilder(buildUniverse(), spec);
  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  auto root = any_positive(constraints);

  // constraint is initially satisfied
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({})), 1e-8);

  // moving objects away from non-accepting host0 is allowed
  EXPECT_NEAR(0, evaluate(root, deltaFromInitial({{"task0", "host1"}})), 1e-8);

  // moving objects into non-accepting host0 is not allowed
  EXPECT_NEAR(1, evaluate(root, deltaFromInitial({{"task1", "host0"}})), 1e-8);
}

TEST_F(NonAcceptingSpecBuilderTest, Goal) {
  interface::NonAcceptingSpec spec;
  spec.scope() = "host";
  spec.items() = {"host0"};

  NonAcceptingSpecBuilder specBuilder(buildUniverse(), spec);
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "NonAcceptingSpec not supported as a goal");
}

TEST_F(NonAcceptingSpecBuilderTest, SpecInfo) {
  interface::NonAcceptingSpec spec;
  spec.scope() = "host";
  spec.items() = {"host0", "host1"};

  const NonAcceptingSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo =
      SpecParameters{.name = "", .scope = "host", .size = 2};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

CO_TEST_F(NonAcceptingSpecBuilderTest, FixedContainersWithScope) {
  co_await addScope("region", {{"region0", {"host0"}}, {"region1", {"host1"}}});

  interface::NonAcceptingSpec spec;
  spec.scope() = "region";
  spec.items() = {"region0", "region1"};

  const NonAcceptingSpecBuilder specBuilder(buildUniverse(), spec);

  // Both containers are non accepting
  auto nonAcceptingContainers = specBuilder.nonAcceptingContainers();
  EXPECT_EQ(2, nonAcceptingContainers.size());
  EXPECT_TRUE(nonAcceptingContainers.contains(containerId("host0")));
  EXPECT_TRUE(nonAcceptingContainers.contains(containerId("host1")));
}

} // namespace facebook::rebalancer::materializer::tests
