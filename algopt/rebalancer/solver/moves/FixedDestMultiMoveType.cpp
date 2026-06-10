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

#include "algopt/rebalancer/solver/moves/FixedDestMultiMoveType.h"

#include "algopt/rebalancer/solver/moves/MoveHelper.h"

namespace facebook::rebalancer {

std::string FixedDestMultiMoveType::name() const {
  return kFixedDestMultiMoveTypeName.str();
}

MoveResult FixedDestMultiMoveType::findBestMove(
    const MovesEvaluator& evaluator,
    entities::ContainerId hotContainer,
    MoveStatsAggregator& stats,
    const SearchHints& hints,
    double timeLimit) {
  if (!spec_.specialContainer()) {
    throw std::runtime_error(
        "FixedDestMultiMoveType needs a special container to perform moves to");
  }

  auto& p = evaluator.getProblem();
  auto dstContainer = p.containerId(*spec_.specialContainer());

  return findBestMoveHelper(
      evaluator, hotContainer, dstContainer, stats, hints, timeLimit);
}

} // namespace facebook::rebalancer
