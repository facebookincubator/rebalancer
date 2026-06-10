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

#include "algopt/rebalancer/solver/moves/GreedyGroupToScopeItemMoveType.h"

#include "algopt/rebalancer/solver/moves/Move.h"
#include "algopt/rebalancer/solver/moves/MoveHelper.h"
#include "algopt/rebalancer/solver/moves/MovesEvaluator.h"

#include <folly/container/irange.h>

namespace facebook::rebalancer {

namespace {
MoveSet generateRandomSampleAndMoveSet(
    std::vector<entities::ContainerId>& allContainers,
    const std::vector<entities::ObjectId>& groupObjectIds,
    const Problem& problem) {
  auto rng = std::default_random_engine{std::random_device{}()};
  std::shuffle(allContainers.begin(), allContainers.end(), rng);

  const int sampleSize = groupObjectIds.size();
  MoveSet candidateMoveSet;
  for (const auto i : folly::irange(sampleSize)) {
    auto hotObject = groupObjectIds.at(i);
    auto sourceContainer = problem.assignment.getContainer(hotObject);
    auto destinationContainer = allContainers.at(i);

    candidateMoveSet.insert(
        Move(hotObject, sourceContainer, destinationContainer));
  }

  return candidateMoveSet;
}
} // namespace

std::string GreedyGroupToScopeItemMoveType::name() const {
  return kGreedyGroupToScopeItemMoveTypeName.str();
}

GreedyGroupToScopeItemMoveType::GreedyGroupToScopeItemMoveType(
    const interface::LocalSearchSolverSpec& solverConfigs,
    const interface::GreedyGroupToScopeItemMoveTypeSpec& spec)
    : MoveType(solverConfigs) {
  if (spec.groupMovesPartition()->empty() ||
      spec.scopeItemMovesScope()->empty()) {
    throw std::runtime_error(
        "GreedyGroupToScopeItemMoveType requires the parameter 'groupMovesPartition' and 'scopeItemMovesScope' to be set");
  }
  partitionName_ = *spec.groupMovesPartition();
  scopeName_ = *spec.scopeItemMovesScope();
  nSampleSetsToExplore_ = *spec.nSampleSetsToExplore();
}

MoveResult GreedyGroupToScopeItemMoveType::exploreMovingGroup(
    const MovesEvaluator& evaluator,
    const std::vector<entities::ObjectId>& groupObjectIds,
    const std::vector<entities::ScopeItemId>& allScopeItemIds,
    MoveStatsAggregator& stats,
    double timeLimit) const {
  const std::function<MoveResult(entities::ScopeItemId)>
      sampleContainersAndEvaluate =
          [&](entities::ScopeItemId destinationScopeItemId) {
            const auto& problem = evaluator.getProblem();
            const auto& universe = problem.getUniverse();
            const auto scopeId = universe.getScopeId(scopeName_);
            const auto& scope = universe.getScope(scopeId);
            const auto containerIdsPtr =
                scope.getContainerIdsPtr(destinationScopeItemId);
            auto containerIds = std::vector<entities::ContainerId>(
                containerIdsPtr->begin(), containerIdsPtr->end());
            if (containerIds.size() < groupObjectIds.size()) {
              // This moveType tries to move each object
              // in the group to a separate container.
              // Therefore, a candidateScopeItem should
              // have at least as many containers as the
              // objects in the group
              return MoveResult::makeEmpty();
            }

            MoveResult bestResult = MoveResult::makeEmpty();
            int nSampleSetsConsidered = 0;
            while (nSampleSetsConsidered < nSampleSetsToExplore_) {
              ++nSampleSetsConsidered;

              auto candidateMoveSet = generateRandomSampleAndMoveSet(
                  containerIds, groupObjectIds, problem);
              auto result = evaluator.evaluate(std::move(candidateMoveSet));
              stats.add(result);

              bestResult.aggregate(std::move(result));
            }
            return bestResult;
          };

  return MoveHelper::findBest(
      evaluator.getProblem().configs.threadPool.get(),
      allScopeItemIds,
      sampleContainersAndEvaluate,
      timeLimit,
      getParallelExecutionConfig());
}

MoveResult GreedyGroupToScopeItemMoveType::findBestMove(
    const MovesEvaluator& evaluator,
    entities::ContainerId hotContainer,
    MoveStatsAggregator& stats,
    const SearchHints& /* hints */,
    double timeLimit) {
  const algopt::Timer timer(true);

  auto& problem = evaluator.getProblem();
  const auto& universe = problem.getUniverse();
  const auto scopeId = universe.getScopeId(scopeName_);
  const auto& scope = universe.getScope(scopeId);

  // Do NOT deduplicate using equivalent sets because this moveType is
  // essentially impossing a constraint that all objects in a group must move
  // together. So it is possible that someone might use this moveType in lieu of
  // such a constraint. Therefore, in the absence of such a constraint it is
  // possible that a set of objects is deemed equivalent even though they are
  // not all part of the same group. (See
  // GreedyGroupToScopeItemMoveTypeTest.Basic where this case exactly happens.)
  auto& hotObjectIds = evaluator.getDynamicObjects(hotContainer);

  auto bestMoves = MoveResult::makeEmpty();
  PackerSet<entities::GroupId> seenGroupIds;
  for (auto hotObjectId : hotObjectIds) {
    if (timer.getSeconds() >= timeLimit) {
      stats.incrNumTimeouts(bestMoves);
      return bestMoves;
    }

    auto groupIdOpt =
        problem.getOnlyGroupIdIfExists(partitionName_, hotObjectId);
    if (!groupIdOpt.has_value()) {
      continue;
    }

    auto groupId = groupIdOpt.value();
    if (seenGroupIds.contains(groupId)) {
      continue;
    }
    seenGroupIds.insert(groupId);
    auto& groupObjectIds =
        problem.getObjectIdsForGroup(partitionName_, groupId);

    auto bestGroupMove = exploreMovingGroup(
        evaluator, groupObjectIds, scope.getScopeItemIds(), stats, timeLimit);

    bestMoves.aggregate(std::move(bestGroupMove));

    // return the first set of moves that improve the objective
    if (bestMoves.isBetter(universe.getPrecision())) {
      return bestMoves;
    }
  }
  return bestMoves;
}

} // namespace facebook::rebalancer
