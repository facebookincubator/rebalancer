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

#include "algopt/rebalancer/solver/moves/AsyncSingleMovesMoveType.h"

#include "algopt/rebalancer/algopt_common/Timer.h"
#include "algopt/rebalancer/common/CoroUtils.h"
#include "algopt/rebalancer/solver/iterators/Filter.h"
#include "algopt/rebalancer/solver/moves/MovesEvaluator.h"
#include "algopt/rebalancer/solver/moves/MoveTypeUtils.h"
#include "algopt/rebalancer/solver/utils/ObjectDeduper.h"

#include <folly/coro/BlockingWait.h>

namespace facebook::rebalancer {

MoveResult AsyncSingleMovesMoveType::findBestMove(
    const MovesEvaluator& evaluator,
    entities::ContainerId hotContainer,
    MoveStatsAggregator& stats,
    const SearchHints& /*hints*/,
    double timeLimit) {
  auto& dynamicObjects = evaluator.getDynamicObjects(hotContainer);
  const auto numObjects = static_cast<int>(dynamicObjects.size());
  auto& problem = evaluator.getProblem();
  const auto dedupedObjs =
      ObjectDeduper(&problem.getEquivalenceSets(), dynamicObjects);
  auto customColdContainers = getCustomColdContainers(evaluator, hotContainer);

  // Create filtered collections for objects and containers
  auto filteredObjects = Filter(
      dedupedObjs, [this, &evaluator, numObjects](entities::ObjectId object) {
        return attemptMoveWithThisObject(evaluator, object, numObjects);
      });

  auto filteredContainers = Filter(
      customColdContainers ? *customColdContainers : problem.containers,
      getAcceptingContainersCheckFunc(problem, hotContainer));

  algopt::Timer timer;
  timer.start();

  return folly::coro::blockingWait(
      CoroUtils::parallelMapReduceAllPairs(
          /* container1 */
          filteredObjects,
          /* container2 */ filteredContainers,
          /* workFunc - processes each (hotObject, coldContainer) pair */
          [this, &timer, timeLimit, &evaluator, &stats, hotContainer](
              entities::ObjectId hotObject,
              entities::ContainerId coldContainer) {
            if (timer.getSeconds() >= timeLimit) {
              return MoveResult::makeEmpty();
            }
            return exploreFromAllSingleMoves(
                evaluator, hotContainer, hotObject, coldContainer, stats);
          },
          /* accumulateFunc - merges batch results into final result */
          [](MoveResult& bestResult, MoveResult&& batchResult) {
            bestResult.aggregate(std::move(batchResult));
          },
          /* initResultFunc - creates empty result for accumulation */
          []() { return MoveResult::makeEmpty(); },
          /* givenExecutor - thread pool for parallel execution */
          evaluator.getProblem().configs.threadPool));
}

std::optional<PackerSet<entities::ContainerId>>
AsyncSingleMovesMoveType::getCustomColdContainers(
    const MovesEvaluator& /*evaluator*/,
    entities::ContainerId /*hotContainer*/) const {
  return std::nullopt;
}

bool AsyncSingleMovesMoveType::attemptMoveWithThisObject(
    const MovesEvaluator& evaluator,
    entities::ObjectId objectId,
    int /*numObjectsInContainer*/) const {
  // although we only sample dynamic objects, we still need to check fixed
  // objects to support the case with restrict_moving_object_only_once
  return !evaluator.getProblem().fixed_objects.contains(objectId);
}

} // namespace facebook::rebalancer
