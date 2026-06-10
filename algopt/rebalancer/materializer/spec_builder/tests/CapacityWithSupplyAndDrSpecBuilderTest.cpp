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
#include "algopt/rebalancer/materializer/spec_builder/CapacityWithSupplyAndDrSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class CapacityWithSupplyAndDrSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"r0", {"p0"}},
        {"r1", {"p1", "s4"}},
        {"r2", {"p2", "s3"}},
        {"r3", {"p3"}},
        {"r4", {"p4", "s0", "s1", "s2"}},
    });

    co_await addScope(
        "zone",
        {
            {"z0", {"r0", "r1"}},
            {"z1", {"r2"}},
            {"z2", {"r3"}},
            {"z3", {"r4"}},
        });

    co_await addObjectDimension(
        "cpu",
        {
            {objectId("p0"), 1},
            {objectId("p1"), 2},
            {objectId("p2"), 4},
            {objectId("p3"), 8},
            {objectId("p4"), 16},
            {objectId("s0"), 1},
            {objectId("s1"), 2},
            {objectId("s2"), 4},
            {objectId("s3"), 8},
            {objectId("s4"), 16},
        },
        0);

    co_await addPartition(
        "primaries",
        {{"primaries",
          {
              "p0",
              "p1",
              "p2",
              "p3",
              "p4",
          }}});

    co_await addPartition("empty", {});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

TEST_F(CapacityWithSupplyAndDrSpecBuilderTest, Description) {
  interface::CapacityWithSupplyAndDrSpec spec;
  spec.dimension() = "cpu";
  spec.prodScope() = "region";
  spec.prodItem() = "vll";
  const CapacityWithSupplyAndDrSpecBuilder specBuilder(buildUniverse(), spec);
  EXPECT_EQ("namespace usage(cpu) on region vll", specBuilder.description());
}

TEST_F(CapacityWithSupplyAndDrSpecBuilderTest, SpecInfo) {
  interface::CapacityWithSupplyAndDrSpec spec;
  spec.name() = "test";
  spec.dimension() = "cpu";
  spec.scope() = "region";
  spec.partitionName() = "primaries";

  const CapacityWithSupplyAndDrSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "test",
      .scope = "region",
      .partition = "primaries",
      .dimension = "cpu",
      .limitType = "ABSOLUTE",
      .filterAllowListSize = 0,
      .filterBlockListSize = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

CO_TEST_F(CapacityWithSupplyAndDrSpecBuilderTest, Goal) {
  interface::CapacityWithSupplyAndDrSpec spec;
  spec.scope() = "zone";
  spec.dimension() = "cpu";
  spec.drPairs() = {
      {"s0", "p0"},
      {"s1", "p1"},
      {"s2", "p2"},
      {"s3", "p3"},
      {"s4", "p4"},
  };
  spec.prodScope() = "host";
  spec.prodItem() = "r4";
  spec.partitionName() = "primaries";
  spec.supplyPartition() = "empty";
  spec.ratio() = 1;
  spec.limit()->globalLimit() = 1;
  spec.limit()->groupLimits()->emplace("primaries", 20);

  const CapacityWithSupplyAndDrSpecBuilder specBuilder(buildUniverse(), spec);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // Host r4 receives the highest cpu load of 20 when zone z1 is down: s2 takes
  // 4 cpu units and p4 takes 16.
  EXPECT_NEAR(0, evaluate(goal, deltaFromInitial({})), 1e-8);

  // If we move p0 from zone z0 to zone z1, then the highest cpu load host r4
  // has to handle becomes 21, which exceeds the limit of 20 by 1.
  EXPECT_NEAR(1.0, evaluate(goal, deltaFromInitial({{"p0", "r2"}})), 1e-8);

  // If we move p2 from zone z1 to zone z0, then zone z0 becomes the bottleneck
  // and sheds 7 units of cpu load onto host r4 on top of the regular 16 cpu
  // units taken by p4, so the limit of 20 is exceeded by 3.
  EXPECT_NEAR(3.0, evaluate(goal, deltaFromInitial({{"p2", "r1"}})), 1e-8);

  // If we exchange the contents of zones z0 and z1, there is no impact to the
  // max cpu load host r4 has to handle and the limit of 20 cpu units is still
  // sufficient.
  EXPECT_NEAR(
      0,
      evaluate(
          goal,
          deltaFromInitial({
              {"p0", "r2"},
              {"p1", "r2"},
              {"p2", "r0"},
          })),
      1e-8);
}
} // namespace facebook::rebalancer::materializer::tests
