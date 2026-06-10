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
#include "algopt/rebalancer/materializer/spec_builder/DiversifyWithinScopeItemSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace entities = facebook::rebalancer::entities;
namespace interface = facebook::rebalancer::interface;

namespace facebook::rebalancer::materializer::tests {

class DiversifyWithinScopeItemSpecTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {
            {"host1", {"trafficObject8"}},
            {"host2", {"trafficObject1", "trafficObject3", "trafficObject4"}},
            {"host3", {"trafficObject2", "trafficObject11"}},
            //
            {"host4", {"trafficObject9", "trafficObject10", "trafficObject12"}},
            {"host5", {"trafficObject5", "trafficObject7"}},
            {"host6", {"trafficObject6"}},
        },
        "trafficObject",
        "host");

    co_await addScope(
        "region",
        {{"region1", {"host1", "host2", "host3"}},
         {"region2", {"host4", "host5", "host6"}}});

    const entities::Map<entities::ObjectId, double> baseObjectToValue = {
        {objectId("trafficObject1"), 1.0},
        {objectId("trafficObject2"), 0.5},
        {objectId("trafficObject3"), 2.0},
        {objectId("trafficObject4"), 0.5},
        {objectId("trafficObject5"), 0.5},
        {objectId("trafficObject6"), 0.5},
        //
        {objectId("trafficObject7"), 1.0},
        {objectId("trafficObject8"), 0.5},
        {objectId("trafficObject9"), 2.0},
        {objectId("trafficObject10"), 3.0},
        {objectId("trafficObject11"), 2.1},
        {objectId("trafficObject12"), 1.0},
    };

    // only difference between object dimension values for region1 and region2
    // is the value of trafficObject7
    auto region2DimensionValues = baseObjectToValue;
    region2DimensionValues[objectId("trafficObject7")] = 0.5;

    co_await addDynamicObjectDimension(
        "replicaCount",
        scopeId("region"),
        {{"region1", makeSharedPtrEntityToValueMap(baseObjectToValue)},
         {"region2", makeSharedPtrEntityToValueMap(region2DimensionValues)}},
        0.0);

    co_await addPartition(
        "tenantTrafficObjects",
        {{"tenant1-trafficObjects",
          {"trafficObject1",
           "trafficObject2",
           "trafficObject3",
           "trafficObject4",
           "trafficObject5",
           "trafficObject6"}},
         {"tenant2-trafficObjects",
          {"trafficObject7",
           "trafficObject8",
           "trafficObject9",
           "trafficObject10",
           "trafficObject11",
           "trafficObject12"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

CO_TEST_F(DiversifyWithinScopeItemSpecTest, GoalBasic) {
  interface::DiversifyWithinScopeItemSpec spec;
  spec.dimension() = "replicaCount";
  spec.partition() = "tenantTrafficObjects";
  spec.scope() = "region";
  spec.groupToLimit()->globalLimit() = 3.8;

  const DiversifyWithinScopeItemSpec specBuilder(buildUniverse(), spec);
  const auto goalExpr = co_await specBuilder.goalCoro(expressionBuilder());

  // initial utils:
  // group1, region1 util = 1.0 + 0.5 + 2.0 + 0.5 = 4.0
  // --- spreading formula: 0^1.1 + (1+2+0.5)^1.1 + 0.5^1.1 = 4.43363

  // group2, region2 util = 0.5 + 2.0 + 3.0 + 1.0 = 6.5
  // --- spreading formula: 0.5^1.1 + (2+3+1)^1.1 = 7.6439
  auto initialUtil =
      (pow(0, 1.1) + pow(1 + 2 + 0.5, 1.1) + pow(0.5, 1.1)) + // group1, region1
      (pow(0.5, 1.1) + pow(2 + 3 + 1, 1.1)); // group2, region2;
  EXPECT_NEAR(initialUtil, evaluate(goalExpr, deltaFromInitial({})), 1e-8);

  // if we move trafficObject4 to host3, group1 is happier as it wants to spread
  auto move1Util =
      (pow(0, 1.1) + pow(1 + 2, 1.1) + pow(0.5 + 0.5, 1.1)) + // group1, region1
      (pow(0.5, 1.1) + pow(2 + 3 + 1, 1.1)); // group2, region2;
  EXPECT_NEAR(
      move1Util,
      evaluate(goalExpr, deltaFromInitial({{"trafficObject4", "host3"}})),
      1e-8);
  EXPECT_TRUE(move1Util < initialUtil);

  // if we move trafficObject3 to host3, group1 is even happier than the case
  // above
  auto move2Util = (pow(0, 1.1) + pow(1 + 0.5, 1.1) +
                    pow(0.5 + 2.0, 1.1)) + // group1, region1
      (pow(0.5, 1.1) + pow(2 + 3 + 1, 1.1)); // group2, region2;
  EXPECT_NEAR(
      move2Util,
      evaluate(goalExpr, deltaFromInitial({{"trafficObject3", "host3"}})),
      1e-8);
  EXPECT_TRUE(move2Util < move1Util);

  // if we move trafficObject11 to host1 and then move trafficObject10 to host3,
  // group2 becomes above limit in region1, and should start spreading.
  // In region2, it no longer has any intent to spread
  auto move4Util =
      (pow(0, 1.1) + pow(1 + 2 + 0.5, 1.1) + pow(0.5, 1.1)) + // group1, region1
      (pow(0.5, 1.1) + pow(3.0 + 2.1, 1.1)); // group2, region1
  EXPECT_NEAR(
      move4Util,
      evaluate(goalExpr, deltaFromInitial({{"trafficObject10", "host3"}})),
      1e-8);
  EXPECT_TRUE(move4Util < move1Util);

  // if we move trafficObject1 to host6, group1 becomes below upper limit so it
  // has not intent to spread
  auto move5Util = (pow(0.5, 1.1) + pow(2 + 3 + 1, 1.1)); // group2, region2;
  EXPECT_NEAR(
      move5Util,
      evaluate(goalExpr, deltaFromInitial({{"trafficObject1", "host6"}})),
      1e-8);
  EXPECT_TRUE(move5Util < initialUtil);
}

CO_TEST_F(DiversifyWithinScopeItemSpecTest, GoalWithRelativeLimits) {
  co_await addScopeDimension(
      "replicaCount",
      scopeId("region"),
      {{"region1", 5.0}, {"region2", 5.0}},
      0.0);

  interface::DiversifyWithinScopeItemSpec spec;
  spec.dimension() = "replicaCount";
  spec.partition() = "tenantTrafficObjects";
  spec.scope() = "region";

  spec.groupToLimit()->type() = interface::LimitType::RELATIVE;
  spec.groupToLimit()->globalLimit() = 0.76;

  const DiversifyWithinScopeItemSpec specBuilder(buildUniverse(), spec);
  const auto goalExpr = co_await specBuilder.goalCoro(expressionBuilder());

  // this is the exact same example as in BasicGoal test, but with relative
  // limits; just check the initial value is computed correctly
  auto initialUtil =
      (pow(0, 1.1) + pow(1 + 2 + 0.5, 1.1) + pow(0.5, 1.1)) + // group1, region1
      (pow(0.5, 1.1) + pow(2 + 3 + 1, 1.1)); // group2, region2;
  EXPECT_NEAR(initialUtil, evaluate(goalExpr, deltaFromInitial({})), 1e-8);
}

TEST_F(DiversifyWithinScopeItemSpecTest, ScopeDimensionValueZeroError) {
  folly::coro::blockingWait(addScopeDimension(
      "replicaCount", scopeId("region"), {{"region1", 0.1}}, 0.0));

  interface::DiversifyWithinScopeItemSpec spec;
  spec.dimension() = "replicaCount";
  spec.partition() = "tenantTrafficObjects";
  spec.scope() = "region";

  DiversifyWithinScopeItemSpec specBuilder(buildUniverse(), spec);
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.goalCoro(expressionBuilder())),
      "Expected scope items to have non-zero dimension value when using DiversifyWithinScopeItemSpec,"
      "but found zero for scope item 'region2' w.r.t. dimemsion 'replicaCount'");
}

CO_TEST_F(DiversifyWithinScopeItemSpecTest, EnsureFilterWorks) {
  interface::DiversifyWithinScopeItemSpec spec;
  spec.dimension() = "replicaCount";
  spec.partition() = "tenantTrafficObjects";
  spec.scope() = "region";
  // both regions are filtered out, so expect const_expr(0, universe_) as
  // the goal
  spec.scopeItemFilter()->itemsBlacklist() = {"region1", "region2"};

  const DiversifyWithinScopeItemSpec specBuilder(buildUniverse(), spec);
  const auto goalExpr = co_await specBuilder.goalCoro(expressionBuilder());

  EXPECT_NEAR(0.0, evaluate(goalExpr, deltaFromInitial({})), 1e-8);
}

TEST_F(DiversifyWithinScopeItemSpecTest, ConstraintThrows) {
  interface::DiversifyWithinScopeItemSpec spec;
  spec.dimension() = "replicaCount";
  spec.partition() = "tenantTrafficObjects";
  spec.scope() = "region";

  DiversifyWithinScopeItemSpec specBuilder(buildUniverse(), spec);
  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder())),
      "DiversifyWithinScopeItemSpec is NOT supported as a constraint");
}

TEST_F(DiversifyWithinScopeItemSpecTest, Description) {
  interface::DiversifyWithinScopeItemSpec spec;
  spec.dimension() = "replicaCount";
  spec.partition() = "tenantTrafficObjects";
  spec.scope() = "region";

  const DiversifyWithinScopeItemSpec specBuilder(buildUniverse(), spec);
  EXPECT_EQ(
      "Diversify group within scope item using partition 'tenantTrafficObjects', scope 'region', and dimension 'replicaCount'",
      specBuilder.description());
}

TEST_F(DiversifyWithinScopeItemSpecTest, SpecInfo) {
  interface::DiversifyWithinScopeItemSpec spec;
  spec.name() = "test";
  spec.dimension() = "replicaCount";
  spec.scope() = "region";
  spec.partition() = "tenantTrafficObjects";

  const DiversifyWithinScopeItemSpec specBuilder(buildUniverse(), spec);

  const auto expectedSpecInfo = facebook::rebalancer::SpecParameters{
      .name = "test",
      .scope = "region",
      .partition = "tenantTrafficObjects",
      .dimension = "replicaCount"};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
