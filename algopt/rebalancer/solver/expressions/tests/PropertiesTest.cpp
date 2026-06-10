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

#include "algopt/rebalancer/solver/expressions/AnyPositive.h"
#include "algopt/rebalancer/solver/expressions/BipartiteSwaps.h"
#include "algopt/rebalancer/solver/expressions/Ceil.h"
#include "algopt/rebalancer/solver/expressions/Log.h"
#include "algopt/rebalancer/solver/expressions/NthLargest.h"
#include "algopt/rebalancer/solver/expressions/ObjectPartitionMoveLimit.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/ProductOperation.h"
#include "algopt/rebalancer/solver/expressions/QuotientOperation.h"
#include "algopt/rebalancer/solver/expressions/Square.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"

#include <fmt/format.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace facebook::rebalancer::packer::tests {

class PropertiesTest : public ExpressionTestsBase {
 protected:
  void SetUp() override {
    // Set up default universe with 20 containers/objects
    constexpr int kNumContainers = 20;
    entities::Map<std::string, std::vector<std::string>> initialAssignment;
    for (const auto i : folly::irange(kNumContainers)) {
      initialAssignment[fmt::format("container{}", i)] = {
          fmt::format("object{}", i)};
    }
    setInitialAssignment(initialAssignment);
  }
};

// Fixture for tests that need custom initial assignments
class PropertiesTestCustom : public ExpressionTestsBase {};

TEST_F(PropertiesTest, AnyPositive) {
  const auto universe = buildUniverse();
  const AnyPositive anyPositive({}, universe, 1e-3);
  ASSERT_EQ("AnyPositive", anyPositive.getType());

  auto properties = anyPositive.getProperties();
  ASSERT_EQ(1, properties.properties()->size());
  ASSERT_EQ(
      1e-3,
      *properties.properties()
           ->at("feasibility_tolerance")
           .valueDouble()
           ->value());
}

TEST_F(PropertiesTest, Product) {
  const auto universe = buildUniverse();
  const ProductOperation product(
      const_expr(1, universe), const_expr(1, universe), universe);
  ASSERT_EQ("Product", product.getType());

  auto properties = product.getProperties();
  ASSERT_EQ(1, properties.properties()->size());
  ASSERT_EQ(
      "PRODUCT", *properties.properties()->at("type").valueString()->value());
}

TEST_F(PropertiesTest, Quotient) {
  const auto universe = buildUniverse();
  const QuotientOperation quotient(
      const_expr(1, universe), const_expr(1, universe), universe);
  ASSERT_EQ("Quotient", quotient.getType());

  auto properties = quotient.getProperties();
  ASSERT_EQ(1, properties.properties()->size());
  ASSERT_EQ(
      "QUOTIENT", *properties.properties()->at("type").valueString()->value());
}

TEST_F(PropertiesTest, BipartiteSwaps) {
  const auto universe = buildUniverse();
  const BipartiteSwaps swaps(
      {}, {container(1), container(2)}, {container(3)}, universe);
  ASSERT_EQ("BipartiteSwaps", swaps.getType());

  auto properties = swaps.getProperties();
  ASSERT_EQ(2, properties.properties()->size());
  // Container list order may vary by platform due to hash map iteration
  auto leftSubset = *properties.properties()
                         ->at("left_subset")
                         .valueContainerIdList()
                         ->value();
  std::sort(leftSubset.begin(), leftSubset.end());
  auto expectedLeft =
      std::vector<int>({container(1).asInt(), container(2).asInt()});
  std::sort(expectedLeft.begin(), expectedLeft.end());
  ASSERT_EQ(expectedLeft, leftSubset);
  ASSERT_EQ(
      std::vector<int>({container(3).asInt()}),
      *properties.properties()
           ->at("right_subset")
           .valueContainerIdList()
           ->value());
}

TEST_F(PropertiesTest, LinearSum) {
  const auto universe = buildUniverse();
  auto sum = const_expr(42, universe);
  ASSERT_EQ("LinearSum", sum->getType());

  auto properties = sum->getProperties();
  ASSERT_EQ(1, properties.properties()->size());
  ASSERT_EQ(
      42, *properties.properties()->at("constant").valueDouble()->value());
}

