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

#include "algopt/rebalancer/solver/expressions/LinearSum.h"

#include "algopt/lp/generic/Operators.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"
#include "algopt/rebalancer/solver/expressions/BottomToTopEvaluator.h"
#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"
#include "algopt/rebalancer/solver/expressions/PropertiesHelper.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/iterators/ObjectPotentialsMerge.h"
#include "algopt/rebalancer/solver/iterators/Transform.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/equivalence_sets/EquivalenceSets.h"

#include <folly/container/Enumerate.h>

#include <utility>
#include <vector>

namespace {
constexpr std::string_view type = "LinearSum";
}

namespace facebook::rebalancer {

using entities::ContainerId;
using entities::EquivalenceSetId;
using entities::ObjectId;

LinearSum::LinearSum(
    std::shared_ptr<const entities::Universe> universe,
    double constant,
    const PackerMap<std::shared_ptr<Expression>, double>& coef)
    : Expression(std::move(universe)), constant_(constant) {
  for (auto [child, weight] : coef) {
    if (weight == 0) {
      continue;
    }
    add_child(child);
    coefficients_.emplace(child.get(), weight);
    if (weight != 1.0) {
      allCoeffsOne_ = false;
    }
  }

  value = constant;
}

const std::string_view& LinearSum::getType() const {
  return type;
}

bool LinearSum::isLinearSum() const {
  return true;
}

bool LinearSum::inner_is_integer(Context& context) {
  if (floor(constant_) != constant_) {
    return false;
  }
  for (const auto& child : children()) {
    const double weight = getChildCoefficient(child.get());
    if (floor(weight) != weight) {
      return false;
    }
    if (!child->is_integer(context)) {
      return false;
    }
  }
  return true;
}

double LinearSum::evaluate(
    const BottomToTopEvaluator& evaluator,
    const ChangeSet& changes) const {
  double sum = value;
  auto& changedChildren = evaluator.getChangedChildren((Expression*)this);
  for (auto expr : changedChildren) {
    sum += expr->delta(evaluator, changes) * getChildCoefficient(expr);
  }

  return getPrecision().isZero(fabs(sum)) ? 0 : sum;
}

double LinearSum::innerFullApply(
    const TopToBottomEvaluator& evaluator,
    const Assignment& assignment) {
  collection.clear();
  for (const auto& child : children()) {
    const double coefficient = getChildCoefficient(child.get());
    collection.update(
        child.get(), evaluator.apply(child.get(), assignment) * coefficient);
  }
  value = constant_ + collection.query();

  // TODO: Improve precision handling:
  // - make cutover be also relative to the max of the individual item
  // (here and in upper/lower bounds, etc)
  if (getPrecision().isZero(fabs(value))) {
    value = 0;
  }
  return value;
}

double LinearSum::innerPartialApply(
    const BottomToTopEvaluator& evaluator,
    const Assignment& assignment,
    [[maybe_unused]] const ChangeSet& changes) {
  auto& changedChildren = evaluator.getChangedChildren((Expression*)this);
  for (auto expr : changedChildren) {
    collection.update(
        expr, evaluator.apply(expr, assignment) * getChildCoefficient(expr));
  }
  value = constant_ + collection.query();
  if (getPrecision().isZero(fabs(value))) {
    value = 0;
  }
  return value;
}

Bounds LinearSum::innerLowerAndUpperBounds(
    Context& context,
    const BoundConstraints& bc) const {
  double lb = constant_;
  double ub = constant_;

  for (const auto& expr : children()) {
    auto coef = getChildCoefficient(expr.get());
    if (coef == 0) {
      continue;
    }

    auto [childLb, childUb] = expr->lowerAndUpperBounds(context, bc);
    if (coef > 0) {
      lb += childLb * coef;
      ub += childUb * coef;
    } else {
      lb += childUb * coef;
      ub += childLb * coef;
    }
  }
  const auto& precision = getPrecision();
  if (precision.isZero(fabs(lb))) {
    lb = 0;
  }
  if (precision.isZero(fabs(ub))) {
    ub = 0;
  }
  return {.lower_bound = lb, .upper_bound = ub};
}

void LinearSum::lpIntent(const LpEvaluator& evaluator, bool minimizing) {
  for (const auto& child : children()) {
    const double coef = getChildCoefficient(child.get());
    if (coef != 0 && evaluator.isChildDynamic(this, child.get()) &&
        !child->getVar()) {
      evaluator.computeLpIntent(child, (minimizing && coef > 0));
    }
  }
}

algopt::lp::Expression LinearSum::lp(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs) {
  auto expr = evaluator.makeLpExpression(constant_);
  for (const auto& child : children()) {
    const double coef = getChildCoefficient(child.get());
    if (!evaluator.isChildDynamic(this, child.get())) {
      expr += coef * child->value;
      continue;
    }
    if (coef != 0) {
      const bool shouldMinimize = minimizing && coef > 0;
      expr += coef * evaluator.lp(child.get(), shouldMinimize, configs);
    }
  }

  return expr;
}

void LinearSum::updateEquivalenceSets(EquivalenceSets& equivalenceSets) const {
  PackerMap<ObjectId, std::vector<std::pair<entities::ContainerId, double>>>
      mapping;
  for (const auto& child : children()) {
    if (auto var = child->getVar()) {
      auto [obj, cont] = *var;
      const double coefficient = getChildCoefficient(child.get());
      mapping[obj].emplace_back(cont, coefficient);
    }
  }
  for (auto& it : mapping) {
    sort(it.second.begin(), it.second.end());
  }
  equivalenceSets.mappingMerge(mapping);
}

AbstractContainer<ObjectPotential> LinearSum::getObjectPotentials(
    bool descending) const {
  std::vector<AbstractContainer<ObjectPotential>> potentials;
  potentials.reserve(children().size());
  for (const auto& child : children()) {
    const double coefficient = getChildCoefficient(child.get());
    const auto multiplier = [coefficient =
                                 coefficient](ObjectPotential objectPotential) {
      return ObjectPotential{
          .objectId = objectPotential.objectId,
          .potential = coefficient * objectPotential.potential};
    };
    if (coefficient > 0) {
      potentials.push_back(makeTransformContainer(
          child->getObjectPotentials(descending), multiplier));
    } else if (coefficient < 0) {
      potentials.push_back(makeTransformContainer(
          child->getObjectPotentials(!descending), multiplier));
    }
  }
  return makeObjectPotentialsMergeContainer(potentials, descending);
}

ExpressionProperties LinearSum::getProperties() const {
  ExpressionProperties properties;
  properties.properties()->emplace(
      "constant", PropertiesHelper::makeDoubleValue(constant_));
  return properties;
}

std::string LinearSum::innerDigest(size_t maxChildren) const {
  std::string result = fmt::format("= {}", constant_);
  if (children().empty()) {
    return result;
  }
  for (const auto& [idx, child] : folly::enumerate(children())) {
    if (idx >= maxChildren) {
      result += fmt::format("... {} more", children().size() - maxChildren);
      break;
    }
    const double coefficient = getChildCoefficient(child.get());
    result += fmt::format(
        " + {}*{}({})", coefficient, child->getType(), child->value);
  }
  return result;
}

LinearSum LinearSum::operator+(double other) const {
  const double constant = constant_ + other;
  PackerMap<std::shared_ptr<Expression>, double> terms;
  for (const auto& child : children()) {
    double coefficient = getChildCoefficient(child.get());
    terms.emplace(child, coefficient);
  }
  return LinearSum(getUniversePtr(), constant, terms);
}

LinearSum LinearSum::operator+(const LinearSum& other) const {
  const double constant = constant_ + other.constant_;
  PackerMap<std::shared_ptr<Expression>, double> coef;

  for (const auto& child : children()) {
    coef[child] += getChildCoefficient(child.get());
  }

  for (const auto& child : other.children()) {
    coef[child] += other.getChildCoefficient(child.get());
  }

  return LinearSum(getUniversePtr(), constant, coef);
}

void LinearSum::combine(const LinearSum& other, double scale) {
  constant_ += other.constant_ * scale;

  const auto& thisChildren = children();
  for (const auto& child : other.children()) {
    if (!thisChildren.contains(child)) {
      add_child(child);
    }
    const double coefficient = other.getChildCoefficient(child.get());
    const double newCoeff = (coefficients_[child.get()] += coefficient * scale);
    if (newCoeff != 1.0) {
      allCoeffsOne_ = false;
    }
  }
}

void LinearSum::operator+=(double other) {
  constant_ += other;
}

void LinearSum::operator+=(const LinearSum& other) {
  combine(other, 1);
}

void LinearSum::operator-=(double other) {
  constant_ -= other;
}

void LinearSum::operator-=(const LinearSum& other) {
  combine(other, -1);
}

LinearSum LinearSum::operator*(double other) const {
  const double constant = constant_ * other;

  PackerMap<std::shared_ptr<Expression>, double> coef;

  for (const auto& child : children()) {
    const double coefficient = getChildCoefficient(child.get());
    coef[child] = coefficient * other;
  }

  return LinearSum(getUniversePtr(), constant, coef);
}

void LinearSum::operator*=(double other) {
  constant_ *= other;

  for (auto& [child, coefficient] : coefficients_) {
    coefficient *= other;
  }
  if (other != 1.0) {
    allCoeffsOne_ = false;
  }
}

void LinearSum::operator/=(double other) {
  *this *= (1 / other);
}

bool LinearSum::operator==(const double other) const {
  if (constant_ != other) {
    return false;
  }
  if (children().size() != 0) {
    return false;
  }
  return true;
}

void LinearSum::add(std::shared_ptr<Expression> expr, double coef) {
  add_child(expr);
  coefficients_.emplace(expr.get(), coef);
  if (coef != 1.0) {
    allCoeffsOne_ = false;
  }
}

double LinearSum::getChildCoefficient(Expression* child) const {
  return allCoeffsOne_ ? 1.0 : folly::get_default(coefficients_, child, 0);
}

} // namespace facebook::rebalancer
