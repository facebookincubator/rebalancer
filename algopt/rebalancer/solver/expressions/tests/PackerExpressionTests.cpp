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

#include "algopt/lp/generic/Operators.h"
#include "algopt/rebalancer/solver/expressions/LinearSum.h"
#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"
#include "algopt/rebalancer/solver/expressions/NthLargest.h"
#include "algopt/rebalancer/solver/expressions/ObjectPartition.h"
#include "algopt/rebalancer/solver/expressions/ObjectPartitionLookup.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/Power.h"
#include "algopt/rebalancer/solver/expressions/Square.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"
#include "algopt/rebalancer/solver/iterators/ExpressionContainersIterator.h"
#include "algopt/rebalancer/solver/tests/ExprProblemCreation.h"
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <folly/container/irange.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/Random.h>
#include <gtest/gtest.h>

#include <memory>

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::packer::tests {

class PackerTests : public ExpressionTestsBase {};

// TODO: this test file is too long and should be broken down into
// smaller files (one per expression). Large test files are bad for
// developer productivity because they take longer to compile and navigate.

TEST_F(PackerTests, VectorBoundsTests) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0", "object1"}}});
  const auto universe = buildUniverse();
  auto vec = makeObjectVector({{object(0), 5}, {object(1), -3}}, universe);
  EXPECT_EQ(5, upper_bound(*vec));
  EXPECT_EQ(-3, lower_bound(*vec));
}

TEST_F(PackerTests, VectorTotalTooSmall) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0", "object1"}}});
  const auto universe = buildUniverse();
  EXPECT_THROW(
      makeObjectVector({{object(0), 5}, {object(1), -3}}, -3, 1, universe),
      std::runtime_error);
}

TEST_F(PackerTests, TransformSquareBoundsTests) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0", "object1"}}});
  const auto universe = buildUniverse();
  const PackerMap<entities::ObjectId, double> input = {
      {object(0), 4}, {object(1), 3}};
  auto vector = makeObjectVector(input, universe);
  Square square(vector, universe);
  EXPECT_EQ(49, upper_bound(square));
  EXPECT_EQ(0, lower_bound(square));

  // check children's unconstrained bounds
  EXPECT_EQ(0, lower_bound(*vector));
  EXPECT_EQ(7, upper_bound(*vector));
}

TEST_F(PackerTests, TransformSquareVariableBoundsTests) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0"}}});
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto var = variable(object(0), container(0), universe, assignment);

  auto sum = var * 3 + 8;
  EXPECT_EQ(8, lower_bound(*sum));
  EXPECT_EQ(11, upper_bound(*sum));
  Square square(sum, universe);
  EXPECT_EQ(64, lower_bound(square));
  EXPECT_EQ(121, upper_bound(square));

  auto negative_sum = var * 2 - 7;
  EXPECT_EQ(-7, lower_bound(*negative_sum));
  EXPECT_EQ(-5, upper_bound(*negative_sum));
  Square square2(negative_sum, universe);
  EXPECT_EQ(25, lower_bound(square2));
  EXPECT_EQ(49, upper_bound(square2));

  auto zero_between_sum = var * 2 - 1;
  EXPECT_EQ(-1, lower_bound(*zero_between_sum));
  EXPECT_EQ(1, upper_bound(*zero_between_sum));
  Square square3(zero_between_sum, universe);
  EXPECT_EQ(0, lower_bound(square3));
  EXPECT_EQ(1, upper_bound(square3));
}
TEST_F(PackerTests, TransformCubedBoundsTests) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0", "object1"}}});
  const auto universe = buildUniverse();
  auto vector = makeObjectVector({{object(0), 1}, {object(1), 4}}, universe);
  Power power(vector, 3, universe);
  EXPECT_EQ(125, upper_bound(power));
  EXPECT_EQ(0, lower_bound(power));
}

TEST_F(PackerTests, TransformNegativeCubedBoundsTests) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0", "object1"}}});
  const auto universe = buildUniverse();
  auto vector = makeObjectVector({{object(0), -1}, {object(1), -4}}, universe);
  Power power(vector, 3, universe);
  EXPECT_EQ(0, upper_bound(power));
  EXPECT_EQ(-125, lower_bound(power));
}