TEST_F(PropertiesTest, NthLargest) {
  const auto universe = buildUniverse();
  const NthLargest nth({const_expr(1, universe)}, 4, false, universe);
  ASSERT_EQ("NthLargest", nth.getType());

  auto properties = nth.getProperties();
  ASSERT_EQ(2, properties.properties()->size());
  ASSERT_EQ(4, *properties.properties()->at("n").valueInt()->value());
  ASSERT_EQ(false, *properties.properties()->at("unique").valueBool()->value());
}

TEST_F(PropertiesTest, ObjectLookup) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto lookup = object_lookup(
      makeObjectVector(PackerMap<entities::ObjectId, double>(), universe),
      std::make_shared<PackerSet<entities::ContainerId>>(
          PackerSet<entities::ContainerId>{container(3), container(4)}),
      universe,
      assignment);
  ASSERT_EQ("ObjectLookup", lookup->getType());

  auto properties = lookup->getProperties();
  ASSERT_EQ(1, properties.properties()->size());
  // Container list order may vary by platform due to hash map iteration
  auto containers = *properties.properties()
                         ->at("containers")
                         .valueContainerIdList()
                         ->value();
  std::sort(containers.begin(), containers.end());
  auto expectedContainers =
      std::vector<int>({container(3).asInt(), container(4).asInt()});
  std::sort(expectedContainers.begin(), expectedContainers.end());
  ASSERT_EQ(expectedContainers, containers);
}

TEST_F(PropertiesTest, ObjectVector) {
  const auto universe = buildUniverse();
  auto vector = makeObjectVector({{object(3), 1.3}}, universe);
  ASSERT_EQ("ObjectVector", vector->getType());

  auto properties = vector->getProperties();
  ASSERT_EQ(2, properties.properties()->size());
  ASSERT_EQ(
      0, properties.properties()->at("default_value").valueDouble()->value());
  const std::map<int, double> expected({{object(3).asInt(), 1.3}});
  ASSERT_EQ(
      expected,
      *properties.properties()
           ->at("object_values")
           .valueObjectIdDoubleMap()
           ->value());
}

TEST_F(PropertiesTest, Piecewise) {
  const auto universe = buildUniverse();
  auto pw = piecewise(
      {{0.0, 10.0}, {10.0, 5.0}}, const_expr(2.5, universe), universe);
  ASSERT_EQ("Piecewise", pw->getType());

  auto properties = pw->getProperties();
  ASSERT_EQ(1, properties.properties()->size());
  auto& points =
      *properties.properties()->at("points").valuePoint2dList()->value();
  ASSERT_EQ(2, points.size());
  ASSERT_EQ(0.0, *points.at(0).x());
  ASSERT_EQ(10.0, *points.at(0).y());
  ASSERT_EQ(10.0, *points.at(1).x());
  ASSERT_EQ(5.0, *points.at(1).y());
}

TEST_F(PropertiesTest, TransformPower) {
  const auto universe = buildUniverse();
  auto transform = power(const_expr(1.0, universe), 2.0, universe);
  ASSERT_EQ("Power", transform->getType());

  auto properties = transform->getProperties();
  ASSERT_EQ(1, properties.properties()->size());
  ASSERT_EQ(
      2.0, *properties.properties()->at("exponent").valueDouble()->value());
}

TEST_F(PropertiesTest, TransformRectangle) {
  const auto universe = buildUniverse();
  auto transform = rectangle(const_expr(1.0, universe), 2.0, 3.0, universe);
  ASSERT_EQ("Rectangle", transform->getType());

  auto properties = transform->getProperties();
  ASSERT_EQ(2, properties.properties()->size());
  ASSERT_EQ(
      2.0, *properties.properties()->at("lower_bound").valueDouble()->value());
  ASSERT_EQ(
      3.0, *properties.properties()->at("upper_bound").valueDouble()->value());
}

