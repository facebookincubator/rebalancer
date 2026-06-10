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

constexpr folly::StringPiece kSingleGreedyMoveTypeName = "SINGLE_GREEDY";

// Find the first move to improve the objective. It picks an arbitrary object
// from the hot container, and evaluates moving it to every destination, from
// coldest to hottest, until it finds a move that improves the objective. If
// no improvement is found, it repeats the process with a different object from
// the hot container.
class SingleGreedyMoveType : public MoveType {
 public:
  std::string name() const override;

  explicit SingleGreedyMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::SingleGreedyMoveTypeSpec& spec);

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

  interface::SingleGreedyMoveTypeSpec spec_;
};
} // namespace facebook::rebalancer
