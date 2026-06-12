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
  auto expr = getOnlyChildRawPtr();
  auto [exprLb, exprUb] = evaluator.lowerAndUpperBounds(expr);
  auto ub = scaled_bound(exprUb);
  const auto& precision = getPrecision();
  const auto tolerance = precision.getTolerances().absolute().value();

  // STEP will always be 0
  if (precision.compare(ub, tolerance) <= 0) {
    REBALANCER_NEWCTR(evaluator.lp(expr, minimizing, configs) <= tolerance);
    return evaluator.makeLpExpression(0);
  }

  // STEP will alway be 1
  if (precision.isStrictlyGtZero(exprLb)) {
    REBALANCER_NEWCTR(evaluator.lp(expr, minimizing, configs) >= -tolerance);
    return evaluator.makeLpExpression(1);
  }

  // At this stage exprLb <= algopt::kEpsilon and ub >=
  // algopt::kEpsilon
  auto var = lp_bool_var(evaluator, "step");

  auto& childVar = evaluator.lp(expr, minimizing, configs);
  REBALANCER_NEWCTR(childVar <= ub * var);

  if (!minimizing) {
    // NOTE: there is a multiplication by 2 below since Xpress seems to ignore
    // the 'smallestPositiveValue' from the relation below when it is set equal
    // to the integer tolerance
    const double smallestPositiveValue =
        expr->is_integer(evaluator.getContext())
        ? 1.0
        : 2 * evaluator.getIntegerTolerance();
    REBALANCER_NEWCTR(
        (exprLb - 1) * (1 - var) + smallestPositiveValue <= childVar);
  }

  return var;
}

} // namespace facebook::rebalancer
