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

#include "algopt/lp/fast/FastExpressionImpl.h"

#include "algopt/lp/fast/FastRelationImpl.h"

#include <stdexcept>

namespace facebook::algopt::lp {

FastExpressionImpl::FastExpressionImpl(double constant) : constant_(constant) {}

FastExpressionImpl::FastExpressionImpl(double coeff, uint64_t variableId)
    : constant_(0) {
  terms_[variableId] = coeff;
}

std::shared_ptr<ExpressionImpl> FastExpressionImpl::clone() const {
  auto cloned = std::make_shared<FastExpressionImpl>(constant_);
  cloned->terms_ = terms_;
  return cloned;
}

void FastExpressionImpl::add(double constant) {
  constant_ += constant;
}

void FastExpressionImpl::add(std::shared_ptr<const ExpressionImpl> expression) {
  const auto& other = static_cast<const FastExpressionImpl&>(*expression);
  for (const auto& [variableId, coefficient] : other.terms_) {
    terms_[variableId] += coefficient;
  }
  constant_ += other.constant_;
}

void FastExpressionImpl::multiply(double constant) {
  for (auto& [variableId, coefficient] : terms_) {
    coefficient *= constant;
  }
  constant_ *= constant;
}

void FastExpressionImpl::multiply(
    std::shared_ptr<const ExpressionImpl> expression) {
  const auto& other = static_cast<const FastExpressionImpl&>(*expression);
  const bool thisIsConstant = terms_.empty();
  const bool otherIsConstant = other.terms_.empty();

  if (!thisIsConstant && !otherIsConstant) {
    throw std::runtime_error(
        "FastExpressionImpl::multiply: multiplication of two non-constant "
        "expressions is not supported");
  }

  if (thisIsConstant) {
    const double c = constant_;
    terms_ = other.terms_;
    constant_ = other.constant_;
    multiply(c);
  } else {
    multiply(other.constant_);
  }
}

std::shared_ptr<RelationImpl> FastExpressionImpl::makeEqualZeroRelation()
    const {
  return std::make_shared<FastRelationImpl>(
      getTerms(), constant_, Relation::EQ_ZERO);
}

std::shared_ptr<RelationImpl> FastExpressionImpl::makeLessEqualZeroRelation()
    const {
  return std::make_shared<FastRelationImpl>(
      getTerms(), constant_, Relation::LE_ZERO);
}

std::shared_ptr<RelationImpl> FastExpressionImpl::makeGreaterEqualZeroRelation()
    const {
  return std::make_shared<FastRelationImpl>(
      getTerms(), constant_, Relation::GE_ZERO);
}

double FastExpressionImpl::getValue() const {
  throw std::runtime_error("FastExpressionImpl::getValue is not supported");
}

double FastExpressionImpl::computeValue() const {
  throw std::runtime_error("FastExpressionImpl::computeValue is not supported");
}

void FastExpressionImpl::print() const {
  throw std::runtime_error("FastExpressionImpl::print is not supported");
}

std::vector<Term> FastExpressionImpl::getTerms() const {
  std::vector<Term> result;
  result.reserve(terms_.size());
  for (const auto& [variableId, coefficient] : terms_) {
    result.push_back(
        Term{.coefficient = coefficient, .variableId = variableId});
  }
  return result;
}

double FastExpressionImpl::getConstant() const {
  return constant_;
}

} // namespace facebook::algopt::lp
