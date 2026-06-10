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

#include "algopt/rebalancer/materializer/spec_builder/UtilIncreaseCostSpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

UtilIncreaseCostSpecBuilder::UtilIncreaseCostSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::UtilIncreaseCostSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> UtilIncreaseCostSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());
  const ScopeItemFilterWrapper filter(*universe_, *spec_.filter(), scopeId);
  auto squares = *spec_.squares();
  auto lb = *spec_.lowerBound();

  auto transform = [squares, this](ExprPtr expr) {
    return squares ? power(std::move(expr), 1.1, universe_) : std::move(expr);
  };

  auto goalExpr = const_expr(0, universe_);
  auto remainingScopeItemIds = filter.getScopeItemIds();
  for (auto scopeItemId : remainingScopeItemIds) {
    auto util = co_await expressionBuilder.getRelativeUtil(
        UtilMetric::AFTER, dimensionId, scopeId, scopeItemId);
    auto limit = expressionBuilder.getInitialValue(
        *transform(max({const_expr(lb, universe_), util}, universe_)));
    goalExpr +=
        max({const_expr(0, universe_), transform(util) - limit}, universe_) /
        remainingScopeItemIds.size();
  }
  co_return goalExpr;
}

folly::coro::Task<std::vector<ConstraintInfo>>
UtilIncreaseCostSpecBuilder::constraints(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error(
      "UtilIncreaseCostSpec not supported as a constraint");
}

std::string UtilIncreaseCostSpecBuilder::description() const {
  return fmt::format(
      "Util increase cost over {} on {} of {}, squares: {}",
      *spec_.lowerBound(),
      *spec_.dimension(),
      *spec_.scope(),
      *spec_.squares());
}

SpecParameters UtilIncreaseCostSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .dimension = *spec_.dimension(),
      .squares = *spec_.squares() ? "yes" : "no",
      .filterAllowListSize = spec_.filter()->itemsWhitelist()
          ? static_cast<int>(spec_.filter()->itemsWhitelist()->size())
          : 0,
      .filterBlockListSize = spec_.filter()->itemsBlacklist()
          ? static_cast<int>(spec_.filter()->itemsBlacklist()->size())
          : 0};
}

} // namespace facebook::rebalancer::materializer
