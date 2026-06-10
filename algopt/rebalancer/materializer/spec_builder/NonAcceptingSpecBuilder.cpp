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

#include "algopt/rebalancer/materializer/spec_builder/NonAcceptingSpecBuilder.h"

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

NonAcceptingSpecBuilder::NonAcceptingSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::NonAcceptingSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> NonAcceptingSpecBuilder::goalCoro(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("NonAcceptingSpec not supported as a goal");
}

folly::coro::Task<std::vector<ConstraintInfo>>
NonAcceptingSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  std::vector<ConstraintInfo> result;

  auto dimensionName = fmt::format("{}_count", universe_->getObjectTypeName());
  auto dimensionId = universe_->getDimensionId(dimensionName);

  auto scopeId = universe_->getScopeId(*spec_.scope());
  for (auto& scopeItemName : *spec_.items()) {
    auto scopeItemId = universe_->getScopeItemId(scopeId, scopeItemName);
    auto newUtil = co_await expressionBuilder.getAbsoluteUtil(
        UtilMetric::NEW, dimensionId, scopeId, scopeItemId);
    result.emplace_back(newUtil);
  }

  co_return result;
}

std::string NonAcceptingSpecBuilder::description() const {
  return fmt::format(
      "Non-accepting {} {}s", spec_.items()->size(), *spec_.scope());
}

entities::Set<entities::ContainerId>
NonAcceptingSpecBuilder::nonAcceptingContainers() const {
  entities::Set<entities::ContainerId> nonAcceptingContainers;
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto& scope = universe_->getScope(scopeId);
  for (auto& scopeItemName : *spec_.items()) {
    auto scopeItemId = universe_->getScopeItemId(scopeId, scopeItemName);
    auto& containerIds = scope.getContainerIds(scopeItemId);
    if (containerIds.size() > 1) {
      // TODO: apply this optimization to arbitrary sized scopeItems. Note that
      // it's not correct to simply mark all containers within a scope item as
      // non-accepting: new objects may not be added to that scope item, but
      // objects are still free to move within containers of the same scope
      // item. (Same reason as for fixedContainers().)
      continue;
    }
    nonAcceptingContainers.insert(containerIds.begin(), containerIds.end());
  }
  return nonAcceptingContainers;
}

SpecParameters NonAcceptingSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .size = static_cast<int>(spec_.items()->size())};
}

} // namespace facebook::rebalancer::materializer
