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

#include "algopt/lp/detail/generic/impl/GenericRelationImpl.h"

#include "algopt/lp/detail/generic/impl/GenericExpressionImpl.h"
#include "algopt/lp/generic/Operators.h"

namespace facebook::algopt::lp::detail {

GenericRelationImpl::GenericRelationImpl(
    std::shared_ptr<GenericExpressionImpl> expr,
    Relation::ConstantType type)
    : expr_(expr), type_(type) {}

std::shared_ptr<GenericExpressionImpl> GenericRelationImpl::getExpr() const {
  return expr_;
}
std::shared_ptr<GenericRelationImpl> GenericRelationImpl::clone() const {
  return std::make_shared<GenericRelationImpl>(
      std::dynamic_pointer_cast<GenericExpressionImpl>(expr_->clone()), type_);
}

bool GenericRelationImpl::isEqZero() const {
  return type_ == Relation::ConstantType::EQ_ZERO;
}
bool GenericRelationImpl::isGeqZero() const {
  return type_ == Relation::ConstantType::GE_ZERO;
}
bool GenericRelationImpl::isLeqZero() const {
  return type_ == Relation::ConstantType::LE_ZERO;
}

std::string GenericRelationImpl::digest(bool showValues) const {
  auto desc = isEqZero() ? " == 0 " : (isGeqZero() ? " >= 0 " : " <= 0");
  return expr_->digest(showValues) + desc;
}

Relation GenericRelationImpl::realize(
    const folly::F14FastMap<ConstGenericVarPtr, Variable>& oldVarsToNew) const {
  if (expr_->isConstant()) {
    return Relation(expr_->getConstant(), type_);
  }
  auto newExpr = expr_->realize(oldVarsToNew);
  if (isEqZero()) {
    return newExpr == 0;
  } else if (isGeqZero()) {
    return newExpr >= 0;
  } else {
    return newExpr <= 0;
  }
}

Relation GenericRelationImpl::image(
    const folly::F14FastSet<PartitionId>& allowedPartitions,
    RealizationRule realizationRule) const {
  auto expr = expr_->image(allowedPartitions, realizationRule);
  if (isEqZero()) {
    return expr.makeEqualZeroRelation();
  } else if (isGeqZero()) {
    return expr.makeGreaterEqualZeroRelation();
  } else {
    return expr.makeLessEqualZeroRelation();
  }
}

} // namespace facebook::algopt::lp::detail