TEST_F(PropertiesTest, Variable) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto var = variable(object(3), container(4), universe, assignment);
  ASSERT_EQ("Variable", var->getType());

  auto properties = var->getProperties();
  ASSERT_EQ(2, properties.properties()->size());
  ASSERT_EQ(
      object(3).asInt(),
      *properties.properties()->at("object").valueObjectId()->value());
  ASSERT_EQ(
      container(4).asInt(),
      *properties.properties()->at("container").valueContainerId()->value());
}

TEST_F(PropertiesTest, Ceil) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto ceil =
      Ceil(variable(object(2), container(3), universe, assignment), universe);
  ASSERT_EQ("Ceil", ceil.getType());
}

TEST_F(PropertiesTest, Log) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto log = Log(
      200 * variable(object(2), container(3), universe, assignment), universe);
  ASSERT_EQ(log.getType(), "Log");
}

CO_TEST_F(PropertiesTestCustom, ObjectPartition) {
  setInitialAssignment(
      folly::F14FastMap<std::string, std::vector<std::string>>{
          {"container1", {"object1"}}});

  const auto objectCountDimensionId = dimensionId("object_count");
  co_await addPartition("partition1", {});

  auto objPartition = object_partition(
      partitionId("partition1"), objectCountDimensionId, {}, buildUniverse());

  EXPECT_EQ("ObjectPartition", objPartition->getType());
}

CO_TEST_F(PropertiesTestCustom, ObjectPartitionLookup) {
  setInitialAssignment(
      folly::F14FastMap<std::string, std::vector<std::string>>{
          {"container1", {"object1"}}});

  const auto objectCountDimensionId = dimensionId("object_count");
  co_await addPartition("partition1", {});

  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  const auto containerScopeId = scopeId("container");
  const auto container1ScopeItemId =
      scopeItemId(containerScopeId, "container1");

  auto objPartitionLookup = object_partition_lookup(
      object_partition(
          partitionId("partition1"), objectCountDimensionId, {}, universe),
      std::make_shared<PackerSet<entities::ContainerId>>(),
      containerScopeId,
      container1ScopeItemId,
      universe,
      assignment);
  EXPECT_EQ("ObjectPartitionLookup", objPartitionLookup->getType());
}

CO_TEST_F(PropertiesTestCustom, ObjectPartitionMoveLimit) {
  setInitialAssignment(
      folly::F14FastMap<std::string, std::vector<std::string>>{
          {"container1", {"object1"}}});

  const auto objectCountDimensionId = dimensionId("object_count");
  co_await addPartition("partition1", {});

  const auto universe = buildUniverse();

  auto moveLimit = ObjectPartitionMoveLimit(
      universe,
      {},
      partitionId("partition1"),
      objectCountDimensionId,
      {},
      {},
      {});
  EXPECT_EQ("ObjectPartitionMoveLimit", moveLimit.getType());
}

TEST_F(PropertiesTest, Square) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto square =
      Square(variable(object(3), container(4), universe, assignment), universe);
  ASSERT_EQ("Square", square.getType());
}

TEST_F(PropertiesTest, StableStayed) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto vec =
      makeObjectVector(PackerMap<entities::ObjectId, double>{}, universe);
  auto stayed = stable_stayed(
      vec,
      vec,
      std::make_shared<PackerSet<entities::ContainerId>>(
          PackerSet<entities::ContainerId>{container(0), container(1)}),
      universe,
      assignment);
  ASSERT_EQ("StableStayed", stayed->getType());
}

TEST_F(PropertiesTest, Step) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto stepExpr =
      step(variable(object(2), container(3), universe, assignment), universe);
  ASSERT_EQ("Step", stepExpr->getType());
}

TEST_F(PropertiesTest, SumOverThreshold) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto overThreshold = sum_over_threshold(
      const_expr(0.6, universe),
      {variable(object(0), container(1), universe, assignment) + 0.2},
      false,
      universe);
  ASSERT_EQ("SumOverThreshold", overThreshold->getType());
}

} // namespace facebook::rebalancer::packer::tests
