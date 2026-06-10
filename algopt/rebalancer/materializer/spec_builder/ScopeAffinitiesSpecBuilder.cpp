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

#include "algopt/rebalancer/materializer/spec_builder/ScopeAffinitiesSpecBuilder.h"

#include "algopt/rebalancer/solver/expressions/Operators.h"

namespace facebook::rebalancer::materializer {

ScopeAffinitiesSpecBuilder::ScopeAffinitiesSpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    facebook::rebalancer::interface::ScopeAffinitiesSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> ScopeAffinitiesSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());

  auto result = const_expr(0, universe_);
  for (auto& [scopeItemName, affinity] : *spec_.affinities()) {
    auto scopeItemId = universe_->getScopeItemId(scopeId, scopeItemName);
    auto util = co_await expressionBuilder.getRelativeUtil(
        UtilMetric::AFTER, dimensionId, scopeId, scopeItemId);
    result += -1.0 * affinity * util;
  }
  co_return result;
}

folly::coro::Task<std::vector<ConstraintInfo>>
ScopeAffinitiesSpecBuilder::constraints(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("ScopeAffinitiesSpec not supported as a constraint");
}

std::string ScopeAffinitiesSpecBuilder::description() const {
  return fmt::format(
      "Scope {} affinities on {}", *spec_.scope(), *spec_.dimension());
}

SpecParameters ScopeAffinitiesSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .dimension = *spec_.dimension(),
      .size = static_cast<int>(spec_.affinities()->size())};
}

} // namespace facebook::rebalancer::materializer
