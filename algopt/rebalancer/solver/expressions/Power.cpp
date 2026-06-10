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

#include "algopt/rebalancer/solver/expressions/Power.h"

#include "algopt/lp/generic/Operators.h"
#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"
#include "algopt/rebalancer/solver/expressions/PropertiesHelper.h"
#include "algopt/rebalancer/solver/expressions/Transform.h"

namespace {
constexpr std::string_view type = "Power";
}

namespace facebook::rebalancer {

Power::Power(
    std::shared_ptr<Expression> expr,
    double exponent,
    std::shared_ptr<const entities::Universe> universe)
    : Transform(std::move(expr), std::move(universe)) {
  exponent_ = exponent;
}

const std::string_view& Power::getType() const {
  return type;
}

double Power::perform_transform(double val) const {
  return pow(val, exponent_);
}

bool Power::inner_is_integer(Context& /* not used */) {
  return false;
}

void Power::lpIntent(const LpEvaluator& evaluator, bool minimizing) {
  evaluator.computeLpIntent(getOnlyChild(), minimizing);
}

algopt::lp::Expression Power::lp(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs) {
  if (minimizing && exponent_ == 2) {
    // If the intent is to minimize and the exponent is 2, we represent it
    // natively in LP by using quadratic expressions.
    auto& childLp = evaluator.lp(getOnlyChildRawPtr(), minimizing, configs);
    auto tempVar = lp_cont_var(evaluator, "quad_temp");
    REBALANCER_NEWCTR(childLp <= tempVar);
    return tempVar * tempVar;
  }

  // Otherwise, we approximate it with a piecewise linear function.
  return approximate_function(evaluator, minimizing, configs, [this](double x) {
    return pow(x, exponent_);
  });
}

ExpressionProperties Power::getProperties() const {
  ExpressionProperties properties;
  properties.properties()->emplace(
      "exponent", PropertiesHelper::makeDoubleValue(exponent_));
  return properties;
}

} // namespace facebook::rebalancer