TEST_F(PackerTests, TransformNegativePowerBoundsTests) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0", "object1"}}});
  const auto universe = buildUniverse();
  auto vector = makeObjectVector({{object(0), .25}, {object(1), -5}}, universe);
  Power power(vector, -2, universe);
  EXPECT_EQ(pow(.25, -2), 16);
  EXPECT_EQ(16, upper_bound(power));
  EXPECT_EQ(.04, lower_bound(power));
}

TEST_F(PackerTests, Quotient) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {}}, {"container1", {"object1", "object2"}}});
  const auto universe = buildUniverse();
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = 0 * inf;
  std::array<double, 8> v1coeff = {4, 2, 2, 2, -3, 5, 5, 5};
  std::array<double, 8> v1const = {3, -3, -3, 3, -2, -3, 0, -3};
  std::array<double, 8> v2coeff = {2, 2, 2, 3, 3, 3, 3, 3};
  std::array<double, 8> v2const = {1, 2, -5, -2, -2, -2, 0, 0};
  std::array<double, 8> eval = {
      3, -1.5, 3.0 / 5, -3.0 / 2, 1, 3.0 / 2, nan, -inf};
  std::array<double, 8> ub = {7, -0.25, 1, inf, inf, inf, inf, inf};
  std::array<double, 8> lb = {1, -1.5, 1.0 / 5, -inf, -inf, -inf, 0, -inf};

  for (const auto i : folly::irange(8)) {
    auto binary_operation = quotient(
        v1coeff[i] *
                variable(object(1), container(0), universe, initialAssignment) +
            v1const[i],
        v2coeff[i] *
                variable(object(2), container(0), universe, initialAssignment) +
            v2const[i],
        universe);

    const Assignment assignment({{container(1), {object(1), object(2)}}});
    // we have variable(1, 0) = variable(2, 0) = 0
    _apply(*binary_operation, assignment);

    if (std::isinf(eval[i])) {
      EXPECT_EQ(eval[i], evaluate(*binary_operation, {})) << "eval " << i;
    } else if (std::isnan(eval[i])) {
      EXPECT_TRUE(std::isnan(evaluate(*binary_operation, {}))) << "eval " << i;
    } else {
      EXPECT_NEAR(eval[i], evaluate(*binary_operation, {}), 0.001)
          << "eval " << i;
    }
    const double ubv = upper_bound(*binary_operation);
    if (std::isinf(ub[i])) {
      EXPECT_EQ(ub[i], ubv) << "ub" << i;
    } else if (std::isnan(ub[i])) {
      EXPECT_TRUE(std::isnan(ubv)) << "ub" << i;
    } else {
      EXPECT_NEAR(ub[i], ubv, 0.001) << "ub" << i;
    }

    const double lbv = lower_bound(*binary_operation);
    if (std::isinf(lb[i])) {
      EXPECT_EQ(lb[i], lbv) << "lb" << i;
    } else if (std::isnan(lb[i])) {
      EXPECT_TRUE(std::isnan(lbv)) << "lb" << i;
    } else {
      EXPECT_NEAR(lb[i], lbv, 0.001) << "lb" << i;
    }
  }
}

