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

#include "algopt/rebalancer/solver/expressions/Max.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class MaxTest : public ExpressionTestsBase {
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

TEST_F(MaxTest, MaxIsBinary) {
  const auto universe = buildUniverse();
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  auto var = variable(object(0), container(0), universe, initialAssignment);
  auto sum = var * 3.5 + 8;
  auto expr = rebalancer::max(
      {sum, const_expr(-12, universe), const_expr(10, universe)}, universe);

  Context context;
  EXPECT_FALSE(expr->is_binary(context));
  expr = rebalancer::max({var}, universe);
  EXPECT_TRUE(expr->is_binary(context));
  expr = rebalancer::max(
      {var, const_expr(0, universe), const_expr(1, universe)}, universe);
  EXPECT_TRUE(expr->is_integer(context));
  EXPECT_TRUE(expr->is_binary(context));
  sum = var + const_expr(0, universe);
  expr = rebalancer::max({var, sum, const_expr(1, universe)}, universe);
  EXPECT_TRUE(expr->is_binary(context));
}

TEST_F(MaxTest, BinaryMinIsBinary) {
  const auto universe = buildUniverse();
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  auto var0 = variable(object(0), container(0), universe, initialAssignment);
  auto var1 = variable(object(1), container(0), universe, initialAssignment);
  auto sum1 = 1 - var0;
  auto sum2 = 1 - var1;
  auto expr1 = 1 - rebalancer::max({sum1, sum2}, universe);
  auto expr2 = rebalancer::min({var0, var1}, universe);

  Context context;
  EXPECT_TRUE(var0->is_integer(context));
  EXPECT_TRUE(var0->is_binary(context));
  EXPECT_TRUE(sum1->is_binary(context));
  EXPECT_TRUE(sum2->is_binary(context));
  EXPECT_TRUE(expr1->is_binary(context));
  EXPECT_TRUE(expr2->is_binary(context));
}

TEST_F(MaxTest, MaxBoundsTests) {
  const auto universe = buildUniverse();
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  auto var = variable(object(0), container(0), universe, initialAssignment);
  auto sum2 = var * 3.5 + 8;
  auto expr = rebalancer::max(
      {sum2, const_expr(-12, universe), const_expr(10, universe)}, universe);

  EXPECT_EQ(11.5, upper_bound(*expr));
  EXPECT_EQ(10, lower_bound(*expr));

  // ensure that children's unconstrained bounds have also been computed and
  // memoized
  EXPECT_EQ(8, lower_bound(*sum2));
  EXPECT_EQ(11.5, upper_bound(*sum2));
}

TEST_F(MaxTest, EquivalenceSetsMax) {
  const auto universe = buildUniverse();
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  auto o1c1 = variable(object(1), container(1), universe, initialAssignment);
  auto o1c2 = variable(object(1), container(2), universe, initialAssignment);
  auto o2c2 = variable(object(2), container(2), universe, initialAssignment);
  auto o3c2 = variable(object(3), container(2), universe, initialAssignment);
  auto expr = rebalancer::max({o1c1, o1c2, o2c2, o3c2}, universe);

  EquivalenceSets equivalenceSets(*universe);
  equivalenceSets.combine(
      std::vector<entities::ObjectId>{object(1), object(2), object(3)});

  updateEquivalenceSets(equivalenceSets, *expr);

  EXPECT_EQ(equivalenceSets.size(), 2);
  EXPECT_NE(equivalenceSets.at(object(1)), equivalenceSets.at(object(2)));
  EXPECT_EQ(equivalenceSets.at(object(2)), equivalenceSets.at(object(3)));
  EXPECT_NE(equivalenceSets.at(object(1)), equivalenceSets.at(object(3)));

  const Assignment assignment(
      {{container(1), {object(1), object(2), object(3)}}});
  EXPECT_EQ(1, apply(expr, assignment));
  {
    ChangeSet changes;
    changes.insert(Change(object(1), container(1), -1));
    EXPECT_FALSE(is_positive(*expr, changes));
  }
  {
    ChangeSet changes;
    changes.insert(Change(object(2), container(2), 1));
    changes.insert(Change(object(1), container(1), -1));
    EXPECT_TRUE(is_positive(*expr, changes));
  }
}

