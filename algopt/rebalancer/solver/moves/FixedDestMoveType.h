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

#include "algopt/rebalancer/solver/moves/SingleMoveType.h"

#include <folly/Random.h>

namespace facebook::rebalancer {

class Problem;

constexpr folly::StringPiece kFixedDestMoveTypeName = "SINGLE_FIXED_DEST";
constexpr folly::StringPiece kSampledFixedDestMoveTypeName =
    "SAMPLED_SINGLE_FIXED_DEST";

// This move types attempts to move objects from hot container to
// a pre-specified "special container" (cold container).
class FixedDestMoveType : public SingleMoveType {
 public:
  std::string name() const override;

  explicit FixedDestMoveType(
      const interface::LocalSearchSolverSpec& configs =
          interface::LocalSearchSolverSpec(),
      const interface::FixedDestMoveTypeSpec& spec =
          interface::FixedDestMoveTypeSpec());

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

  std::optional<PackerSet<entities::ContainerId>> getCustomColdContainers(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer) const override;

  static void makeSampled(
      interface::FixedDestMoveTypeSpec& spec,
      int sampleSize);

 protected:
  bool attemptMoveWithThisObject(
      const MovesEvaluator& evaluator,
      entities::ObjectId objectId,
      int numObjects) const override;

  bool getSamplingStatus(int numObjects) const;

  MoveResult findBestMoveWithBundleOptions(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit,
      const interface::ObjectsToExploreOptions& bundleOptions);

 private:
  void getBundleMoveCandidates(
      entities::ContainerId srcContainerId,
      entities::ContainerId dstContainerId,
      const MovesEvaluator& evaluator,
      const interface::ObjectsToExploreOptions& bundleOptions,
      folly::F14FastSet<std::pair<ObjectBundle, entities::ContainerId>>& moves);

  interface::FixedDestMoveTypeSpec spec_;
  std::shared_ptr<folly::Random::DefaultGenerator> rng_;
};

} // namespace facebook::rebalancer
