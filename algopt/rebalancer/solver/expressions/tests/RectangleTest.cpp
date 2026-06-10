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

#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class RectangleTest : public ExpressionTestsBase {
 protected:
  void SetUp() override {
    constexpr int kNumContainers = 20;
    entities::Map<std::string, std::vector<std::string>> initialAssignment;
    for (const auto i : folly::irange(kNumContainers)) {
      initialAssignment[fmt::format("container{}", i)] = {
          fmt::format("object{}", i)};
    }
    setInitialAssignment(initialAssignment);
  }
};

TEST_F(RectangleTest, Constants) {
  const auto universe = buildUniverse();
  {
    auto expr = rectangle(const_expr(-1.01, universe), 0, 1, universe);
    EXPECT_EQ(0, apply(expr, {}));
  }
  {
    auto expr = rectangle(const_expr(-0.99, universe), -1, 0, universe);
    EXPECT_EQ(1.0, apply(expr, {}));
  }
  {
    auto expr = rectangle(const_expr(0, universe), 0, 1, universe);
    EXPECT_EQ(1.0, apply(expr, {}));
  }
  {
    auto expr = rectangle(const_expr(1e-12, universe), -1, 1, universe);
    EXPECT_EQ(1.0, apply(expr, {}));
  }
  {
    auto expr = rectangle(const_expr(1 - 1e-12, universe), -1, 0.9, universe);
    EXPECT_EQ(0.0, apply(expr, {}));
  }
  {
    auto expr = rectangle(const_expr(1, universe), -1, 1, universe);
    EXPECT_EQ(1.0, apply(expr, {}));
  }
  {
    auto expr = rectangle(const_expr(1.01, universe), 0, 1, universe);
    EXPECT_EQ(0.0, apply(expr, {}));
  }
}

TEST_F(RectangleTest, NonConstant) {
  const auto universe = buildUniverse();
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  auto expr =
      2 * variable(object(1), container(0), universe, initialAssignment) - 1.5;
  const Assignment assignment({
      {container(0), {}},
      {container(1), {object(1)}},
  });

  auto rectangleExpr = rectangle(expr, -1, 1, universe);

  EXPECT_EQ(0.0, apply(rectangleExpr, assignment));

  EXPECT_EQ(
      1.0, evaluate(rectangleExpr, {{object(1), container(0)}}, assignment));

  EXPECT_EQ(
      1.0,
      applyChanges(rectangleExpr, {{object(1), container(0)}}, assignment));
}
} // namespace facebook::rebalancer::packer::tests
