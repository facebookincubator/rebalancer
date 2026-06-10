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

#include "algopt/rebalancer/materializer/spec_builder/SpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/ExpressionBuilder.h"

namespace facebook::rebalancer::materializer {

class DisasterRecoveryCapacitySpecBuilder : public SpecBuilder {
 public:
  DisasterRecoveryCapacitySpecBuilder(
      std::shared_ptr<const entities::Universe> universe,
      facebook::rebalancer::interface::DisasterRecoveryCapacitySpec spec);

  folly::coro::Task<ExprPtr> goalCoro(
      ExpressionBuilder& expressionBuilder) const override;

  folly::coro::Task<std::vector<ConstraintInfo>> constraints(
      ExpressionBuilder& expressionBuilder) const override;

  std::string description() const override;

  SpecParameters getSpecInfo() const override;

 private:
  facebook::rebalancer::interface::DisasterRecoveryCapacitySpec spec_;

  std::vector<entities::Set<entities::ScopeItemId>> sharedDisasterGroups_;

  void setSharedDisasterGroups();

  std::vector<entities::Map<entities::ObjectId, ExprPtr>>
  getObjectsInDisasterGroupExprs(ExpressionBuilder& expressionBuilder) const;

  folly::coro::Task<entities::Map<entities::ScopeItemId, ExprPtr>>
  getTotalDisasterUsageForDimensionIndex(
      int dimensionIndex,
      ExpressionBuilder& expressionBuilder,
      const std::vector<entities::ScopeItemId>& scopeItemIds,
      const std::vector<entities::Map<entities::ObjectId, ExprPtr>>&
          objectsInDisasterGroupExprs) const;

  entities::ObjectIdToDoubleMap getPrimaryDimensionValuesAtDimensionIndex(
      int dimensionIndex,
      std::optional<entities::ScopeItemId> scopeItemId) const;

  folly::coro::Task<entities::Map<entities::ScopeItemId, ExprPtr>>
  getPrimaryUsageAtScopeItems(
      ExpressionBuilder& expressionBuilder,
      int dimensionIndex) const;

  folly::coro::Task<entities::Map<entities::ScopeItemId, ExprPtr>>
  getExcessLoadAtScopeItems(
      ExpressionBuilder& expressionBuilder,
      const entities::Map<entities::ObjectId, ExprPtr>&
          objectsInDisasterGroupExprs,
      int disasterGroupIndex,
      int dimensionIndex) const;

  folly::coro::Task<entities::Map<entities::ScopeItemId, ExprPtr>>
  getMaxDisasterUsageAtScopeItems(
      ExpressionBuilder& expressionBuilder,
      int dimensionIndex,
      const std::vector<entities::Map<entities::ObjectId, ExprPtr>>&
          objectsInDisasterGroupExprs) const;

 private:
  entities::ScopeId scopeId_;
  entities::DimensionId dimensionId_;
  const entities::ObjectDimension& objectDimension_;
};

} // namespace facebook::rebalancer::materializer