TEST_F(MaxTest, Min) {
  const auto universe = buildUniverse();
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  auto A = variable(object(1), container(0), universe, initialAssignment);
  auto B = variable(object(2), container(0), universe, initialAssignment);
  auto minAB = rebalancer::min({A, B}, universe);
  const Assignment assignment00(
      {{container(0), {}}, {container(1), {object(1), object(2)}}});
  const Assignment assignment01(
      {{container(0), {object(2)}}, {container(1), {object(1)}}});
  const Assignment assignment10(
      {{container(0), {object(1)}}, {container(1), {object(2)}}});
  const Assignment assignment11(
      {{container(0), {object(1), object(2)}}, {container(1), {}}});
  EXPECT_EQ(0, apply(minAB, assignment00));
  EXPECT_EQ(0, apply(minAB, assignment01));
  EXPECT_EQ(0, apply(minAB, assignment10));
  EXPECT_EQ(1, apply(minAB, assignment11));

  auto minExpr = rebalancer::min({4 * A - 1, B + 1}, universe);
  EXPECT_EQ(-1, apply(minExpr, assignment00));
  EXPECT_EQ(-1, apply(minExpr, assignment01));
  EXPECT_EQ(1, apply(minExpr, assignment10));
  EXPECT_EQ(2, apply(minExpr, assignment11));
}

TEST_F(MaxTest, MaxInitialValue) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());

  auto v0c0 = variable(object(0), container(0), universe, assignment);
  auto v1c0 = variable(object(1), container(0), universe, assignment);
  // v0c0=1, v1c0=0 → max(1, 0) = 1
  EXPECT_EQ(1.0, rebalancer::max(v0c0, v1c0, universe)->getInitialValue());
}

TEST_F(MaxTest, MaxAddClearsSortedValues) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());

  auto v0c0 = variable(object(0), container(0), universe, assignment);
  auto v1c0 = variable(object(1), container(0), universe, assignment);

  // Create max(v0c0) → initialValue=1, sorted_values populated
  ExprPtr m = rebalancer::max({v0c0}, universe);
  EXPECT_EQ(1.0, m->getInitialValue());

  // add() inserts the new child into the set
  inplace_max(m, v1c0, universe);
  EXPECT_EQ(1.0, m->getInitialValue());

  auto* maxExpr = dynamic_cast<Max*>(m.get());
  // sorted_values now includes both children
  EXPECT_EQ(2u, maxExpr->sorted_values_.size());
  EXPECT_EQ(2u, m->children().size());
}

TEST_F(MaxTest, MaxAddStaleSortedValuesBreaksEvaluate) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());

  // v0c0=1 (obj0 is in c0), v1c1=1 (obj1 is in c1)
  auto v0c0 = variable(object(0), container(0), universe, assignment);
  auto v1c1 = variable(object(1), container(1), universe, assignment);

  // Create max(v0c0), fullApply populates sorted_values = [(1.0, v0c0)]
  ExprPtr m = rebalancer::max({v0c0}, universe);
  EXPECT_EQ(1.0, apply(m, assignment));

  // Add v1c1 via inplace_max — sorted_values now includes both children
  inplace_max(m, v1c1, universe);

  // Evaluate moving obj0 from c0 → c1.
  // Correct: max(0, 1) = 1 (v0c0 becomes 0, v1c1 unchanged at 1)
  ChangeSet changes;
  changes.insert(Change(object(0), container(0), -1));
  changes.insert(Change(object(0), container(1), 1));
  const double result = evaluate(*m, changes);
  EXPECT_EQ(1.0, result);
}

// Demonstrates that evaluate() depends on sorted_values being in descending
// order. After add() inserts a child with value 0, the set must keep it after
// unchanged children with value 1 — otherwise evaluate()'s early-break
// optimization returns the wrong result.
TEST_F(MaxTest, MaxEvaluateAfterAddRequiresSortedOrder) {
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());

  // v0c0=1 (obj0 in c0), v1c1=1 (obj1 in c1), v2c0=0 (obj2 NOT in c0)
  auto v0c0 = variable(object(0), container(0), universe, assignment);
  auto v1c1 = variable(object(1), container(1), universe, assignment);
  auto v2c0 = variable(object(2), container(0), universe, assignment);

  // Create max(v0c0, v1c1), fullApply → sorted_values = [(1, v0c0), (1, v1c1)]
  ExprPtr m = rebalancer::max({v0c0, v1c1}, universe);
  EXPECT_EQ(1.0, apply(m, assignment));

  // Add v2c0 (value 0) — sorted_values must place it AFTER entries with value 1
  inplace_max(m, v2c0, universe);

  // Evaluate: move obj0 from c0 → c2.
  // v0c0 becomes 0 (changed), v1c1 stays 1 (unchanged), v2c0 stays 0
  // (unchanged) Correct: max(0, 1, 0) = 1 Wrong (if v2c0 appeared before v1c1):
  // evaluate finds v2c0=0 first, returns 0
  ChangeSet changes;
  changes.insert(Change(object(0), container(0), -1));
  changes.insert(Change(object(0), container(2), 1));
  const double result = evaluate(*m, changes);
  EXPECT_EQ(1.0, result);
}

} // namespace facebook::rebalancer::packer::tests
