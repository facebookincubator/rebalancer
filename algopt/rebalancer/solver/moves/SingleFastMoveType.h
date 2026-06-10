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

constexpr folly::StringPiece kSingleFastMoveTypeName = "SINGLE_FAST";

// Find a single move that decreases the objective, not necessarily the best.
// Unlike SINGLE, SINGLE_FAST doesn't explore moving every possible object from
// the hot container. It picks an arbitrary object from the hot container and
// evaluates all possible destinations. If none of such moves improve the
// objective, then the process is repeated with a different object from the hot
// container, until an improvement is found, or all objects are exhausted.
class SingleFastMoveType : public MoveType {
 public:
  std::string name() const override;

  explicit SingleFastMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::SingleFastMoveTypeSpec& spec);

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

 private:
  PackerSet<entities::ContainerId> getDestinations(
      entities::ObjectId hotObject,
      entities::ContainerId hotContainer,
      Problem& problem,
      interface::DestinationsToExploreOptions& destinationsToExplore);

  interface::SingleFastMoveTypeSpec spec_;
};

} // namespace facebook::rebalancer
