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

#include "algopt/rebalancer/materializer/spec_builder/ExclusiveSwapsSpecBuilder.h"

#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <stdexcept>

namespace facebook::rebalancer::materializer {

namespace {

Swaps::SubsetDefinition translate(
    interface::ExclusiveSwapsSpecSubsetDefinition definition) {
  switch (definition) {
    case interface::ExclusiveSwapsSpecSubsetDefinition::AT_LEAST_ONE_IN_SUBSET:
      return Swaps::SubsetDefinition::AT_LEAST_ONE_IN_SUBSET;
    case interface::ExclusiveSwapsSpecSubsetDefinition::EXACTLY_ONE_IN_SUBSET:
      return Swaps::SubsetDefinition::EXACTLY_ONE_IN_SUBSET;
    case interface::ExclusiveSwapsSpecSubsetDefinition::
        BOTH_SAME_SIDE_OF_SUBSET:
      return Swaps::SubsetDefinition::BOTH_SAME_SIDE_OF_SUBSET;
    default:
      throw std::runtime_error("Unhandled ExclusiveSwapsSpecSubsetDefinition");
  }
}

std::string toString(interface::ExclusiveSwapsSpecSubsetDefinition definition) {
  switch (definition) {
    case interface::ExclusiveSwapsSpecSubsetDefinition::AT_LEAST_ONE_IN_SUBSET:
      return "at least one in subset";
    case interface::ExclusiveSwapsSpecSubsetDefinition::EXACTLY_ONE_IN_SUBSET:
      return "exactly one in subset";
    case interface::ExclusiveSwapsSpecSubsetDefinition::
        BOTH_SAME_SIDE_OF_SUBSET:
      return "both same side of subset";
    default:
      throw std::runtime_error("Unhandled ExclusiveSwapsSpecSubsetDefinition");
  }
}

} // namespace

ExclusiveSwapsSpecBuilder::ExclusiveSwapsSpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    facebook::rebalancer::interface::ExclusiveSwapsSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> ExclusiveSwapsSpecBuilder::goalCoro(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("ExclusiveSwapsSpec not supported as a goal");
}

folly::coro::Task<std::vector<ConstraintInfo>>
ExclusiveSwapsSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  entities::Map<entities::ObjectId, entities::ContainerId>
      initialObjectIdToContainerId;
  auto& containers = universe_->getContainers();
  for (auto& containerId : containers.getContainerIds()) {
    auto& initialObjectIds = containers.getInitialObjectIds(containerId);
    for (auto objectId : initialObjectIds) {
      initialObjectIdToContainerId.emplace(objectId, containerId);
    }
  }

  auto toConstraintInfos = [&](std::vector<ExprPtr>&& exprs) {
    std::vector<ConstraintInfo> constraintInfos;
    constraintInfos.reserve(exprs.size());
    for (auto& expr : exprs) {
      constraintInfos.emplace_back(std::move(expr));
    }
    return constraintInfos;
  };

  if (!spec_.subsetObjects()) {
    co_return toConstraintInfos(
        equals(swaps(initialObjectIdToContainerId, universe_), 1, universe_));
  }

  auto& subsetObjectNames = *spec_.subsetObjects();
  auto subsetDefinition = translate(*spec_.subsetDefinition());

  entities::Set<entities::ObjectId> subsetObjectIds;
  for (const auto& objectName : subsetObjectNames) {
    auto objectId = universe_->getObjectId(objectName);
    subsetObjectIds.insert(objectId);
  }

  co_return toConstraintInfos(equals(
      swaps(
          initialObjectIdToContainerId,
          universe_,
          subsetObjectIds,
          subsetDefinition),
      1,
      universe_));
}

std::string ExclusiveSwapsSpecBuilder::description() const {
  return fmt::format(
      "Exclusive swaps ({})",
      spec_.subsetObjects() ? toString(*spec_.subsetDefinition())
                            : "no subset");
}

SpecParameters ExclusiveSwapsSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .definition =
          apache::thrift::util::enumNameSafe(*spec_.subsetDefinition())};
}

} // namespace facebook::rebalancer::materializer
