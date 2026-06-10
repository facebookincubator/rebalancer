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

constexpr folly::StringPiece kReplicaDropMoveTypeName = "REPLICA_DROP";

// Evaluates moving objects in replicaDropScope to containers out of the scope.
// It compares moving each possible object in the same replica group, determined
// by replicaDropPartition, before choosing the best option. This move type can
// be used for solving models where shards are over-replicated and the solver
// must choose the best replicas to drop.
class ReplicaDropMoveType : public MoveType {
 public:
  explicit ReplicaDropMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::ReplicaDropMoveTypeSpec& spec);

  std::string name() const override;

  MoveResult findBestMove(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      MoveStatsAggregator& stats,
      const SearchHints& hints,
      double timeLimit) override;

 private:
  std::string partition_;
  std::string scope_;
};

} // namespace facebook::rebalancer
