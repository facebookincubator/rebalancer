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

#include "algopt/rebalancer/materializer/spec_builder/ObjectAffinitiesSpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

namespace facebook::rebalancer::materializer {

ObjectAffinitiesSpecBuilder::ObjectAffinitiesSpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    facebook::rebalancer::interface::ObjectAffinitiesSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> ObjectAffinitiesSpecBuilder::goalCoro(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("ObjectAffinitiesSpec not supported as a goal");
}

folly::coro::Task<std::vector<ConstraintInfo>>
ObjectAffinitiesSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  std::vector<ConstraintInfo> result;

  auto scopeId = universe_->getScopeId(
      spec_.scope()->empty() ? universe_->getContainerTypeName()
                             : *spec_.scope());
  const ScopeItemFilterWrapper filter(*universe_, *spec_.filter(), scopeId);

  for (auto& affinity : *spec_.affinities()) {
    auto objectId0 = universe_->getObjectId(*affinity.object0());

    // TODO: make this less confusing by not having object1 be a scope item.
    if (isObjectName(*affinity.object1())) {
      auto objectId1 = universe_->getObjectId(*affinity.object1());

      for (auto scopeItemId : filter.getScopeItemIds()) {
        // if is_assigned(object1) == 1 && is_assigned(object0) == 0, then for
        // each objectN: is_assigned(objectN) == 1
        auto assigned0 =
            expressionBuilder.isAssigned(scopeId, scopeItemId, objectId0);
        auto assigned1 =
            expressionBuilder.isAssigned(scopeId, scopeItemId, objectId1);
        auto mustAssignN = assigned1 - assigned0;

        for (const auto& objectN : *affinity.objectsN()) {
          auto objectIdN = universe_->getObjectId(objectN);
          auto assignedN =
              expressionBuilder.isAssigned(scopeId, scopeItemId, objectIdN);
          result.emplace_back(mustAssignN - assignedN);
        }
      }
    } else {
      auto scopeItemId =
          universe_->getScopeItemId(scopeId, *affinity.object1());

      if (filter.isIncluded(scopeItemId)) {
        // if is_assigned(object0) == 0, then for each objectN:
        // is_assigned(objectN) == 1
        auto assigned0 =
            expressionBuilder.isAssigned(scopeId, scopeItemId, objectId0);
        auto mustAssignN = 1 - assigned0;

        for (const auto& objectN : *affinity.objectsN()) {
          auto objectIdN = universe_->getObjectId(objectN);
          auto assignedN =
              expressionBuilder.isAssigned(scopeId, scopeItemId, objectIdN);
          result.emplace_back(mustAssignN - assignedN);
        }
      }
    }
  }

  co_return result;
}

std::string ObjectAffinitiesSpecBuilder::description() const {
  return fmt::format("Object affinity across {}", *spec_.scope());
}

bool ObjectAffinitiesSpecBuilder::isObjectName(
    const std::string& entityName) const {
  for (const auto objectId : universe_->getObjects().getObjectIds()) {
    auto& objectName = universe_->getEntityName(objectId);
    if (objectName == entityName) {
      return true;
    }
  }
  return false;
}

SpecParameters ObjectAffinitiesSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .size = static_cast<int>(spec_.affinities()->size()),
      .filterAllowListSize = spec_.filter()->itemsWhitelist()
          ? static_cast<int>(spec_.filter()->itemsWhitelist()->size())
          : 0,
      .filterBlockListSize = spec_.filter()->itemsBlacklist()
          ? static_cast<int>(spec_.filter()->itemsBlacklist()->size())
          : 0};
}

} // namespace facebook::rebalancer::materializer
