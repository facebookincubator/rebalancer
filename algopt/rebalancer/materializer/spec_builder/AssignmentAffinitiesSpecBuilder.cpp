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

#include "algopt/rebalancer/materializer/spec_builder/AssignmentAffinitiesSpecBuilder.h"

#include "algopt/rebalancer/entities/ObjectStaticDimension.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

AssignmentAffinitiesSpecBuilder::AssignmentAffinitiesSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::AssignmentAffinitiesSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> AssignmentAffinitiesSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  auto scopeId = universe_->getScopeId(
      spec_.scope()->empty() ? universe_->getContainerTypeName()
                             : *spec_.scope());

  Map<ObjectId, Map<ScopeItemId, double>> objectItemAffinity;

  for (auto& affinity : *spec_.affinities()) {
    auto objectId = universe_->getObjectId(*affinity.objectName());
    auto scopeItemId =
        universe_->getScopeItemId(scopeId, *affinity.scopeItemName());
    objectItemAffinity[objectId][scopeItemId] += *affinity.affinity();
  }

  const auto numObjects = universe_->getNumObjects();
  double maxAffinitySum = 0;
  ObjectIdToDoubleMap objectMaxAffinity(
      numObjects,
      /*defaultValue=*/0.0,
      /*expectedNonDefaultSize=*/objectItemAffinity.size());

  const auto& scopeItemIds = universe_->getScope(scopeId).getScopeItemIds();
  Map<ScopeItemId, ObjectIdToDoubleMap> itemObjectPenalty;
  itemObjectPenalty.reserve(
      static_cast<decltype(itemObjectPenalty)::size_type>(scopeItemIds.size()));
  for (auto& [objectId, itemAffinity] : objectItemAffinity) {
    double maxAffinity = 0;
    for (auto& [_, affinity] : itemAffinity) {
      maxAffinity = std::max(maxAffinity, affinity);
    }

    maxAffinitySum += maxAffinity;

    for (auto scopeItemId : scopeItemIds) {
      const double affinity = folly::get_default(itemAffinity, scopeItemId, 0);
      auto [it, _] = itemObjectPenalty.try_emplace(
          scopeItemId,
          numObjects,
          /*defaultValue=*/0.0,
          /*expectedNonDefaultSize=*/objectItemAffinity.size());
      it->second.emplace(objectId, maxAffinity - affinity);
    }

    objectMaxAffinity.emplace(objectId, maxAffinity);
  }

  // Choice of implementation: assignment affinities are implemented as a
  // penalty contributed to the formula by objects assigned to sub-optimal
  // containers. With this implementation, containers with unhappy objects
  // will have a non-zero potential (value minus lower bound), and the
  // local search heuristic to select source containers will pick them first.
  auto result = const_expr(0, universe_);
  for (auto& [scopeItemId, objectPenalty] : itemObjectPenalty) {
    const ObjectStaticDimension objectDimension(std::move(objectPenalty));
    result += co_await expressionBuilder.getAbsoluteUtil(
        UtilMetric::AFTER, objectDimension, scopeId, scopeItemId);
  }

  // Objects have an affinity of zero to being placed outside of the scope.
  // Following the formula above (penalty = maxAffinity - affinity), the
  // penalty paid by objects outside of the scope is maxAffinity.
  const ObjectStaticDimension outOfScopeObjectPenalties(
      std::move(objectMaxAffinity));
  auto outOfScopePenalty = expressionBuilder.getAbsoluteUtilOutOfScope(
      UtilMetric::AFTER, outOfScopeObjectPenalties, scopeId);
  result += std::move(outOfScopePenalty);

  // Legacy constant adjustment.
  result -= maxAffinitySum;
  co_return result;
}

folly::coro::Task<std::vector<ConstraintInfo>>
AssignmentAffinitiesSpecBuilder::constraints(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("not supported as a constraint");
}

std::string AssignmentAffinitiesSpecBuilder::description() const {
  return fmt::format(
      "Assignment affinities of {} to {}",
      universe_->getObjectTypeName(),
      *spec_.scope());
}

SpecParameters AssignmentAffinitiesSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .size = static_cast<int>(spec_.affinities()->size())};
}

} // namespace facebook::rebalancer::materializer
