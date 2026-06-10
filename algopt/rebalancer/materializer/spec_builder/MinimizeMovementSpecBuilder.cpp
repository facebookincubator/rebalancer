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

#include "algopt/rebalancer/materializer/spec_builder/MinimizeMovementSpecBuilder.h"

#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

constexpr double kCostMultiplier = 0.001;

MinimizeMovementSpecBuilder::MinimizeMovementSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::MinimizeMovementSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<std::vector<ConstraintInfo>>
MinimizeMovementSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  ExprPtr result = const_expr(0, universe_);
  auto scopeId = universe_->getScopeId(
      spec_.scope()->empty() ? universe_->getContainerTypeName()
                             : *spec_.scope());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());
  const auto& scopeItemIds = universe_->getScope(scopeId).getScopeItemIds();

  double weight = 1;
  if (*spec_.magicScaling()) {
    // weight without weight_multiplier, which is added separately for all
    // goals.
    weight = kCostMultiplier;
  }

  // there are two sources of normalization for minimize movement
  // i)  division by number of items
  // ii) division by capacity of each container (in the given dimension) due to
  // item_expr
  // if the user explicitly asks us not to normalize, we prevent both ways
  if (*spec_.doNotNormalize()) {
    for (auto scopeItemId : scopeItemIds) {
      result += co_await expressionBuilder.getAbsoluteUtil(
          UtilMetric::NEW, dimensionId, scopeId, scopeItemId);
    }
    result += co_await expressionBuilder.getAbsoluteUtilOutOfScope(
        UtilMetric::NEW, dimensionId, scopeId);

    if (*spec_.allowance() != 0) {
      result = max(
          {const_expr(0, universe_), result - *spec_.allowance()}, universe_);
    }
  } else {
    if (!scopeItemIds.empty()) {
      weight /= (double)scopeItemIds.size();
    }

    for (auto scopeItemId : scopeItemIds) {
      result += weight *
          co_await expressionBuilder.getRelativeUtil(
              UtilMetric::NEW, dimensionId, scopeId, scopeItemId);
    }
    result += weight *
        co_await expressionBuilder.getAbsoluteUtilOutOfScope(
            UtilMetric::NEW, dimensionId, scopeId);
  }
  co_return std::vector<ConstraintInfo>{ConstraintInfo(result)};
}

folly::coro::Task<ExprPtr> MinimizeMovementSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto constraintInfo = co_await constraints(expressionBuilder);
  co_return constraintInfo.at(0).constraintExpr;
}

std::string MinimizeMovementSpecBuilder::description() const {
  return fmt::format(
      "Minimize movement on {} for scope {}",
      *spec_.dimension(),
      *spec_.scope());
}

SpecParameters MinimizeMovementSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .dimension = *spec_.dimension()};
}

} // namespace facebook::rebalancer::materializer
