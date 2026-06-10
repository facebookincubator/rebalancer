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

#include "algopt/rebalancer/materializer/spec_builder/ExclusiveObjectsSpecBuilder.h"

#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

ExclusiveObjectsSpecBuilder::ExclusiveObjectsSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::ExclusiveObjectsSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> ExclusiveObjectsSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  co_return getAggregatedConstraintViolation(
      co_await constraints(expressionBuilder), universe_);
}

folly::coro::Task<std::vector<ConstraintInfo>>
ExclusiveObjectsSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  std::vector<ConstraintInfo> result;

  auto separate = *spec_.separate();
  auto scopeId = universe_->getScopeId(*spec_.scope());

  const ScopeItemFilterWrapper filter(*universe_, *spec_.filter(), scopeId);
  auto scopeItemIds = filter.getScopeItemIds();

  for (auto& pair : *spec_.pairs()) {
    auto& objectName1 = *pair.object1();
    auto& objectName2 = *pair.object2();

    auto objectId1 = universe_->getObjectId(objectName1);
    auto objectId2 = universe_->getObjectId(objectName2);

    for (auto scopeItemId : scopeItemIds) {
      auto assigned1 =
          expressionBuilder.isAssigned(scopeId, scopeItemId, objectId1);
      auto assigned2 =
          expressionBuilder.isAssigned(scopeId, scopeItemId, objectId2);

      if (separate) {
        result.emplace_back(assigned1 + assigned2 - 1);
      } else {
        result.emplace_back(1 - assigned1 - assigned2);
      }
    }
  }

  co_return result;
}

std::string ExclusiveObjectsSpecBuilder::description() const {
  return fmt::format("Object separation across {}", *spec_.name());
}

SpecParameters ExclusiveObjectsSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .size = static_cast<int>(spec_.pairs()->size()),
      .filterAllowListSize = spec_.filter()->itemsWhitelist()
          ? static_cast<int>(spec_.filter()->itemsWhitelist()->size())
          : 0,
      .filterBlockListSize = spec_.filter()->itemsBlacklist()
          ? static_cast<int>(spec_.filter()->itemsBlacklist()->size())
          : 0};
}

} // namespace facebook::rebalancer::materializer
