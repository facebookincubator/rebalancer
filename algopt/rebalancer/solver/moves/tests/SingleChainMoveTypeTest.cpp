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

#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"
#include "algopt/rebalancer/solver/moves/SingleChainMoveType.h"
#include "algopt/rebalancer/solver/moves/tests/MoveTestBase.h"

#include <fmt/core.h>
#include <folly/container/irange.h>
#include <folly/coro/BlockingWait.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class MockSingleChainMoveType : public SingleChainMoveType {
 public:
  explicit MockSingleChainMoveType(
      const interface::LocalSearchSolverSpec& solverConfigs,
      const interface::SingleChainMoveTypeSpec& spec)
      : SingleChainMoveType(solverConfigs, spec) {}

  MoveResult exploreWithinGroupAndGetBestResult(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      entities::ObjectId hotObject,
      entities::ContainerId coldContainer,
      entities::ContainerId otherContainer,
      const std::string& partitionName,
      MoveStatsAggregator& stats) {
    return this->SingleChainMoveType::exploreWithinGroupAndGetBestResult(
        evaluator,
        hotContainer,
        hotObject,
        coldContainer,
        otherContainer,
        partitionName,
        stats,
        false /*greedy*/);
  }

  MoveResult exploreAllAndGetBestResult(
      const MovesEvaluator& evaluator,
      entities::ContainerId hotContainer,
      entities::ObjectId hotObject,
      entities::ContainerId coldContainer,
      entities::ContainerId otherContainer,
      MoveStatsAggregator& stats) {
    return this->SingleChainMoveType::exploreAllAndGetBestResult(
        evaluator,
        hotContainer,
        hotObject,
        coldContainer,
        otherContainer,
        stats,
        false /*greedy*/);
  }

  MoveResult generateMoveSetAndGetResult(
      const MovesEvaluator& evaluator,
      entities::ObjectId hotObject,
      entities::ContainerId hotContainer,
      entities::ContainerId coldContainer,
      entities::ObjectId otherObject,
      entities::ContainerId otherContainer,
      MoveStatsAggregator& stats) {
    return this->SingleChainMoveType::generateMoveSetAndGetResult(
        evaluator,
        hotObject,
        hotContainer,
        coldContainer,
        otherObject,
        otherContainer,
        stats);
  }
};

class SingleChainMoveTypeTest : public MoveTestBase {
 protected:
  const std::string kPartitionName = "tenant";

  SingleChainMoveTypeTest() : MoveTestBase("replica", "region") {}

  folly::coro::Task<std::shared_ptr<const entities::Universe>> setUpUniverse() {
    setInitialAssignment({
        {"region1", {"replica1", "replica2", "replica3", "replica4"}},
        {"region2", {}},
        {"region3", {"replica5", "replica6", "replica7"}},
        {"region4", {"replica8"}},
    });

    co_await addPartition(
        kPartitionName,
        {
            {"tenant1", {"replica1", "replica3", "replica5", "replica7"}},
            {"tenant2", {"replica2", "replica4", "replica6", "replica8"}},
        });

    co_return buildUniverse();
  }

  ExprPtr getLookupExprOn(PackerSet<entities::ContainerId> containers) {
    entities::ObjectIdToDoubleMap objectValues(
        /*totalSize=*/9, /*defaultValue=*/0.0, /*expectedNonDefaultSize=*/8);
    for (const auto id : folly::irange(1, 9)) {
      objectValues.emplace(object(id), id);
    }
    auto objectVector = makeObjectVector(objectValues, getUniversePtr());
    const Assignment assignment(
        getUniverse().getContainers().getInitialAssignment());
    return std::make_shared<ObjectLookup>(
        objectVector,
        std::make_shared<PackerSet<entities::ContainerId>>(containers),
        getUniversePtr(),
        assignment);
  }
};

CO_TEST_F(SingleChainMoveTypeTest, TestMoveSet) {
  auto mockSingleChainMoveType = MockSingleChainMoveType(
      interface::LocalSearchSolverSpec{}, interface::SingleChainMoveTypeSpec());

  co_await setUpUniverse();

  // objective and constraint are not relevant when directly testing
  // for the moves evaluated in 'generateMoveSetAndGetResult'
  const auto universe = getUniversePtr();
  createProblem({const_expr(0, universe)}, const_expr(0, universe));

  auto moveResult = mockSingleChainMoveType.generateMoveSetAndGetResult(
      getMovesEvaluator(),
      object(1) /*hotObject*/,
      container(1) /*hotContainer*/,
      container(2) /*coldContainer*/,
      object(5) /*otherObject*/,
      container(3) /*otherContainer*/,
      getMoveStatsAggregator());

  // we expect 1 moveSets to be evaluated which has 2 moves.
  EXPECT_EQ(2, getTotalMovesEvaluated());

  const std::vector<Move> expectedMoveSet = {
      Move{object(5), container(3), container(1)},
      Move{object(1), container(1), container(2)}};
  REBALANCER_EXPECT_EQ_MOVESETS(expectedMoveSet, moveResult.getMoveSet());
}

