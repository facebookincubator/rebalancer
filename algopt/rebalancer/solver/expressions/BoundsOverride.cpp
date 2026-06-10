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

#include <optional>

namespace {
constexpr std::string_view type = "BoundsOverride";
}

namespace facebook::rebalancer {

BoundsOverride::BoundsOverride(
    std::shared_ptr<Expression> child,
    std::optional<double> lowerBound,
    std::optional<double> upperBound,
    std::shared_ptr<const entities::Universe> universe)
    : Expression(std::move(universe)),
      lowerBound_(lowerBound),
      upperBound_(upperBound) {
  if (child == nullptr) {
    throw std::runtime_error("Child cannot be null");
  }
  if (lowerBound == std::nullopt && upperBound == std::nullopt) {
    throw std::runtime_error("Must at least override 1 bound");
  }
  add_child(child);
}

const std::string_view& BoundsOverride::getType() const {
  return type;
}

Bounds BoundsOverride::innerLowerAndUpperBounds(
    Context& context,
    const BoundConstraints& bc) const {
  auto expr = getOnlyChildRawPtr();

  if (lowerBound_ == std::nullopt || upperBound_ == std::nullopt) {
    // if only one bound is set, we need to get the other from expr
    auto [exprLb, exprUb] = expr->lowerAndUpperBounds(context, bc);
    return {
        .lower_bound = lowerBound_.value_or(exprLb),
        .upper_bound = upperBound_.value_or(exprUb)};
  } else {
    return {
        .lower_bound = lowerBound_.value(), .upper_bound = upperBound_.value()};
  }
}

bool BoundsOverride::inner_is_integer(Context& context) {
  // either we determine it is an integer from the overridden bounds (this
  // happens when lowerBound=upperBound, and lowerBound is an integer) or the
  // child is an integer
  const auto& precision = getPrecision();
  return (lowerBound_.has_value() && upperBound_.has_value() &&
          precision.compare(*lowerBound_, *upperBound_) == 0 &&
          precision.isInteger(*lowerBound_)) ||
      getOnlyChildRawPtr()->inner_is_integer(context);
}

double BoundsOverride::evaluate(
    const BottomToTopEvaluator& evaluator,
    const ChangeSet& changes) const {
  return getOnlyChildRawPtr()->evaluate(evaluator, changes);
}

double BoundsOverride::innerFullApply(
    const TopToBottomEvaluator& evaluator,
    const Assignment& assignment) {
  value = getOnlyChildRawPtr()->fullApply(evaluator, assignment);
  return value;
}

double BoundsOverride::innerPartialApply(
    const BottomToTopEvaluator& evaluator,
    const Assignment& assignment,
    const ChangeSet& changes) {
  value = getOnlyChildRawPtr()->partialApply(evaluator, assignment, changes);
  return value;
}

algopt::lp::Expression BoundsOverride::lp(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs) {
  return getOnlyChildRawPtr()->lp(evaluator, minimizing, configs);
}

void BoundsOverride::updateEquivalenceSets(
    EquivalenceSets& equivalenceSets) const {
  getOnlyChildRawPtr()->updateEquivalenceSets(equivalenceSets);
}

AbstractContainer<ObjectPotential> BoundsOverride::getObjectPotentials(
    bool descending) const {
  return getOnlyChildRawPtr()->getObjectPotentials(descending);
}

ExpressionProperties BoundsOverride::getProperties() const {
  return getOnlyChildRawPtr()->getProperties();
}

std::string BoundsOverride::innerDigest(size_t maxChildren) const {
  return getOnlyChildRawPtr()->innerDigest(maxChildren);
}

std::optional<std::pair<entities::ObjectId, entities::ContainerId>>
BoundsOverride::getVar() const {
  return getOnlyChildRawPtr()->getVar();
}

std::vector<std::pair<Expression*, double>> BoundsOverride::get_sorted_children(
    bool descending) const {
  return getOnlyChildRawPtr()->get_sorted_children(descending);
}

std::optional<AffectedByChange> BoundsOverride::isAffectedByChange(
    const AffectedByChangeDecisionData& data) const {
  return getOnlyChildRawPtr()->isAffectedByChange(data);
}

double BoundsOverride::getChildCoefficient(Expression* child) const {
  return getOnlyChildRawPtr()->getChildCoefficient(child);
}

bool BoundsOverride::hasNoLpIntent() const {
  return getOnlyChildRawPtr()->hasNoLpIntent();
}

void BoundsOverride::lpIntent(const LpEvaluator& evaluator, bool minimizing) {
  return getOnlyChildRawPtr()->lpIntent(evaluator, minimizing);
}

} // namespace facebook::rebalancer
