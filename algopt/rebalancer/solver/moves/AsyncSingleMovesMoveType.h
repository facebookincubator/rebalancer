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

class Problem;

/*
 * To explore all moves, MoveTypes that inherit from this class
 * will asynchronously calculate each (hot object, cold container)
 * combination at a time
 */
class AsyncSingleMovesMoveType : public MoveType {
 public:
  explicit AsyncSingleMovesMoveType(
      const interface::LocalSearchSolverSpec& configs)
      : MoveType(configs) {}
  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

  virtual MoveResult exploreFromAllSingleMoves(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      entities::ObjectId hotObject,
      entities::ContainerId coldContainer,
      MoveStatsAggregator& stats) = 0;

  // by default, all remaining containers are considered as candidates for
  // coldContainer. Override this function to customize the set of cold
  // containers that we want to be considered
  virtual std::optional<PackerSet<entities::ContainerId>>
  getCustomColdContainers(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer) const;

  // in some cases, we may not want to attempt move with certain objects, for
  // example if the object is fixed or when we only want to explore a random set
  // of moves
  virtual bool attemptMoveWithThisObject(
      const MovesEvaluator& evaluator,
      entities::ObjectId objectId,
      int numObjectsInContainer) const;
};
} // namespace facebook::rebalancer
