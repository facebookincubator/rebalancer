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

class LargeShardSpecBuilder : public SpecBuilder {
 public:
  LargeShardSpecBuilder(
      std::shared_ptr<const entities::Universe> universe,
      interface::LargeShardSpec spec);

  folly::coro::Task<ExprPtr> goalCoro(
      ExpressionBuilder& expressionBuilder) const override;

  folly::coro::Task<std::vector<ConstraintInfo>> constraints(
      ExpressionBuilder& expressionBuilder) const override;

  std::string description() const override;

  SpecParameters getSpecInfo() const override;

 private:
  interface::LargeShardSpec spec_;
  const entities::ScopeId scopeId_;
  const entities::DimensionId dimensionId_;
  const entities::ScopeItemId unassignedScopeItemId_;
  entities::Set<entities::ObjectId> immovableShards_;
  entities::Set<entities::ScopeItemId> nonAcceptingScopeItemIds_;

  using ObjectIdValue = typename std::pair<entities::ObjectId, double>;
  using RawDrainMetric = std::pair<double, int>;
  using DrainMetric = std::pair<int, int>;

  folly::coro::Task<std::optional<entities::ScopeItemId>>
  findCandidateScopeItemToDrain(
      ExpressionBuilder& expressionBuilder,
      const std::vector<entities::ScopeItemId>& assignableScopeItemIds,
      double candidateLargeShardSize) const;

  folly::coro::Task<entities::Map<entities::ScopeItemId, DrainMetric>>
  computeDrainMetricForScopeItems(
      ExpressionBuilder& expressionBuilder,
      const std::vector<entities::ScopeItemId>& assignableScopeItemIds,
      double candidateLargeShardSize) const;

  folly::coro::Task<std::optional<RawDrainMetric>> computeRawDrainMetric(
      ExpressionBuilder& expressionBuilder,
      const std::vector<ObjectIdValue>& objectDimValues,
      double candidateLargeShardSize,
      double containerCapacity,
      entities::ScopeItemId scopeItemId) const;

  void collectImmovableShardsAndNonAcceptingScopeItems();

  std::optional<ObjectIdValue> findLargestMovableShardIn(
      entities::ScopeItemId scopeItemId) const;

  std::vector<ObjectIdValue> getSortedDimValuesOfInitialObjectsIn(
      entities::ScopeItemId scopeItemId) const;

  double getObjectDimValue(
      entities::ObjectId objectId,
      entities::ScopeItemId scopeItemId) const;
};

} // namespace facebook::rebalancer::materializer
