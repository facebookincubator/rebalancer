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

#include <folly/Random.h>

namespace facebook::rebalancer {

class Problem;

constexpr folly::StringPiece kSingleFixedSourceMoveTypeName =
    "SINGLE_FIXED_SOURCE";
constexpr folly::StringPiece kSampledFixedSourceMoveTypeName =
    "SAMPLED_SINGLE_FIXED_SOURCE";

// This move types attempts to move objects from a pre-specified "special
// container" (fixed source) to the hot container such that the objective is
// improved. This is a base class for SINGLE and MULTIPLE FIXED_SOURCE move
// types
class FixedSourceMoveType : public MoveType {
 public:
  std::string name() const override;

  explicit FixedSourceMoveType(
      const interface::LocalSearchSolverSpec& configs =
          interface::LocalSearchSolverSpec(),
      const interface::SingleFixedSourceMoveTypeSpec& singleFixedSourceSpec =
          interface::SingleFixedSourceMoveTypeSpec());

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

  // some common specializations of this move type config
  static void makeSampled(
      interface::SingleFixedSourceMoveTypeSpec& spec,
      int sampleSize);

 protected:
  // If sampling settings were provided, this function will return true with
  // probability `sampleSize/ numMoveCandidates`. If sampling was disabled, this
  // will always return true
  bool getSamplingStatus(int numObjects) const;

 private:
  void getSingleMoveCandidates(
      entities::ContainerId srcContainerId,
      const MovesEvaluator& evaluator,
      folly::F14FastSet<std::pair<ObjectBundle, entities::ContainerId>>& moves);

  void getBundleMoveCandidates(
      entities::ContainerId srcContainerId,
      entities::ContainerId dstContainerId,
      const MovesEvaluator& evaluator,
      const interface::ObjectsToExploreOptions& bundleOptions,
      folly::F14FastSet<std::pair<ObjectBundle, entities::ContainerId>>& moves);

  std::shared_ptr<folly::Random::DefaultGenerator> rng_;
  interface::SingleFixedSourceMoveTypeSpec singleFixedSourceSpec_;
};
} // namespace facebook::rebalancer
