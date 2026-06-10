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

#include "algopt/rebalancer/solver/expressions/Transform.h"

#include "algopt/rebalancer/solver/expressions/BottomToTopEvaluator.h"
#include "algopt/rebalancer/solver/expressions/Evaluator.h"
#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"
#include "algopt/rebalancer/solver/expressions/Piecewise.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/iterators/Transform.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/equivalence_sets/EquivalenceSets.h"

#include <folly/container/Enumerate.h>

#include <sstream>

namespace facebook::rebalancer {

constexpr size_t kDefaultPieceCount = 100;

Transform::Transform(
    std::shared_ptr<Expression> expr,
    std::shared_ptr<const entities::Universe> universe,
    std::optional<ApproximationHint> hint)
    : Expression(std::move(universe)) {
  add_child(std::move(expr));

  if (hint.has_value() && hint.value().valid) {
    soft_bounds =
        std::make_pair(hint.value().lower_bound, hint.value().upper_bound);
    piece_count = hint.value().piece_count;
  } else {
    piece_count = kDefaultPieceCount;
  }
}

void Transform::updateEquivalenceSets(EquivalenceSets& equivalenceSets) const {
  if (auto var = getOnlyChildRawPtr()->getVar()) {
    auto [obj, cont] = *var;
    PackerMap<entities::ObjectId, entities::ContainerId> mapping;
    mapping.emplace(obj, cont);
    equivalenceSets.mappingMerge(mapping);
  }
}

double Transform::evaluate(
    const BottomToTopEvaluator& evaluator,
    const ChangeSet& changes) const {
  return perform_transform(evaluator.evaluate(getOnlyChildRawPtr(), changes));
}

double Transform::_applyUsingChildValues(
    const Evaluator& evaluator,
    const Assignment& assignment) {
  value = perform_transform(evaluator.apply(getOnlyChildRawPtr(), assignment));
  return value;
}

double Transform::innerFullApply(
    const TopToBottomEvaluator& evaluator,
    const Assignment& assignment) {
  return _applyUsingChildValues(evaluator, assignment);
}

double Transform::innerPartialApply(
    const BottomToTopEvaluator& evaluator,
    const Assignment& assignment,
    const ChangeSet& /* unused */) {
  return _applyUsingChildValues(evaluator, assignment);
}

Bounds Transform::innerLowerAndUpperBounds(
    Context& context,
    const BoundConstraints& bc) const {
  auto expr = getOnlyChildRawPtr();
  auto [exprLb, exprUb] = expr->lowerAndUpperBounds(context, bc);

  const bool different_children_signs = (exprLb < 0 && exprUb > 0);
  auto fromLb = perform_transform(exprLb);
  auto fromUb = perform_transform(exprUb);

  return {
      // If 0 is in range, it might have a different value for some transforms.
      .lower_bound = different_children_signs
          ? std::min({fromUb, fromLb, perform_transform(0)})
          : std::min(fromUb, fromLb),
      .upper_bound = std::max(fromUb, fromLb)};
}

AbstractContainer<ObjectPotential> Transform::getObjectPotentials(
    bool descending) const {
  const double currentInput = getOnlyChildRawPtr()->value;
  const double currentOutput = value;
  const auto transform =
      [this, currentInput, currentOutput](ObjectPotential objectPotential) {
        const double newInput = currentInput - objectPotential.potential;
        const double newOutput = perform_transform(newInput);
        return ObjectPotential{
            .objectId = objectPotential.objectId,
            .potential = currentOutput - newOutput};
      };
  return makeTransformContainer(
      getOnlyChildRawPtr()->getObjectPotentials(descending), transform);
}

std::string Transform::innerDigest(size_t /* unused */) const {
  std::stringstream ss;
  ss << fmt::format("{}(", getType());
  for (const auto& [idx, child] : folly::enumerate(children())) {
    if (idx > 0) {
      ss << ", ";
    }
    ss << fmt::format("{}({})", child->getType(), child->value);
  }
  ss << ")";
  return ss.str();
}

algopt::lp::Expression Transform::approximate_function(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs,
    const std::function<double(double)>& function) {
  auto transformExpr = getOnlyChild();
  auto [hard_lb, hard_ub] = evaluator.lowerAndUpperBounds(transformExpr.get());
  const double soft_lb = soft_bounds ? soft_bounds->first : hard_lb;
  const double soft_ub = soft_bounds ? soft_bounds->second : hard_ub;
  return build_piecewise_linear_function(
      function,
      hard_lb,
      hard_ub,
      soft_lb,
      soft_ub,
      piece_count,
      std::move(transformExpr),
      evaluator,
      minimizing,
      configs);
}

} // namespace facebook::rebalancer
