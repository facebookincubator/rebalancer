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

#include "algopt/rebalancer/materializer/spec_builder/NestedScopeLimitSpecBuilder.h"

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/materializer/utils/LimitWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <folly/container/F14Map.h>

#include <stdexcept>

namespace facebook::rebalancer::materializer {

using namespace entities;

folly::F14FastMap<ScopeItemId, ScopeItemId>
NestedScopeLimitSpecBuilder::getInnerToOuterScopeMapping(
    ScopeId outerScopeId,
    ScopeId innerScopeId) const {
  auto& outerScope = universe_->getScope(outerScopeId);
  auto& innerScope = universe_->getScope(innerScopeId);
  folly::F14FastMap<ContainerId, ScopeItemId> containerToOuterScopeItem;
  for (auto& scopeItem : outerScope.getScopeItemIds()) {
    for (auto containerId : outerScope.getContainerIds(scopeItem)) {
      containerToOuterScopeItem.emplace(containerId, scopeItem);
    }
  }
  folly::F14FastMap<ScopeItemId, ScopeItemId> innerToOuterScope;
  const ScopeItemFilterWrapper innerScopeFilter(
      *universe_, *spec_.filter(), innerScopeId);
  for (auto innerScopeItem : innerScopeFilter.getScopeItemIds()) {
    std::optional<ScopeItemId> outerScopeItem;
    for (auto containerId : innerScope.getContainerIds(innerScopeItem)) {
      if (auto scopeItemPtr =
              folly::get_ptr(containerToOuterScopeItem, containerId)) {
        if (!outerScopeItem) {
          outerScopeItem = *scopeItemPtr;
        } else if (*outerScopeItem != *scopeItemPtr) {
          // containers of same innerScopeItem correspond to different
          // outerscopeItems
          throw std::runtime_error("inner and outer scopes must be nested");
        }
      } else {
        XLOG(INFO) << fmt::format(
            "Ignoring container {} that exists in inner scope {} but does not exist in outer scope {} ",
            universe_->getEntityName(containerId),
            universe_->getEntityName(innerScopeId),
            universe_->getEntityName(outerScopeId));
      }
    }
    innerToOuterScope.emplace(innerScopeItem, *outerScopeItem);
  }
  return innerToOuterScope;
}

NestedScopeLimitSpecBuilder::NestedScopeLimitSpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    interface::NestedScopeLimitSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<std::vector<ConstraintInfo>>
NestedScopeLimitSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  std::vector<ConstraintInfo> constraints;
  auto innerScopeId = universe_->getScopeId(*spec_.scope());
  const ScopeItemFilterWrapper innerScopeFilter(
      *universe_, *spec_.filter(), innerScopeId);
  auto outerScopeId = universe_->getScopeId(*spec_.outerScope());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());

  auto innerToOuterScope =
      getInnerToOuterScopeMapping(outerScopeId, innerScopeId);
  const LimitWrapper limit(universe_, *spec_.limit(), innerScopeId);
  for (auto& innerScopeItemId : innerScopeFilter.getScopeItemIds()) {
    auto innerUtil = co_await expressionBuilder.getAbsoluteUtil(
        UtilMetric::AFTER, dimensionId, innerScopeId, innerScopeItemId);
    if (auto outerScopeItemPtr =
            folly::get_ptr(innerToOuterScope, innerScopeItemId)) {
      auto outerUtil = co_await expressionBuilder.getAbsoluteUtil(
          UtilMetric::AFTER, dimensionId, outerScopeId, *outerScopeItemPtr);
      constraints.emplace_back(
          innerUtil - limit.getLimit(innerScopeItemId) * outerUtil);
    } else {
      throw std::runtime_error(
          fmt::format(
              "Unable to map item {} of inner scope {} to an item of outer scope {}",
              universe_->getEntityName(innerScopeItemId),
              universe_->getEntityName(innerScopeId),
              universe_->getEntityName(outerScopeId)));
    }
  }
  co_return constraints;
}

folly::coro::Task<ExprPtr> NestedScopeLimitSpecBuilder::goalCoro(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("NestedScopeLimit not supported as goal");
}

std::string NestedScopeLimitSpecBuilder::description() const {
  return fmt::format(
      "util of scope {}'s items <= {} * util of outer scope {}'s item under dim {}",
      *spec_.scope(),
      *spec_.limit()->globalLimit(),
      *spec_.outerScope(),
      *spec_.dimension());
}

SpecParameters NestedScopeLimitSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .dimension = *spec_.dimension(),
      .limitType = apache::thrift::util::enumNameSafe(*spec_.limit()->type()),
      .filterAllowListSize = spec_.filter()->itemsWhitelist()
          ? static_cast<int>(spec_.filter()->itemsWhitelist()->size())
          : 0,
      .filterBlockListSize = spec_.filter()->itemsBlacklist()
          ? static_cast<int>(spec_.filter()->itemsBlacklist()->size())
          : 0};
}

} // namespace facebook::rebalancer::materializer
