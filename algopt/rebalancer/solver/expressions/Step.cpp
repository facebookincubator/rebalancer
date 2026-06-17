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

#include "algopt/rebalancer/solver/expressions/Step.h"

#include "algopt/lp/generic/Operators.h"
#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"
#include "algopt/rebalancer/solver/expressions/Transform.h"

namespace {
constexpr std::string_view type = "Step";
}

namespace facebook::rebalancer {

Step::Step(
    std::shared_ptr<Expression> expr,
    std::shared_ptr<const entities::Universe> universe)
    : Transform(std::move(expr), std::move(universe)) {
  setInitialValue(perform_transform(getOnlyChildRawPtr()->getInitialValue()));
}

const std::string_view& Step::getType() const {
  return type;
}

double Step::perform_transform(double val) const {
  return getPrecision().isStrictlyGtZero(val);
}

bool Step::inner_is_integer(Context& /* not used */) {
  return true;
}

void Step::lpIntent(const LpEvaluator& evaluator, bool minimizing) {
  evaluator.computeLpIntent(getOnlyChild(), minimizing);
}

algopt::lp::Expression Step::lp(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs) {
  const auto child = getOnlyChildRawPtr();
  return encodeLp(
      evaluator.lp(child, minimizing, configs),
      evaluator.lowerAndUpperBounds(child),
      child->is_integer(evaluator.getContext()),
      *this,
      evaluator,
      minimizing);
}

algopt::lp::Expression Step::encodeLp(
    const algopt::lp::Expression& child,
    const Bounds& childBounds,
    bool childIsInteger,
    const Expression& constraintOwner,
    const LpEvaluator& evaluator,
    bool minimizing) {
  const auto& [childLb, childUb] = childBounds;
  const auto ub = constraintOwner.scaled_bound(childUb);
  const auto& precision = constraintOwner.getPrecision();
  const auto tolerance = precision.getTolerances().absolute().value();

  // STEP will always be 0
  if (precision.compare(ub, tolerance) <= 0) {
    constraintOwner.newCtr(evaluator, "step_zero", child <= tolerance);
    return evaluator.makeLpExpression(0);
  }

  // STEP will alway be 1
  if (precision.isStrictlyGtZero(childLb)) {
    constraintOwner.newCtr(evaluator, "step_one", child >= -tolerance);
    return evaluator.makeLpExpression(1);
  }

  // At this stage childLb <= algopt::kEpsilon and ub >=
  // algopt::kEpsilon
  auto var = lp_bool_var(evaluator, "step");
  constraintOwner.newCtr(evaluator, "step_ub", child <= ub * var);

  if (!minimizing) {
    // NOTE: there is a multiplication by 2 below since Xpress seems to ignore
    // the 'smallestPositiveValue' from the relation below when it is set equal
    // to the integer tolerance
    const auto smallestPositiveValue =
        childIsInteger ? 1.0 : 2 * evaluator.getIntegerTolerance();
    constraintOwner.newCtr(
        evaluator,
        "step_lb",
        (childLb - 1) * (1 - var) + smallestPositiveValue <= child);
  }

  return var;
}

} // namespace facebook::rebalancer
