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

#include "algopt/rebalancer/solver/expressions/BoundsOverride.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class BoundsOverrideTest : public ExpressionTestsBase {
 protected:
  static ExprPtr createBoundsOverride(
      ExprPtr expr,
      std::shared_ptr<const entities::Universe> universe) {
    return std::make_shared<BoundsOverride>(
        expr, std::optional(1.0), std::optional(2.0), universe);
  }
};

TEST_F(BoundsOverrideTest, test) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{{"container1", {}}});
  const auto universe = buildUniverse();
  auto boundsOverride = createBoundsOverride(const_expr(0, universe), universe);
  Context context;
  auto [lb, ub] = boundsOverride->lowerAndUpperBounds(context);
  EXPECT_EQ(lb, std::optional<double>(1.0));
  EXPECT_EQ(ub, std::optional<double>(2.0));
}

TEST_F(BoundsOverrideTest, Apply) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{{"container1", {}}});
  const auto universe = buildUniverse();
  auto a = const_expr(1, universe);
  auto b = const_expr(-10, universe);
  auto sum = a + b;
  Context context;
  auto boundsOverride = createBoundsOverride(sum, universe);
  auto [lb, ub] = boundsOverride->lowerAndUpperBounds(context);
  EXPECT_EQ(lb, std::optional<double>(1.0));
  EXPECT_EQ(ub, std::optional<double>(2.0));

  auto assignment = makeAssignment({});
  EXPECT_EQ(-9, _apply(*boundsOverride, assignment));
}

TEST_F(BoundsOverrideTest, Type) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{{"container1", {}}});
  const auto universe = buildUniverse();
  auto expr = createBoundsOverride(const_expr(0, universe), universe);
  EXPECT_EQ("BoundsOverride", expr->getType());
}

TEST_F(BoundsOverrideTest, isMax) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{{"container1", {}}});
  const auto universe = buildUniverse();
  auto boundsOverride =
      createBoundsOverride(const_expr(-1, universe), universe);
  EXPECT_FALSE(boundsOverride->isMax());
}

TEST_F(BoundsOverrideTest, isAnyPositive) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{{"container1", {}}});
  const auto universe = buildUniverse();
  auto boundsOverride = createBoundsOverride(const_expr(1, universe), universe);
  EXPECT_FALSE(boundsOverride->isAnyPositive());
}

TEST_F(BoundsOverrideTest, isLinearSum) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{{"container1", {}}});
  const auto universe = buildUniverse();
  auto boundsOverride = createBoundsOverride(
      const_expr(-1, universe) + const_expr(1, universe), universe);
  EXPECT_FALSE(boundsOverride->isLinearSum());
}

TEST_F(BoundsOverrideTest, isInteger) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{{"container1", {}}});
  const auto universe = buildUniverse();
  auto boundsOverride = createBoundsOverride(
      const_expr(-1, universe) + const_expr(2, universe), universe);
  Context context;
  EXPECT_TRUE(boundsOverride->is_integer(context));

  auto boundsOverride2 = createBoundsOverride(
      const_expr(1.1, universe) + const_expr(3, universe), universe);
  EXPECT_FALSE(boundsOverride2->is_integer(context));

  auto boundsOverride3 = std::make_shared<BoundsOverride>(
      const_expr(-1, universe) + const_expr(2.5, universe),
      std::optional(2),
      std::optional(2),
      universe);
  EXPECT_TRUE(boundsOverride3->is_integer(context));
}

TEST_F(BoundsOverrideTest, onlyOneBoundSet) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{{"container1", {}}});
  const auto universe = buildUniverse();
  auto boundsOverride = std::make_shared<BoundsOverride>(
      const_expr(-1, universe) + const_expr(2, universe),
      std::optional<double>(0),
      std::nullopt,
      universe);
  Context context;
  auto [lb, ub] = boundsOverride->lowerAndUpperBounds(context);
  EXPECT_EQ(lb, std::optional<double>(0));
  EXPECT_EQ(ub, std::optional<double>(1));
  auto boundsOverride2 = std::make_shared<BoundsOverride>(
      const_expr(-1, universe) + const_expr(2, universe),
      std::nullopt,
      std::optional<double>(10),
      universe);
  auto [lb2, ub2] = boundsOverride2->lowerAndUpperBounds(context);
  EXPECT_EQ(lb2, 1);
  EXPECT_EQ(ub2, 10);
}

} // namespace facebook::rebalancer::packer::tests