CO_TEST_F(
    SingleChainMoveTypeTest,
    ExploreMovesOnlyWithinGroupInOtherContainer) {
  interface::SingleChainMoveTypeSpec singleChainSpec;
  singleChainSpec.partitionNameToExploreChainsWithinObjectGroup() =
      kPartitionName;

  auto mockSingleChainMoveType = MockSingleChainMoveType(
      interface::LocalSearchSolverSpec{}, singleChainSpec);

  const auto universe = co_await setUpUniverse();
  // lookup expression ensures that the no two objects are equivalent
  createProblem({getLookupExprOn({container(1)})}, const_expr(0, universe));

  mockSingleChainMoveType.exploreWithinGroupAndGetBestResult(
      getMovesEvaluator(),
      container(1) /*hotContainer*/,
      object(1) /*hotObject*/,
      container(2) /*coldContainer*/,
      container(3) /*otherContainer*/,
      kPartitionName,
      getMoveStatsAggregator());

  // we expect 2 moveSets to be evaluated each with 2 moves.
  EXPECT_EQ(4, getTotalMovesEvaluated());
}

CO_TEST_F(SingleChainMoveTypeTest, ExploreMovesWithAllObjectsInOtherContainer) {
  auto mockSingleChainMoveType = MockSingleChainMoveType(
      interface::LocalSearchSolverSpec{}, interface::SingleChainMoveTypeSpec());

  const auto universe = co_await setUpUniverse();
  // lookup expression ensures that the no two objects are equivalent
  createProblem({getLookupExprOn({container(1)})}, const_expr(0, universe));

  mockSingleChainMoveType.exploreAllAndGetBestResult(
      getMovesEvaluator(),
      container(1) /*hotContainer*/,
      object(1) /*hotObject*/,
      container(2) /*coldContainer*/,
      container(3) /*otherContainer*/,
      getMoveStatsAggregator());

  // we expect 3 moveSets (because otherContainer container(3) has 3
  // non-equivalent objects objects) to be evaluated each with 2 moves.
  EXPECT_EQ(6, getTotalMovesEvaluated());
}

CO_TEST_F(
    SingleChainMoveTypeTest,
    IgnoreEquivalentObjectsWhenExploringWithinGroup) {
  interface::SingleChainMoveTypeSpec singleChainSpec;
  singleChainSpec.partitionNameToExploreChainsWithinObjectGroup() =
      kPartitionName;

  auto mockSingleChainMoveType = MockSingleChainMoveType(
      interface::LocalSearchSolverSpec{}, singleChainSpec);

  const auto universe = co_await setUpUniverse();
  // since objective and constraint are const_expr(0), all
  // objects will be equivalent
  createProblem({const_expr(0, universe)}, const_expr(0, universe));

  mockSingleChainMoveType.exploreWithinGroupAndGetBestResult(
      getMovesEvaluator(),
      container(1) /*hotContainer*/,
      object(1) /*hotObject*/,
      container(2) /*coldContainer*/,
      container(3) /*otherContainer*/,
      kPartitionName,
      getMoveStatsAggregator());

  EXPECT_EQ(2, getTotalMovesEvaluated());
}

CO_TEST_F(SingleChainMoveTypeTest, IgnoreEquivalentObjectsWhenExploringAll) {
  auto mockSingleChainMoveType = MockSingleChainMoveType(
      interface::LocalSearchSolverSpec{}, interface::SingleChainMoveTypeSpec());

  const auto universe = co_await setUpUniverse();
  // since objective and constraint are const_expr(0, universe), all
  // objects will be equivalent
  createProblem({const_expr(0, universe)}, const_expr(0, universe));

  mockSingleChainMoveType.exploreAllAndGetBestResult(
      getMovesEvaluator(),
      container(1) /*hotContainer*/,
      object(1) /*hotObject*/,
      container(2) /*coldContainer*/,
      container(3) /*otherContainer*/,
      getMoveStatsAggregator());

  EXPECT_EQ(2, getTotalMovesEvaluated());
}

CO_TEST_F(SingleChainMoveTypeTest, TestTotalNumberOfEvaluations) {
  auto mockSingleChainMoveType = MockSingleChainMoveType(
      interface::LocalSearchSolverSpec{}, interface::SingleChainMoveTypeSpec());

  const auto universe = co_await setUpUniverse();
  // lookup expression ensures that the no two objects are equivalent
  createProblem({getLookupExprOn({container(1)})}, const_expr(0, universe));

  mockSingleChainMoveType.exploreFromAllSingleMoves(
      getMovesEvaluator(),
      container(1) /*hotContainer*/,
      object(1) /*hotObject*/,
      container(2) /*coldContainer*/,
      getMoveStatsAggregator());

  // we expect 4 moveSet evaluations, 3 with container(3) as the
  // 'otherContainer' and 1 with container(4) as the 'otherContainer'.
  EXPECT_EQ(8, getTotalMovesEvaluated());
}

TEST_F(SingleChainMoveTypeTest, Name) {
  auto mockSingleChainMoveType = MockSingleChainMoveType(
      interface::LocalSearchSolverSpec{}, interface::SingleChainMoveTypeSpec());
  EXPECT_EQ(mockSingleChainMoveType.name(), "SINGLE_CHAIN");
}

} // namespace facebook::rebalancer::packer::tests