TEST_F(PackerTests, ContainerOrderTest) {
  /*
   *                   SUM=8
   *                /    |   \
   *              8     -10   5
   *            /        |     \
   *        MAX=1      MAX=0  MAX=0
   *        /  |       /  \     \
   *      v2=1 v3=1   v4  v1    v1
   *      c3   c1     c2  c3    c3
   * Desired order when NOT skipping optimal expressions: (c2, c3, c1). In this
   * case, child2 has the maximum potential initially, and both v1 and v4 have
   * zero potential. Although there is no priority among c2/c3 based
   * on potentials, we break ties in favor of expressions that have larger id.
   * Here v4 will have a larger id than v1, and hence the expected order is (c2,
   * c3, c1).
   *
   * Desired order when skipping optimal expressions: (c1, c3). In this case,
   * although child2 has the maximum potential initially, both v1 and v4 have
   * zero potential. Therefore, they will be skipped. Following that, Child1 has
   * the next max potential, and both its children v2 and v3 have potential
   * = 1. Due to deterministic tie-breaking in favor of expression with larger
   * id, the expected order is (c1, c3)
   */
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container1", {"object1"}},
          {"container2", {}},
          {"container3", {"object0"}}});
  const auto universe = buildUniverse();

  // false placement
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  auto v1 = variable(object(1), container(3), universe, initialAssignment);
  // true placement
  auto v2 = variable(object(0), container(3), universe, initialAssignment);
  // true placement
  auto v3 = variable(object(1), container(1), universe, initialAssignment);
  // false placement
  auto v4 = variable(object(1), container(2), universe, initialAssignment);

  Assignment assignment(
      {{container(1), {object(1)}}, {container(3), {object(0)}}});

  auto child1 = rebalancer::max(v2, v3, universe);
  auto child2 = rebalancer::max(v1, v4, universe);
  auto child3 = rebalancer::max({v1}, universe);
  auto linearsum = std::make_shared<LinearSum>(
      universe,
      0,
      PackerMap<std::shared_ptr<Expression>, double>{
          {child1, 8.0}, {child2, -10.0}, {child3, 5.0}});

  auto objective = GlobalObjective::Builder{}
                       .addToObjective(0, linearsum, universe)
                       .build(universe);

  Context context;
  auto objExpr = objective.getOnlyObjective();
  objExpr->init_unconstrained_bounds(context);
  _apply(*objExpr, assignment);

  // verify that expression tree confirms to the representation above
  EXPECT_EQ(linearsum->value, 8);
  auto children =
      linearsum->get_sorted_children(true); // get descending sorted children
  ASSERT_EQ(3, children.size());
  auto child = children.begin();
  EXPECT_EQ(0, child->first->value);
  EXPECT_EQ(-10, child->second);
  child++;
  EXPECT_EQ(1, child->first->value);
  EXPECT_EQ(8, child->second);
  child++;
  EXPECT_EQ(0, child->first->value);
  EXPECT_EQ(5, child->second);

  // verify DescendingChildPotentials
  EXPECT_TRUE(descendingChildPotentialsAsExpected(
      *linearsum,
      {10.0, 8.0, 0.0},
      std::vector<ExprPtr>{child2, child1, child3}));

  {
    // case1: optimal expressions are NOT skipped
    const DescendingExpressionContainersTraversal descending(
        objective.getView());
    std::vector<entities::ContainerId> order(
        descending.begin(), descending.end());
    ASSERT_EQ(3, order.size());
    EXPECT_EQ(container(2), order[0]);
    EXPECT_EQ(container(3), order[1]);
    EXPECT_EQ(container(1), order[2]);
  }

  {
    // case2: optimal expressions are skipped
    const DescendingExpressionContainersTraversal descending(
        objective.getView(), true /*skipOptimalExpressions*/);
    std::vector<entities::ContainerId> order(
        descending.begin(), descending.end());
    ASSERT_EQ(2, order.size());
    EXPECT_EQ(container(1), order[0]);
    EXPECT_EQ(container(3), order[1]);
  }

  {
    // apply the move where object(1) is moved to container(2) and object(0) is
    // moved to container(1)
    Context applyContext;
    assignment.moveTo(object(1), container(2));
    assignment.moveTo(object(0), container(1));
    applyContext.changes() = ChangeSet(
        {Change(object(1), container(1), -1),
         Change(object(1), container(2), 1),
         Change(object(0), container(3), -1),
         Change(object(0), container(1), 1)});
    _applyChanges(*linearsum, applyContext, assignment);

    // Note that although all the children of linearSum have the same potential,
    // the order below follows from the following deterministic tie-breaking
    // rule: 1) first prefer expressions that have non-zero uniquelyAffected +
    // directlyAffected containers, 2) if there is a tie, then prefer
    // expressions that have larger ids. Therefore, here child3 comes first
    // (because it has 1 uniquelyAffectedContainer c3), child2 and child1 are
    // tied (neither have any uniquelyAffectedContainers in their subgraph nor
    // any directlyAffectedContainers); child2 comes before child1 because it
    // has a larger id
    EXPECT_TRUE(descendingChildPotentialsAsExpected(
        *linearsum,
        {0.0, 0.0, 0.0},
        std::vector<ExprPtr>{child3, child2, child1}));
  }
}

