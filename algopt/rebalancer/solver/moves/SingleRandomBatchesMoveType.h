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

constexpr folly::StringPiece kSingleRandomBatchesMoveTypeName =
    "SINGLE_RANDOM_BATCHES";

/*
 * Find any single move that decreases the objective using this strategy:
 * 1. Pick an arbitrary object from the given hot container.
 * 2. Evaluate a random sample of destination containers, pick the best.
 * 3. If the best container found improves the objective score, we are done.
 * 4. If we haven't tried all containers, go to step 2.
 * 5. If we haven't tried all hot objects, go to step 1.
 * 6. No move can be found.
 */
class SingleRandomBatchesMoveType : public MoveType {
 public:
  std::string name() const override;

  explicit SingleRandomBatchesMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::SingleRandomBatchesMoveTypeSpec& spec);

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

 private:
  interface::SingleRandomBatchesMoveTypeSpec spec_;
};
} // namespace facebook::rebalancer
