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

#include "algopt/rebalancer/materializer/spec_builder/MinimizeSquaresSpecBuilder.h"

#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

MinimizeSquaresSpecBuilder::MinimizeSquaresSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::MinimizeSquaresSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

ApproximationHint MinimizeSquaresSpecBuilder::getApproximationHint() const {
  if (!spec_.upperBound()) {
    return {
        .valid = false, .upper_bound = 0, .lower_bound = 0, .piece_count = 0};
  }

  return {
      .valid = true,
      .upper_bound = *spec_.upperBound(),
      .lower_bound = *spec_.lowerBound(),
      .piece_count = static_cast<size_t>(*spec_.pieceCount())};
}

folly::coro::Task<ExprPtr> MinimizeSquaresSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  ExprPtr goal = const_expr(0, universe_);
  auto hint = getApproximationHint();
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());
  const auto& scopeItemIds = universe_->getScope(scopeId).getScopeItemIds();
  const double coef = 1.0 / (double)scopeItemIds.size();

  const ScopeItemFilterWrapper filter(*universe_, *spec_.filter(), scopeId);
  auto filteredScopeItemIds = filter.getScopeItemIds();

  for (auto scopeItemId : filteredScopeItemIds) {
    auto relUtil = co_await expressionBuilder.getRelativeUtil(
        UtilMetric::AFTER, dimensionId, scopeId, scopeItemId);
    auto after_expr = max({const_expr(0, universe_), relUtil}, universe_);
    goal += (coef * square(after_expr, hint, universe_));
  }
  co_return goal;
}

folly::coro::Task<std::vector<ConstraintInfo>>
MinimizeSquaresSpecBuilder::constraints(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("MinimizeSquaresSpec not supported as a constraint");
}

std::string MinimizeSquaresSpecBuilder::description() const {
  return fmt::format(
      "Minimize sum of squared {} across {}s",
      *spec_.dimension(),
      *spec_.scope());
}

SpecParameters MinimizeSquaresSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .dimension = *spec_.dimension(),
      .filterAllowListSize = spec_.filter()->itemsWhitelist()
          ? static_cast<int>(spec_.filter()->itemsWhitelist()->size())
          : 0,
      .filterBlockListSize = spec_.filter()->itemsBlacklist()
          ? static_cast<int>(spec_.filter()->itemsBlacklist()->size())
          : 0};
}

} // namespace facebook::rebalancer::materializer
