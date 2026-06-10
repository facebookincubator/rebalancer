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

#include "algopt/lp/detail/generic/impl/GenericConstraintImpl.h"

#include "algopt/lp/detail/generic/impl/GenericExpressionImpl.h"
#include "algopt/lp/detail/generic/impl/GenericVariableImpl.h"

#include <memory>
#include <optional>

namespace facebook::algopt::lp::detail {

GenericConstraintImpl::GenericConstraintImpl(
    const GenericRelationImpl& relation,
    const std::string& name)
    : name_(name), relation_(relation) {}

std::optional<PartitionId> GenericConstraintImpl::getPartitionId() const {
  return relation_.getExpr()->getPartitionId();
}

bool GenericConstraintImpl::isComplicating() const {
  return getPartitionId() == std::nullopt;
}

void GenericConstraintImpl::setName(std::string name) {
  name_ = std::move(name);
}

std::string GenericConstraintImpl::getName() const {
  return name_;
}

const GenericRelationImpl& GenericConstraintImpl::getRelation() const {
  return relation_;
}

thrift::GenericConstraint GenericConstraintImpl::toThrift(
    const folly::F14FastMap<ConstGenericVarPtr, int64_t>& varToId) const {
  thrift::GenericConstraint ctr;
  ctr.name() = name_;
  if (relation_.isEqZero()) {
    ctr.type() = thrift::ConstraintType::EQUALS_ZERO;
  } else if (relation_.isGeqZero()) {
    ctr.type() = thrift::ConstraintType::GEQ_ZERO;
  } else {
    ctr.type() = thrift::ConstraintType::LEQ_ZERO;
  }
  ctr.expr() = relation_.getExpr()->toThrift(varToId);
  return ctr;
}

std::shared_ptr<const RelationImpl> GenericConstraintImpl::fromThrift(
    const thrift::GenericConstraint& ctr,
    const folly::F14FastMap<int64_t, ConstGenericVarPtr>& varToId) {
  auto expr = GenericExpressionImpl::fromThrift(*ctr.expr(), varToId);
  auto type = *ctr.type();
  auto relation = type == thrift::ConstraintType::EQUALS_ZERO
      ? expr->makeEqualZeroRelation()
      : (type == thrift::ConstraintType::GEQ_ZERO
             ? expr->makeGreaterEqualZeroRelation()
             : expr->makeLessEqualZeroRelation());
  return relation;
}

} // namespace facebook::algopt::lp::detail
