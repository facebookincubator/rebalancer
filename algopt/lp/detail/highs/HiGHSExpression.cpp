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

#ifdef REBALANCER_USE_HIGHS

#include "algopt/lp/detail/highs/HiGHSExpression.h"

#include "algopt/lp/detail/highs/HiGHSRelation.h"

#include <folly/container/irange.h>

#include <iostream>

namespace facebook::algopt::lp::detail {

HiGHSExpression::HiGHSExpression(double coeff) : constant_{coeff} {}

HiGHSExpression::HiGHSExpression(const HiGHSVariable& var, double coeff)
    : variables_{var}, coeffs_{coeff} {}

std::shared_ptr<ExpressionImpl> HiGHSExpression::clone() const {
  return std::make_shared<HiGHSExpression>(*this);
}

void HiGHSExpression::add(double constant) {
  constant_ += constant;
}

void HiGHSExpression::add(std::shared_ptr<const ExpressionImpl> expression) {
  const auto& expr = dynamic_cast<const HiGHSExpression&>(*expression);
  variables_.insert(
      variables_.end(), expr.variables_.begin(), expr.variables_.end());
  coeffs_.insert(coeffs_.end(), expr.coeffs_.begin(), expr.coeffs_.end());
  constant_ += expr.constant_;
  for (const auto& [pair, coeff] : expr.quadraticCoeffs_) {
    quadraticCoeffs_[pair] += coeff;
  }
}

void HiGHSExpression::multiply(double constant) {
  for (auto& coeff : coeffs_) {
    coeff *= constant;
  }
  for (auto& [_, coeff] : quadraticCoeffs_) {
    coeff *= constant;
  }
  constant_ *= constant;
}

static HiGHSExpression::ColPair makeOrderedPair(HighsInt a, HighsInt b) {
  return a <= b ? std::make_pair(a, b) : std::make_pair(b, a);
}

void HiGHSExpression::multiply(
    std::shared_ptr<const ExpressionImpl> expression) {
  const auto& rhs = dynamic_cast<const HiGHSExpression&>(*expression);

  if (!quadraticCoeffs_.empty() || !rhs.quadraticCoeffs_.empty()) {
    throw std::runtime_error(
        "Multiplication of a quadratic expression by non-constant is not supported");
  }

  if (variables_.empty()) {
    const auto origConstant = constant_;
    *this = rhs;
    multiply(origConstant);
    return;
  }

  if (rhs.variables_.empty()) {
    multiply(rhs.constant_);
    return;
  }

  folly::F14FastMap<HighsInt, double> lhsColToCoeff;
  for (const auto i : folly::irange(variables_.size())) {
    lhsColToCoeff[variables_[i].getColIndex()] += coeffs_[i];
  }
  folly::F14FastMap<HighsInt, double> rhsColToCoeff;
  for (const auto i : folly::irange(rhs.variables_.size())) {
    rhsColToCoeff[rhs.variables_[i].getColIndex()] += rhs.coeffs_[i];
  }

  for (const auto& [col1, coef1] : lhsColToCoeff) {
    for (const auto& [col2, coef2] : rhsColToCoeff) {
      const auto pair = makeOrderedPair(col1, col2);
      quadraticCoeffs_[pair] += coef1 * coef2;
    }
  }

  for (auto& coeff : coeffs_) {
    coeff *= rhs.constant_;
  }

  for (const auto i : folly::irange(rhs.variables_.size())) {
    variables_.push_back(rhs.variables_[i]);
    coeffs_.push_back(constant_ * rhs.coeffs_[i]);
  }

  constant_ *= rhs.constant_;
}

std::shared_ptr<RelationImpl> HiGHSExpression::makeEqualZeroRelation() const {
  return std::make_shared<HiGHSRelation>(*this, Relation::EQ_ZERO);
}

std::shared_ptr<RelationImpl> HiGHSExpression::makeLessEqualZeroRelation()
    const {
  return std::make_shared<HiGHSRelation>(*this, Relation::LE_ZERO);
}

std::shared_ptr<RelationImpl> HiGHSExpression::makeGreaterEqualZeroRelation()
    const {
  return std::make_shared<HiGHSRelation>(*this, Relation::GE_ZERO);
}

double HiGHSExpression::getValue() const {
  double value = constant_;
  for (const auto i : folly::irange(variables_.size())) {
    value += coeffs_[i] * variables_[i].getValue();
  }
  if (!quadraticCoeffs_.empty() && !variables_.empty()) {
    const auto& solution = variables_[0].getHighs()->getSolution().col_value;
    for (const auto& [pair, coeff] : quadraticCoeffs_) {
      const auto [col1, col2] = pair;
      value += coeff * solution[col1] * solution[col2];
    }
  }
  return value;
}

double HiGHSExpression::computeValue() const {
  // For now, this returns the same value as getValue
  return getValue();
}

void HiGHSExpression::print() const {
  for (const auto i : folly::irange(variables_.size())) {
    std::cout << coeffs_[i] << " * " << variables_[i].getName();
    if (i < variables_.size() - 1) {
      std::cout << " + ";
    }
  }
  if (constant_ != 0) {
    if (!variables_.empty()) {
      if (constant_ < 0) {
        std::cout << " - " << -constant_;
      } else {
        std::cout << " + " << constant_;
      }
    } else {
      std::cout << constant_;
    }
  }
}

size_t HiGHSExpression::size() const {
  return variables_.size();
}

const std::vector<HiGHSVariable>& HiGHSExpression::getVariables() const {
  return variables_;
}

const std::vector<double>& HiGHSExpression::getCoeffs() const {
  return coeffs_;
}

double HiGHSExpression::getConstant() const {
  return constant_;
}

bool HiGHSExpression::isQuadratic() const {
  return !quadraticCoeffs_.empty();
}

const folly::F14FastMap<HiGHSExpression::ColPair, double>&
HiGHSExpression::getQuadraticCoeffs() const {
  return quadraticCoeffs_;
}

} // namespace facebook::algopt::lp::detail

#endif
