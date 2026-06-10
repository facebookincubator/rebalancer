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

#include "algopt/rebalancer/algopt_common/Cache.h"
#include "algopt/rebalancer/materializer/spec_builder/SpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/LimitWrapper.h"

namespace facebook::rebalancer::materializer {
/**
Given a scope S, a partition P, and a dimension D, DiversifyWithinScopeItemSpec
is used to spread the utilization of each group G in P within each scope item
I in S if the utilization of G w.r.t. I is greater than a specified threshold
(specified using 'DiversifyWithinScopeItemSpec.limits').

Note that here, unlike usual Balance-like specs, the goal is not to spread
across the scope items in S, but for each I in S is to spread across the
containers in I if the utilization of G w.r.t. I is greater than some threshold.

*/
class DiversifyWithinScopeItemSpec : public SpecBuilder {
 public:
  DiversifyWithinScopeItemSpec(
      std::shared_ptr<const entities::Universe> universe,
      interface::DiversifyWithinScopeItemSpec spec);

  folly::coro::Task<ExprPtr> goalCoro(
      ExpressionBuilder& expressionBuilder) const override;

  folly::coro::Task<std::vector<ConstraintInfo>> constraints(
      ExpressionBuilder& expressionBuilder) const override;

  std::string description() const override;

  SpecParameters getSpecInfo() const override;

 private:
  folly::coro::Task<ExprPtr> getObjectiveExpr(
      ExpressionBuilder& expressionBuilder,
      entities::GroupId groupId,
      const std::vector<entities::ScopeItemId>& scopeItemsIds) const;

  folly::coro::Task<ExprPtr> getDiversificationExpr(
      ExpressionBuilder& expressionBuilder,
      entities::GroupId groupId,
      entities::ScopeItemId scopeItemId) const;

  folly::coro::Task<ExprPtr> getSpreadingFormula(
      ExpressionBuilder& expressionBuilder,
      entities::GroupId groupId,
      entities::ScopeItemId scopeItemId) const;

  void throwIfZeroScopeDimensionValuesExist(
      const std::vector<entities::ScopeItemId>& scopeItemIds) const;

 private:
  interface::DiversifyWithinScopeItemSpec spec_;
  entities::DimensionId dimensionId_;
  entities::ScopeId scopeId_;
  entities::PartitionId partitionId_;
  entities::ScopeId containerScopeId_;
  const entities::Scope& scope_;
  const entities::Scope& containerScope_;
  const entities::Partition& partition_;
  const entities::ScopeDimension& scopeDimension_;
  LimitWrapper groupToLimit_;
};

} // namespace facebook::rebalancer::materializer
