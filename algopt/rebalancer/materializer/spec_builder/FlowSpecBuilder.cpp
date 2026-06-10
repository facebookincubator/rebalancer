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

#include "algopt/rebalancer/materializer/spec_builder/FlowSpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/materializer/utils/LimitWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

namespace facebook::rebalancer::materializer {

FlowSpecBuilder::FlowSpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    facebook::rebalancer::interface::FlowSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> FlowSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  co_return getAggregatedConstraintViolation(
      co_await constraints(expressionBuilder), universe_);
}

folly::coro::Task<std::vector<ConstraintInfo>> FlowSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  std::vector<ConstraintInfo> result;

  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());

  const LimitWrapper limits(universe_, *spec_.limit(), scopeId);
  const LimitWrapper coefficients(universe_, *spec_.coefficients(), scopeId);
  auto& objectDimension =
      universe_->getObjects().getDimension(dimensionId).at(0);

  auto bound = *spec_.bound();
  auto& pairs = *spec_.pairs();
  auto& destinationFilters = *spec_.destinationFilter();

  const interface::Filter defaultFilter;
  const ScopeItemFilterWrapper sourceFilter(
      *universe_, *spec_.sourceFilter(), scopeId);

  for (auto sourceItemId : sourceFilter.getScopeItemIds()) {
    auto& sourceItemName = universe_->getEntityName(sourceItemId);
    auto& filter = folly::get_ref_default(
        destinationFilters, sourceItemName, defaultFilter);
    const ScopeItemFilterWrapper destinationFilter(*universe_, filter, scopeId);

    for (auto destinationItemId : destinationFilter.getScopeItemIds()) {
      if (sourceItemId == destinationItemId) {
        continue;
      }

      ExprPtr expr;
      for (auto& pair : pairs) {
        auto objectId1 = universe_->getObjectId(*pair.object1());
        auto objectId2 = universe_->getObjectId(*pair.object2());
        const double weight = objectDimension.getValue(objectId1);
        auto assigned1 =
            expressionBuilder.isAssigned(scopeId, sourceItemId, objectId1);
        auto assigned2 =
            expressionBuilder.isAssigned(scopeId, destinationItemId, objectId2);
        expr += weight * binary_min(assigned1, assigned2, universe_);
      }

      auto& destinationItemName = universe_->getEntityName(destinationItemId);
      double limit =
          getLimit(*spec_.limit(), sourceItemName, destinationItemName);

      if (bound == interface::FlowSpecBound::UPPER) {
        expr -= limit;
      } else if (bound == interface::FlowSpecBound::LOWER) {
        expr = limit - expr;
      } else {
        throw std::runtime_error("unknown bound");
      }

      double coefficient =
          getLimit(*spec_.coefficients(), sourceItemName, destinationItemName);
      expr *= coefficient;

      expr->description = fmt::format(
          "{} * (flow from {} to {} {} {})",
          coefficient,
          sourceItemName,
          destinationItemName,
          bound == interface::FlowSpecBound::UPPER ? "<=" : ">=",
          limit);
      result.emplace_back(expr);
    }
  }

  co_return result;
}

std::string FlowSpecBuilder::description() const {
  return fmt::format(
      "limit flow capacity ({}) on scope {}",
      *spec_.dimension(),
      *spec_.scope());
}

double FlowSpecBuilder::getLimit(
    const interface::Limit& spec,
    const std::string& sourceItemName,
    const std::string& destinationItemName) {
  if (auto limits =
          folly::get_ptr(*spec.scopeItemToGroupLimits(), sourceItemName)) {
    if (auto limit = folly::get_ptr(*limits, destinationItemName)) {
      return *limit;
    }
  }
  if (auto limit = folly::get_ptr(*spec.scopeItemLimits(), sourceItemName)) {
    return *limit;
  }
  return folly::get_default(
      *spec.groupLimits(), destinationItemName, *spec.globalLimit());
}

SpecParameters FlowSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .dimension = *spec_.dimension(),
      .boundType = apache::thrift::util::enumNameSafe(*spec_.bound()),
      .limitType = apache::thrift::util::enumNameSafe(*spec_.limit()->type())};
}

} // namespace facebook::rebalancer::materializer
