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

constexpr folly::StringPiece kFixedDestMultiMoveTypeName =
    "FIXED_DEST_MULTIPLE";

// This move types attempts to move MULTIPLE objects from the hot container
// to a pre-specified "special container" such that the objective is improved
class FixedDestMultiMoveType
    : public FixedSrcDstMultiMoveType<interface::FixedDestMultiMoveTypeSpec> {
 public:
  std::string name() const override;

  explicit FixedDestMultiMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::FixedDestMultiMoveTypeSpec& spec)
      : FixedSrcDstMultiMoveType(solverConfigs, spec), spec_(spec) {}

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

 private:
  interface::FixedDestMultiMoveTypeSpec spec_;
};

} // namespace facebook::rebalancer
