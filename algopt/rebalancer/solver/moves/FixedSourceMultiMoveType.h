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

constexpr folly::StringPiece kFixedSourceMultiMoveTypeName =
    "FIXED_SOURCE_MULTIPLE";

// This move types attempts to move MULTIPLE objects from a pre-specified
// "special container" (fixed source) to the hot container such that the
// objective is improved
class FixedSourceMultiMoveType
    : public FixedSrcDstMultiMoveType<interface::FixedSrcMultiMoveTypeSpec> {
 public:
  std::string name() const override;

  explicit FixedSourceMultiMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::FixedSrcMultiMoveTypeSpec& spec)
      : FixedSrcDstMultiMoveType(solverConfigs, spec), spec_(spec) {}

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

 private:
  interface::FixedSrcMultiMoveTypeSpec spec_;
};
} // namespace facebook::rebalancer
