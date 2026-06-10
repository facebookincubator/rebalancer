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

#include "algopt/rebalancer/solver/moves/SwapFullWithEmptyContainersMoveType.h"

#include "algopt/rebalancer/solver/moves/MovesEvaluator.h"

namespace facebook::rebalancer {

SwapFullWithEmptyContainersMoveType::SwapFullWithEmptyContainersMoveType(
    const interface::LocalSearchSolverSpec& solverConfigs,
    const interface::SwapFullWithEmptyContainersMoveTypeSpec& spec)
    : SwapFullContainersMoveType(
          solverConfigs,
          interface::SwapFullContainersMoveTypeSpec()),
      spec_(spec) {}

std::string SwapFullWithEmptyContainersMoveType::name() const {
  return kSwapFullWithEmptyContainersTypeName.str();
}

MoveResult SwapFullWithEmptyContainersMoveType::exploreFromAllDestinations(
    const MovesEvaluator& evaluator,
    entities::ContainerId hot_container,
    entities::ContainerId cold_container,
    MoveStatsAggregator& stats) {
  if (!SwapFullContainersMoveType::isContainerEmpty(evaluator, hot_container) &&
      !SwapFullContainersMoveType::isContainerEmpty(
          evaluator, cold_container)) {
    return MoveResult::makeEmpty();
  }

  return SwapFullContainersMoveType::exploreFromAllDestinations(
      evaluator, hot_container, cold_container, stats);
}

} // namespace facebook::rebalancer
