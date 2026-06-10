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

#pragma once

#include "algopt/lp/detail/generic/impl/GenericExpressionImpl.h"
#include "algopt/lp/generic/Relation.h"
#include "algopt/lp/generic/Variable.h"

#include <folly/container/F14Map.h>

namespace facebook::algopt::lp::detail {

class GenericExpressionImpl;

class GenericRelationImpl : public RelationImpl {
 public:
  explicit GenericRelationImpl(
      std::shared_ptr<GenericExpressionImpl> expr,
      Relation::ConstantType type);

  std::shared_ptr<GenericExpressionImpl> getExpr() const;
  std::shared_ptr<GenericRelationImpl> clone() const;

  bool isEqZero() const;
  bool isGeqZero() const;
  bool isLeqZero() const;

  std::string digest(bool showValues = false) const;

  /** realize this relation using variable mapping in @param oldVarsToNew */
  Relation realize(
      const folly::F14FastMap<ConstGenericVarPtr, Variable>& oldVarsToNew)
      const;

  Relation image(
      const folly::F14FastSet<PartitionId>& allowedPartitions,
      RealizationRule realizationRule) const;

 private:
  std::shared_ptr<GenericExpressionImpl> expr_;
  Relation::ConstantType type_;
};

} // namespace facebook::algopt::lp::detail
