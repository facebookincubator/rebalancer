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

#include "algopt/rebalancer/solver/moves/SwapFullContainersMoveType.h"

namespace facebook::rebalancer {

class Problem;

constexpr folly::StringPiece kSwapFullWithEmptyContainersTypeName =
    "SWAP_FULL_WITH_EMPTY_CONTAINERS";

class SwapFullWithEmptyContainersMoveType : public SwapFullContainersMoveType {
 public:
  std::string name() const override;

  explicit SwapFullWithEmptyContainersMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::SwapFullWithEmptyContainersMoveTypeSpec& spec);
  MoveResult exploreFromAllDestinations(
      const MovesEvaluator& evaluator,
      entities::ContainerId hot_container,
      entities::ContainerId cold_container,
      MoveStatsAggregator& stats) override;

 private:
  interface::SwapFullWithEmptyContainersMoveTypeSpec spec_;
};
} // namespace facebook::rebalancer
