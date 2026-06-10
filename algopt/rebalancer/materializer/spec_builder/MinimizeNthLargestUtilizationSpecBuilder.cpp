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

#include "algopt/rebalancer/materializer/spec_builder/MinimizeNthLargestUtilizationSpecBuilder.h"

#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

namespace facebook::rebalancer::materializer {

MinimizeNthLargestUtilizationSpecBuilder::
    MinimizeNthLargestUtilizationSpecBuilder(
        std::shared_ptr<const entities::Universe> universe,
        facebook::rebalancer::interface::MinimizeNthLargestUtilizationSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<std::vector<ConstraintInfo>>
MinimizeNthLargestUtilizationSpecBuilder::constraints(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error(
      "MinimizeNthLargestUtilizationSpec is not supported as a constraint");
}

folly::coro::Task<ExprPtr> MinimizeNthLargestUtilizationSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto targetUtilization = *spec_.targetUtilization();
  auto n = *spec_.n();

  const ScopeItemFilterWrapper filter(*universe_, *spec_.filter(), scopeId);
  auto filteredScopeItemIds = filter.getScopeItemIds();

  std::vector<ExprPtr> scopeItemRelativeUtils;
  scopeItemRelativeUtils.reserve(filteredScopeItemIds.size());

  auto& scope = universe_->getScope(scopeId);
  auto& dimension = scope.getDimension(dimensionId);

  for (auto scopeItemId : filteredScopeItemIds) {
    if (dimension.getValue(scopeItemId) == 0) {
      continue;
    }
    auto util = co_await expressionBuilder.getRelativeUtil(
        UtilMetric::AFTER, dimensionId, scopeId, scopeItemId);
    scopeItemRelativeUtils.push_back(util);
  }

  if (scopeItemRelativeUtils.empty()) {
    // Degenerate case where there are no scope items with non-zero size,
    // just return constant zero penalty.
    co_return const_expr(0, universe_);
  }

  auto nthLargestUtil = nth_largest(scopeItemRelativeUtils, n, universe_);

  if (targetUtilization > 0) {
    // if 'targetUtilization' > 0, this ensures that there is no incentive to
    // minimize 'nthLargestUtil' below value of 'targetUtilization'
    co_return max(
        {const_expr(0, universe_), nthLargestUtil - targetUtilization},
        universe_);
  }

  co_return nthLargestUtil;
}

/*
Spec description
-----------------
Given a scope `S`, dimension `D`, and a non-negative integer `n`,
MinimizeNthLargestUtilizationSpec aims to minimize the `(n+1)-th` largest
relative utilizations `U_{n+1}` (w.r.t. `D`) of the scopeItems in `S`. If a
target utilization `T` is specified, then it has no incentive to minimize `U_n`
below `T`.
*/
std::string MinimizeNthLargestUtilizationSpecBuilder::description() const {
  auto description = fmt::format(
      "Minimize {}-th largest {} utilization across {} with target utilization {}",
      *spec_.n(),
      *spec_.dimension(),
      *spec_.scope(),
      *spec_.targetUtilization());
  return description;
}

SpecParameters MinimizeNthLargestUtilizationSpecBuilder::getSpecInfo() const {
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
