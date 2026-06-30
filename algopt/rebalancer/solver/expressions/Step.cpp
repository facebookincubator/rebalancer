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

#include <stdexcept>

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

  // Native indicator-constraint path. We commit to it only after confirming the
  // backend supports indicator constraints, so we never leave the two
  // contradictory hard constraints (childLp >= tolerance AND childLp <= 0) in
  // the model when falling through to the Big-M formulation below.
  if (evaluator.supportsIndicatorConstraints()) {
    const auto& childLp = child;
    auto stepVar = lp_bool_var(evaluator, "step");
    // Mirror the Big-M smallestPositiveValue logic: integer children need a
    // threshold of 1.0 so the LP relaxation cannot satisfy stepVar==1 while
    // child sits between 0 and 1; continuous children use the absolute
    // tolerance.
    const double posThreshold = childIsInteger ? 1.0 : tolerance;
    // stepVar == 1 => childLp >= posThreshold (child is strictly positive)
    auto posConstraint = evaluator.addLpConstraint(
        childLp >= posThreshold, "step_indicator_pos");
    // stepVar == 0 => childLp <= 0 (child is non-positive)
    auto zeroConstraint =
        evaluator.addLpConstraint(childLp <= 0, "step_indicator_zero");
    // Indicator direction: 1 = "var==1 activates constraint", 0 = "var==0
    // activates". supportsIndicatorConstraints() guarantees the backend accepts
    // indicators, so both calls are expected to succeed. Evaluate both
    // unconditionally (no short-circuit) so that a partial failure doesn't
    // leave one constraint as a conditional indicator and the other as an
    // unconditional hard constraint. If either fails, the Problem now holds the
    // two contradictory hard constraints (childLp >= posThreshold AND
    // childLp <= 0) and cannot be recovered: callers MUST discard the entire
    // Problem on this throw, not catch it and retry only the Step encoding.
    const bool posOk =
        evaluator.setIndicatorOnConstraint(posConstraint, stepVar, 1);
    const bool zeroOk =
        evaluator.setIndicatorOnConstraint(zeroConstraint, stepVar, 0);
    if (posOk && zeroOk) {
      return stepVar;
    }
    throw std::runtime_error(
        "Native Step indicator attachment failed despite the backend "
        "reporting indicator-constraint support; Problem must be discarded");
  }

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
