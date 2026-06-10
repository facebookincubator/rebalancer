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
#include "algopt/rebalancer/materializer/spec_builder/ScopeAffinitiesSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

using namespace facebook::rebalancer;
using namespace entities;

namespace facebook::rebalancer::materializer::tests {

class ScopeAffinitiesSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host0", {"task0", "task1", "task2"}}, {"host1", {}}, {"host2", {}}});

    const entities::Map<entities::ObjectId, double> cpuValues = {
        {objectId("task0"), 10},
        {objectId("task1"), 20},
        {objectId("task2"), 40}};
    co_await addObjectDimension("cpu", cpuValues, 0);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(ScopeAffinitiesSpecBuilderTest, Goal) {
  interface::ScopeAffinitiesSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.affinities() = {{"host1", 0.01}, {"host2", 0.1}};

  const ScopeAffinitiesSpecBuilder specBuilder(buildUniverse(), spec);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  EXPECT_NEAR(0.0, evaluate(goal, deltaFromInitial({})), 1e-8);
  EXPECT_NEAR(
      -0.1, evaluate(goal, deltaFromInitial({{"task0", "host1"}})), 1e-8);
  EXPECT_NEAR(
      -1.0, evaluate(goal, deltaFromInitial({{"task0", "host2"}})), 1e-8);
  EXPECT_NEAR(
      -4.2,
      evaluate(
          goal, deltaFromInitial({{"task1", "host1"}, {"task2", "host2"}})),
      1e-8);
}

TEST_F(ScopeAffinitiesSpecBuilderTest, Constraint) {
  const interface::ScopeAffinitiesSpec spec;
  ScopeAffinitiesSpecBuilder specBuilder(buildUniverse(), spec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder())),
      "ScopeAffinitiesSpec not supported as a constraint");
}

TEST_F(ScopeAffinitiesSpecBuilderTest, Description) {
  interface::ScopeAffinitiesSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  const ScopeAffinitiesSpecBuilder specBuilder(buildUniverse(), spec);
  EXPECT_EQ("Scope host affinities on cpu", specBuilder.description());
}

TEST_F(ScopeAffinitiesSpecBuilderTest, SpecInfo) {
  interface::ScopeAffinitiesSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.affinities() = {{"host1", 0.01}, {"host2", 0.1}};
  const ScopeAffinitiesSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "", .scope = "host", .dimension = "cpu", .size = 2};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

} // namespace facebook::rebalancer::materializer::tests
