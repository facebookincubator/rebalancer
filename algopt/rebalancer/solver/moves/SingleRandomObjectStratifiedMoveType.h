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

#include <string_view>

namespace facebook::rebalancer {

class Problem;

constexpr std::string_view kSingleRandomObjectStratifiedMoveTypeName{
    "SINGLE_RANDOM_OBJECT_STRATIFIED"};
/*
    Given a partition, this move type samples objects randomly in each group of
   the partition and moves the best object to the "hotContainer"
*/
class SingleRandomObjectStratifiedMoveType : public MoveType {
 public:
  std::string name() const override;

  explicit SingleRandomObjectStratifiedMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::SingleRandomObjectStratifiedMoveTypeSpec& spec);

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

 private:
  PackerSet<entities::ObjectId> getSampledSet(
      const ReferenceList<const std::vector<entities::ObjectId>>&
          similarObjectList) const;

 public:
  interface::SingleRandomObjectStratifiedMoveTypeSpec spec_;
};

} // namespace facebook::rebalancer