TEST_F(PackerTests, EquivalenceSetsBinaryOperation) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container1", {"object1", "object2"}},
          {"container2", {"object3", "object4"}}});
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto o1c1 = variable(object(1), container(1), universe, assignment);
  auto o3c2 = variable(object(3), container(2), universe, assignment);
  auto b = product(o1c1, o3c2, universe);

  EquivalenceSets equivalenceSets(*universe);
  equivalenceSets.combine(
      std::vector<entities::ObjectId>{
          object(1), object(2), object(3), object(4)});

  updateEquivalenceSets(equivalenceSets, *b);

  EXPECT_EQ(equivalenceSets.size(), 3);
  EXPECT_NE(equivalenceSets.at(object(1)), equivalenceSets.at(object(2)));
  EXPECT_EQ(equivalenceSets.at(object(2)), equivalenceSets.at(object(4)));
  EXPECT_NE(equivalenceSets.at(object(1)), equivalenceSets.at(object(3)));
}

TEST_F(PackerTests, EquivalenceSetsTransform) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container1", {"object1", "object2", "object3", "object4"}}});
  const auto universe = buildUniverse();
  auto allObjectIds = std::vector<entities::ObjectId>{
      object(1), object(2), object(3), object(4)};
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto var = variable(object(1), container(1), universe, assignment);
  auto sum = 10 * var + 5;
  auto b = square(sum, universe);

  EquivalenceSets equivalenceSets(*universe);
  equivalenceSets.combine(allObjectIds);

  updateEquivalenceSetsRecursive(equivalenceSets, *b, allObjectIds.size());

  EXPECT_EQ(equivalenceSets.size(), 2);
  EXPECT_NE(equivalenceSets.at(object(1)), equivalenceSets.at(object(2)));
  EXPECT_EQ(equivalenceSets.at(object(2)), equivalenceSets.at(object(4)));
  EXPECT_NE(equivalenceSets.at(object(1)), equivalenceSets.at(object(3)));
}

TEST_F(PackerTests, NthLargest) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{{"container0", {}}});
  const auto universe = buildUniverse();
  std::vector<std::shared_ptr<Expression>> values = {
      const_expr(-10, universe),
      const_expr(-10, universe),
      const_expr(-5, universe),
      const_expr(0, universe),
      const_expr(0.5, universe),
      const_expr(50, universe),
      const_expr(50, universe),
      const_expr(100, universe),
  };
  std::shuffle(values.begin(), values.end(), folly::ThreadLocalPRNG());
  auto test = [&values, &universe](int n, bool unique, double expected) {
    NthLargest nthLargest(values, n, unique, universe);
    const double result = _apply(nthLargest, Assignment());
    EXPECT_EQ(expected, result);
  };
  test(0, false, 100);
  test(1, false, 50);
  test(2, false, 50);
  test(3, false, 0.5);
  test(4, false, 0);
  test(5, false, -5);
  test(6, false, -10);
  test(7, false, -10);
  test(8, false, -10);
  test(1000, false, -10);
  test(0, true, 100);
  test(1, true, 50);
  test(2, true, 0.5);
  test(3, true, 0);
  test(4, true, -5);
  test(5, true, -10);
  test(6, true, -10);
  test(7, true, -10);
  test(8, true, -10);
  test(1000, true, -10);
}

TEST_F(PackerTests, StepTest) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0"}}});
  const auto universe = buildUniverse();

  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto var = 0.6 * variable(object(0), container(0), universe, assignment);
  auto stepExpr = step(var, universe);

  auto p_ptr = createTestProblem(
      universe, {const_expr(0, universe)}, const_expr(0, universe));
  auto& p = *p_ptr;

  const PackerSet<entities::ContainerId> dynamicContainers = {container(0)};
  LpContext context(
      dynamicContainers,
      p.getDynamicEquivalentSets(dynamicContainers),
      p.getOrchestrator().getDynamicChildren(dynamicContainers));

  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  p.lp_store.reset(
      facebook::algopt::getAvailableMIPSolver().value(), false, context);
  const auto nodesPriority = folly::F14FastMap<Expression*, PriorityInfo>();
  const LpEvaluator evaluator(context, p, nodesPriority);
  p.lp_store.setObjective({-1 * stepExpr->lp(evaluator, false, {})});

  p.lp_store.solve();

  EXPECT_NEAR(
      -1.0, *p.lp_store.getLpProblem().getOnlyResult().bestObjective(), 1e-8);
}

} // namespace facebook::rebalancer::packer::tests
