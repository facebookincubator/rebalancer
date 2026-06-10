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

constexpr folly::StringPiece kSingleRandomStratifiedMoveTypeName =
    "SINGLE_RANDOM_STRATIFIED";

// Pick an arbitrary object from the hot container, and evaluate moving it to a
// stratified random sample of destinations. It relies on the similar containers
// hint provided by the user. The sample is picked so that it contains about the
// same number of containers from each class of similarity. Containers within
// each class of similarity are chosen randomly, each with the same likelihood.
// Repeat the process with a different object if no improvement is found.
class SingleRandomStratifiedMoveType : public MoveType {
 public:
  std::string name() const override;

  explicit SingleRandomStratifiedMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::SingleRandomStratifiedMoveTypeSpec& spec);

  explicit SingleRandomStratifiedMoveType(
      const interface::LocalSearchSolverSpec& configs);

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

 private:
  PackerSet<entities::ContainerId> getSampledDestinationsSet(
      entities::ObjectId hotObject,
      entities::ContainerId hotContainer,
      Problem& problem);

  // TODO: make this non-optional
  std::optional<interface::SingleRandomStratifiedMoveTypeSpec> spec_ =
      std::nullopt;
};
} // namespace facebook::rebalancer
