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

class ExclusiveScopeItemsSpecBuilder : public SpecBuilder {
 public:
  ExclusiveScopeItemsSpecBuilder(
      std::shared_ptr<const entities::Universe> universe,
      interface::ExclusiveScopeItemsSpec spec);

  folly::coro::Task<ExprPtr> goalCoro(
      ExpressionBuilder& expressionBuilder) const override;

  folly::coro::Task<std::vector<ConstraintInfo>> constraints(
      ExpressionBuilder& expressionBuilder) const override;

  std::string description() const override;

  SpecParameters getSpecInfo() const override;

 private:
  // This constraint enforces that a pair of scopeItems cannot be utilized
  // concurrently. If an object partition is provided, we enforce this
  // constraint for each group of objects, otherwise we enforce one constraint
  // for all objects
  folly::coro::Task<std::vector<ConstraintInfo>> buildConstraintPerGroup(
      ExpressionBuilder& expressionBuilder,
      entities::PartitionId partitionId) const;

  // This goal attempts to minimize the number of scopeItems that are
  // invalidated by other used scopeItems.
  folly::coro::Task<ExprPtr> getMinimizeInvalidatedScopeItemsCountGoal(
      ExpressionBuilder& expressionBuilder) const;

  // This goal is more aggressive in "packing" scopeItems to maximize the
  // available space in the non-invalidated scope items. It maximizes the score
  // as defined by:
  //
  //     For each scope item S_i, let w_i specify the size of S_i, default 1.
  //     And for each conflicting scope item S_j where S_i conflicts with S_j,
  //     let overlap_i_j be the size of the overlap between the two conflicting
  //     scope items, S_i and S_j, also default 1. Then the amount that each
  //     scope item is "packed", P_i, can be calculated by summing w_i *
  //     step(utilization(S_i)) and overlap_i_j * step(utilization(S_j)) for
  //     each S_j that conflicts with S_i.
  //
  //     Then the objective score can be calculated as:
  //         Sum[w_i * P_i ^ 2] for all i
  folly::coro::Task<ExprPtr> getAggressivePackingGoal(
      ExpressionBuilder& expressionBuilder) const;

  interface::ExclusiveScopeItemsSpec spec_;
  entities::DimensionId dimensionId_;
  entities::ScopeId scopeId_;
};

} // namespace facebook::rebalancer::materializer
