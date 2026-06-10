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

#include <functional>
#include <vector>

namespace facebook::rebalancer::materializer {

class BalanceSpecBuilder : public SpecBuilder {
 public:
  BalanceSpecBuilder(
      std::shared_ptr<const entities::Universe> universe,
      interface::BalanceSpec spec,
      bool continuousExpressions);

  folly::coro::Task<ExprPtr> goalCoro(
      ExpressionBuilder& expressionBuilder) const override;

  folly::coro::Task<std::vector<ConstraintInfo>> constraints(
      ExpressionBuilder& expressionBuilder) const override;

  std::string description() const override;

  SpecParameters getSpecInfo() const override;

 private:
  folly::coro::Task<std::pair<ExprPtr, std::size_t>>
  getTotalAbsoluteOrRelativeUtil(
      UtilMetric metric,
      const std::vector<entities::ScopeItemId>& scopeItemIds,
      ExpressionBuilder& expressionBuilder,
      bool computeRelativeUtil) const;

  bool shouldFixAverageToInitial(
      const std::vector<entities::ScopeItemId>& scopeItemIds) const;

  ExprPtr computeMaxPenalty(
      const std::vector<ExprPtr>& allUtils,
      const ExprPtr& thresholdExpr) const;

  ExprPtr computeLinearOrSquaresPenalty(
      const std::vector<ExprPtr>& allUtils,
      const ExprPtr& thresholdExpr,
      interface::BalanceSpecFormula formula) const;

  ExprPtr computeIdealPenalty(
      const std::vector<ExprPtr>& allUtils,
      const std::vector<double>& adjustments,
      const std::function<ExprPtr(double)>& boundExpr,
      double upperBound,
      bool applyBound) const;

  ExprPtr computeVariancePenalty(
      const std::vector<ExprPtr>& allUtils,
      const std::function<ExprPtr(double)>& boundExpr,
      double upperBound) const;

  ExprPtr computeLegacyPenalty(
      const std::vector<ExprPtr>& allUtils,
      double initialUtil,
      double sumCapacity,
      double upperBound) const;

 private:
  interface::BalanceSpec spec_;
  entities::DimensionId dimensionId_;
  entities::ScopeId scopeId_;
  bool continuousExpressions_;
};

} // namespace facebook::rebalancer::materializer
