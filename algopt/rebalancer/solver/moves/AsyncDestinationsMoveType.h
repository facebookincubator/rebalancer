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
 * will asynchronously calculate each (hot container, cold container)
 * combination at a time
 */
class AsyncDestinationsMoveType : public MoveType {
 public:
  explicit AsyncDestinationsMoveType(
      const interface::LocalSearchSolverSpec& configs)
      : MoveType(configs) {}
  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

  virtual MoveResult exploreFromAllDestinations(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      entities::ContainerId coldContainer,
      MoveStatsAggregator& stats) = 0;
};
} // namespace facebook::rebalancer
