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

#include "algopt/rebalancer/solver/moves/AsyncSingleMovesMoveType.h"

namespace facebook::rebalancer {

class Problem;

constexpr folly::StringPiece kTripleLoopMoveTypeName = "TRIPLE_LOOP";

class TripleLoopMoveType : public AsyncSingleMovesMoveType {
 public:
  std::string name() const override;

  explicit TripleLoopMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::TripleLoopMoveTypeSpec& config);

  MoveResult exploreFromAllSingleMoves(
      const MovesEvaluator& evaluator,
      entities::ContainerId hot_container,
      entities::ObjectId hotObject,
      entities::ContainerId cold_container,
      MoveStatsAggregator& stats) override;

 public:
  interface::TripleLoopMoveTypeSpec config_;
};

} // namespace facebook::rebalancer
