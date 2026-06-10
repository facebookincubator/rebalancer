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

#include "algopt/rebalancer/materializer/spec_builder/MaximizeAllocationSpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

MaximizeAllocationSpecBuilder::MaximizeAllocationSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::MaximizeAllocationSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> MaximizeAllocationSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());

  auto result = const_expr(0, universe_);
  const double objectCount = universe_->getNumObjects();
  const double coef = (-1.0 / objectCount);

  const ScopeItemFilterWrapper filter(*universe_, *spec_.filter(), scopeId);
  auto scopeItemIds = filter.getScopeItemIds();
  for (auto scopeItemId : scopeItemIds) {
    result +=
        (coef *
         co_await expressionBuilder.getRelativeUtil(
             UtilMetric::AFTER, dimensionId, scopeId, scopeItemId));
  }
  co_return result;
}

folly::coro::Task<std::vector<ConstraintInfo>>
MaximizeAllocationSpecBuilder::constraints(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error(
      "MaximizeAllocationSpec not supported as a constraint");
}

std::string MaximizeAllocationSpecBuilder::description() const {
  return fmt::format(
      "For {}, maximize allocated {} across {}s",
      *spec_.dimension(),
      universe_->getObjectTypeName(),
      *spec_.scope());
}

SpecParameters MaximizeAllocationSpecBuilder::getSpecInfo() const {
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
