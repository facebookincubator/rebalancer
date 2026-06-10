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

#include "algopt/rebalancer/solver/moves/SingleChainFastMoveType.h"

#include "algopt/rebalancer/solver/iterators/Filter.h"
#include "algopt/rebalancer/solver/moves/MoveHelper.h"
#include "algopt/rebalancer/solver/utils/ObjectDeduper.h"

namespace facebook::rebalancer {

std::string SingleChainFastMoveType::name() const {
  return kSingleChainFastMoveTypeName.str();
}

SingleChainFastMoveType::SingleChainFastMoveType(
    const interface::LocalSearchSolverSpec& solverConfigs,
    const interface::SingleChainFastMoveTypeSpec& spec)
    : SingleChainMoveType(solverConfigs, interface::SingleChainMoveTypeSpec()) {
  partitionNameToExploreFastChainsWithinObjectGroup_ =
      spec.partitionNameToExploreFastChainsWithinObjectGroup().to_optional();
  specialFastColdContainer_ = spec.specialFastColdContainer().to_optional();
}

MoveResult SingleChainFastMoveType::findBestMove(
    const MovesEvaluator& evaluator,
    entities::ContainerId hotContainer,
    MoveStatsAggregator& stats,
    const SearchHints& /*hints*/,
    double timeLimit) {
  auto& dynamicObjects = evaluator.getDynamicObjects(hotContainer);
  auto& problem = evaluator.getProblem();
  const ObjectDeduper dedupedObjs(
      &problem.getEquivalenceSets(), dynamicObjects);

  auto coldContainers = Filter(
      problem.containers,
      [&problem, hotContainer](entities::ContainerId container) {
        return container != hotContainer &&
            !problem.not_accepting_containers.contains(container);
      });

  auto bestResult = MoveResult::makeEmpty();
  const auto& precision = problem.getUniverse().getPrecision();

  for (const auto hotObject : dedupedObjs) {
    const std::function<MoveResult(entities::ContainerId)> evaluate =
        [this, &evaluator, &stats, hotContainer, hotObject](
            auto coldContainer) {
          return exploreChainsWithHotObjectToColdContainer(
              evaluator,
              hotContainer,
              hotObject,
              coldContainer,
              stats,
              true /* greedy */);
        };

    auto result = MoveHelper::findBest(
        problem.configs.threadPool.get(),
        coldContainers,
        evaluate,
        timeLimit,
        getParallelExecutionConfig());
    bestResult.aggregate(std::move(result));
    if (bestResult.isBetter(precision)) {
      break;
    }
  }
  return bestResult;
}

} // namespace facebook::rebalancer
