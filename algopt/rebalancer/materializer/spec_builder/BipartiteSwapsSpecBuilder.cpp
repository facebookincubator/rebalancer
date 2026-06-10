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

#include "algopt/rebalancer/materializer/spec_builder/BipartiteSwapsSpecBuilder.h"

#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/Set.h"
#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

BipartiteSwapsSpecBuilder::BipartiteSwapsSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::BipartiteSwapsSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> BipartiteSwapsSpecBuilder::goalCoro(
    ExpressionBuilder& /*expressionBuilder*/) const {
  throw std::runtime_error("not supported as a goal");
}

folly::coro::Task<std::vector<ConstraintInfo>>
BipartiteSwapsSpecBuilder::constraints(
    ExpressionBuilder& /*expressionBuilder*/) const {
  Set<entities::ContainerId> leftSubset;
  Set<entities::ContainerId> rightSubset;
  Map<entities::ObjectId, entities::ContainerId> initialAssignment;

  const std::unordered_set<std::string> subsetContainersSet(
      spec_.subsetContainers()->begin(), spec_.subsetContainers()->end());

  for (auto containerId : universe_->getContainers().getContainerIds()) {
    for (auto objectId :
         universe_->getContainers().getInitialObjectIds(containerId)) {
      initialAssignment.emplace(objectId, containerId);
    }

    const auto& containerName = universe_->getEntityName(containerId);
    if (subsetContainersSet.contains(containerName)) {
      leftSubset.insert(universe_->getContainerId(containerName));
    } else {
      rightSubset.insert(universe_->getContainerId(containerName));
    }
  }

  auto exprs = equals(
      bipartite_swaps(
          std::move(initialAssignment),
          std::move(leftSubset),
          std::move(rightSubset),
          universe_),
      1,
      universe_);
  std::vector<ConstraintInfo> constraintInfos;
  constraintInfos.reserve(exprs.size());
  for (auto& expr : exprs) {
    constraintInfos.emplace_back(std::move(expr));
  }
  co_return constraintInfos;
}

std::string BipartiteSwapsSpecBuilder::description() const {
  return fmt::format(
      "Enforce swaps between left and right side of bipartite graph {}",
      *spec_.name());
}

SpecParameters BipartiteSwapsSpecBuilder::getSpecInfo() const {
  return SpecParameters{.name = *spec_.name()};
}

} // namespace facebook::rebalancer::materializer
