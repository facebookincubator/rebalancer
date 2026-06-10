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

#include "algopt/rebalancer/solver/moves/FixedSrcDstMultiMoveType.h"
#include "algopt/rebalancer/solver/moves/MoveType.h"

namespace facebook::rebalancer {

class Problem;

constexpr folly::StringPiece kFixedDestSwapMultiMoveTypeName =
    "FIXED_DEST_SWAP_MULTIPLE";

// This move types attempts to swap MULTIPLE objects between the hot container
// and a pre-specified "special container" such that the objective is improved
class FixedDestSwapMultiMoveType : public MoveType {
 public:
  explicit FixedDestSwapMultiMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::FixedDestSwapMultiMoveTypeSpec& spec)
      : MoveType(solverConfigs),
        srcContainerMoveGenerator_(spec),
        dstContainerMoveGenerator_(spec),
        spec_(spec) {}

  std::string name() const override;

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

 private:
  // Specialized implementation for swap ratio enabled case
  MoveResult findBestMoveWithSwapRatio(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      entities::ContainerId dstContainer,
      MoveStatsAggregator& stats,
      double timeLimit);

  // To implement a swap move type, we need to generate move candidates from
  // both src and dst containers. To reuse existing code for move generation
  // and to avoid the need to maintain multiple indices for src and dst
  // containers, we use two separate instances of FixedSrcDstMultiMoveType
  FixedSrcDstMoveCandidateGenerator<interface::FixedDestSwapMultiMoveTypeSpec>
      srcContainerMoveGenerator_;
  FixedSrcDstMoveCandidateGenerator<interface::FixedDestSwapMultiMoveTypeSpec>
      dstContainerMoveGenerator_;
  interface::FixedDestSwapMultiMoveTypeSpec spec_;
};

} // namespace facebook::rebalancer
