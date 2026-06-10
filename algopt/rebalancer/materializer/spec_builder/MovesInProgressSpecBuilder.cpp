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

#include "algopt/rebalancer/materializer/spec_builder/MovesInProgressSpecBuilder.h"

#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

MovesInProgressSpecBuilder::MovesInProgressSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::MovesInProgressSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> MovesInProgressSpecBuilder::goalCoro(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("MovesInProgressSpec not supported as a goal");
}

folly::coro::Task<std::vector<ConstraintInfo>>
MovesInProgressSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  ExprPtr validMoves = const_expr(0, universe_);
  auto scopeId = universe_->getScopeId(universe_->getContainerTypeName());
  for (const auto& move : *spec_.moves()) {
    auto objectId = universe_->getObjectId(*move.objName());
    auto scopeItemId = universe_->getScopeItemId(scopeId, *move.toContainer());
    validMoves += expressionBuilder.isAssigned(scopeId, scopeItemId, objectId);
  }

  const int expectedMovesInProgress = static_cast<int>(spec_.moves()->size());
  // if all objects are in correct container then result should be 0
  auto result = expectedMovesInProgress - validMoves;
  co_return std::vector<ConstraintInfo>{ConstraintInfo(result)};
}

std::string MovesInProgressSpecBuilder::description() const {
  return fmt::format("{} objects are being moved", spec_.moves()->size());
}

Map<ObjectId, ContainerId>
MovesInProgressSpecBuilder::getUpdatesInInitialAssignment() const {
  // update initial assignment with movesInProgress
  Map<ObjectId, ContainerId> inProgress;
  for (auto& move : *spec_.moves()) {
    auto objectId = universe_->getObjectId(*move.objName());
    auto containerId = universe_->getContainerId(*move.toContainer());
    inProgress.emplace(objectId, containerId);
  }
  return inProgress;
}

entities::Set<entities::ObjectId> MovesInProgressSpecBuilder::fixedObjects()
    const {
  entities::Set<entities::ObjectId> objectIds;
  for (const auto& move : *spec_.moves()) {
    auto objectId = universe_->getObjectId(*move.objName());
    objectIds.insert(objectId);
  }
  return objectIds;
}

SpecParameters MovesInProgressSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(), .size = static_cast<int>(spec_.moves()->size())};
}

} // namespace facebook::rebalancer::materializer
