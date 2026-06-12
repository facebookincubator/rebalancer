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

#include "algopt/rebalancer/solver/expressions/Square.h"

#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"

namespace {
constexpr std::string_view type = "Square";
}

namespace facebook::rebalancer {

Square::Square(
    std::shared_ptr<Expression> expr,
    const ApproximationHint& hint,
    std::shared_ptr<const entities::Universe> universe)
    : Transform(
          std::move(expr),
          std::move(universe),
          std::optional<ApproximationHint>{hint}) {
  setInitialValue(perform_transform(getOnlyChildRawPtr()->getInitialValue()));
}

Square::Square(
    std::shared_ptr<Expression> expr,
    std::shared_ptr<const entities::Universe> universe)
    : Transform(std::move(expr), std::move(universe)) {
  setInitialValue(perform_transform(getOnlyChildRawPtr()->getInitialValue()));
}

const std::string_view& Square::getType() const {
  return type;
}

double Square::perform_transform(double val) const {
  return val * val;
}

bool Square::inner_is_integer(Context& /* not used */) {
  return false;
}

void Square::lpIntent(const LpEvaluator& evaluator, bool minimizing) {
  evaluator.computeLpIntent(getOnlyChild(), minimizing);
}

algopt::lp::Expression Square::lp(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs) {
  return approximate_function(
      evaluator, minimizing, configs, [](double x) { return x * x; });
}

} // namespace facebook::rebalancer
