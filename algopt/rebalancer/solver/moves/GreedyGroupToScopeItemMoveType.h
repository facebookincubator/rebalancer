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

#pragma once

#include "algopt/rebalancer/solver/moves/MoveType.h"

namespace facebook::rebalancer {

constexpr folly::StringPiece kGreedyGroupToScopeItemMoveTypeName =
    "GREEDY_GROUP_TO_SCOPE_ITEM";

/*
GreedyGroupToScopeItemMoveType evaluates moving an entire group of objects from
the given partition (as specified in the field 'groupMovesPartition') to a
scopeItem in the given scope (as specified in the field 'scopeItemMovesScope').
Note that in the current implementation, each object in the group is forced to
move into a *unique* container in the chosen scopeItem. This in turn implies
that moves are only explored to scopeItems that have as many containers as
objects in the group being considered.
*/
class GreedyGroupToScopeItemMoveType : public MoveType {
 public:
  std::string name() const override;

  explicit GreedyGroupToScopeItemMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::GreedyGroupToScopeItemMoveTypeSpec& spec);

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

 private:
  MoveResult exploreMovingGroup(
      const MovesEvaluator& evaluator,
      const std::vector<entities::ObjectId>& groupObjectIdsInt,
      const std::vector<entities::ScopeItemId>& allScopeItemIds,
      MoveStatsAggregator& stats,
      double timeLimit) const;

  std::string partitionName_;
  std::string scopeName_;
  int nSampleSetsToExplore_;
};
} // namespace facebook::rebalancer
