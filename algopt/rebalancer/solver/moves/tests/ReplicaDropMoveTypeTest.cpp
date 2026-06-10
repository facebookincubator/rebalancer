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

#include "algopt/rebalancer/algopt_common/TestUtils.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"
#include "algopt/rebalancer/solver/moves/ReplicaDropMoveType.h"
#include "algopt/rebalancer/solver/moves/tests/MoveTestBase.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class ReplicaDropMoveTypeTest : public MoveTestBase {
 protected:
  const std::string kPartitionName = "replica-set";
  const std::string kScopeName = "assigned_servers";

  ReplicaDropMoveTypeTest() : MoveTestBase("replica", "server") {}

  folly::coro::Task<std::shared_ptr<const entities::Universe>> setUpUniverse(
      const entities::Map<std::string, std::vector<std::string>>&
          initialAssignment) {
    setInitialAssignment(initialAssignment);

    co_await addPartition(
        kPartitionName,
        {
            {"group1", {"replica1", "replica2", "replica3", "replica4"}},
            {"group2", {"replica5", "replica6", "replica7", "replica8"}},
        });

    co_await addScope(
        kScopeName, {{"assigned", {"server1", "server3", "server4"}}});

    co_return buildUniverse();
  }

  static interface::ReplicaDropMoveTypeSpec getReplicaDropMoveTypeSpec() {
    interface::ReplicaDropMoveTypeSpec replicaDropMoveTypeSpec;
    replicaDropMoveTypeSpec.replicaDropPartition() = "replica-set";
    replicaDropMoveTypeSpec.replicaDropScope() = "assigned_servers";

    return replicaDropMoveTypeSpec;
  }

  static entities::Map<std::string, std::vector<std::string>>
  getDefaultAssignment() {
    return {
        {"server1", {"replica1", "replica2"}},
        {"server2", {"replica4"}},
        {"server3", {"replica5", "replica6", "replica7", "replica3"}},
        {"server4", {"replica8"}},
    };
  }
};

CO_TEST_F(ReplicaDropMoveTypeTest, VerifyMoveSetBasic) {
  auto replicaDropMoveType = ReplicaDropMoveType(
      interface::LocalSearchSolverSpec{}, getReplicaDropMoveTypeSpec());

  const auto universe = co_await setUpUniverse(getDefaultAssignment());

  auto containers = std::make_shared<PackerSet<entities::ContainerId>>(
      PackerSet<entities::ContainerId>{
          container(1), container(3), container(4)});

  createProblem(
      {object_lookup(
          makeAllUnequalObjectVector(8),
          containers,
          universe,
          Assignment(
              universe->getContainers().getInitialAssignment()))} /*objective*/,
      const_expr(0, universe) /*constraint*/);

  auto bestResult = replicaDropMoveType.findBestMove(
      getMovesEvaluator(),
      container(4) /*hotContainer*/,
      getMoveStatsAggregator(),
      getEmptySearchHints(),
      std::numeric_limits<double>::max() /*timelimit*/);

  // container(4) has only one object, object(8), that belongs to group,
  // group(2). Therefore, we expect 4 moves in total, where each object in the
  // group is moved to container(2)
  EXPECT_EQ(4, getTotalMovesEvaluated());

  // Note that object(8) has the highest dimension value, so the best move
  // should move it to container(2).
  const std::vector<Move> expectedMoveSet = {
      Move{object(8), container(4), container(2)}};
  REBALANCER_EXPECT_EQ_MOVESETS(expectedMoveSet, bestResult.getMoveSet());
}

CO_TEST_F(ReplicaDropMoveTypeTest, VerifyMoveSetWithOutOfScopeObject) {
  auto replicaDropMoveType = ReplicaDropMoveType(
      interface::LocalSearchSolverSpec{}, getReplicaDropMoveTypeSpec());

  const auto universe = co_await setUpUniverse(getDefaultAssignment());

  auto containers = std::make_shared<PackerSet<entities::ContainerId>>(
      PackerSet<entities::ContainerId>{
          container(1), container(3), container(4)});

  createProblem(
      {object_lookup(
          makeAllUnequalObjectVector(8),
          containers,
          universe,
          Assignment(
              universe->getContainers().getInitialAssignment()))} /*objective*/,
      const_expr(0, universe) /*constraint*/);

  auto bestResult = replicaDropMoveType.findBestMove(
      getMovesEvaluator(),
      container(1) /*hotContainer*/,
      getMoveStatsAggregator(),
      getEmptySearchHints(),
      std::numeric_limits<double>::max() /*timelimit*/);

  // container(1) has the presence of only one group, group(1). Out of this,
  // object(4) is already out of replica drop scope (since it is in
  // container(2)), therefore there will be only 3 move evaluations (one move
  // each for object(1), object(2), object(3) to container(2))
  EXPECT_EQ(3, getTotalMovesEvaluated());

  // Note that object(3) has the highest dimension value, so the best move
  // should move it from container(3) to container(2) (Note that hottest
  // container is container(1), but the best move is from container(3))
  const std::vector<Move> expectedMoveSet = {
      Move{object(3), container(3), container(2)}};
  REBALANCER_EXPECT_EQ_MOVESETS(expectedMoveSet, bestResult.getMoveSet());
}

CO_TEST_F(ReplicaDropMoveTypeTest, VerifyMoveSetBasicEmptyBest) {
  auto replicaDropMoveType = ReplicaDropMoveType(
      interface::LocalSearchSolverSpec{}, getReplicaDropMoveTypeSpec());

  const auto universe = co_await setUpUniverse(getDefaultAssignment());

  // make an object vector so that all group1 objects have value 0, and all
  // group2 objects have value -1. 0/-1 because we don't want any move to
  // succeed
  auto objVector = makeObjectVector(
      {{object(5), -1}, {object(6), -1}, {object(7), -1}, {object(8), -1}},
      0 /*defautValue*/,
      8 /*objectCount*/,
      universe);
  auto containers = std::make_shared<PackerSet<entities::ContainerId>>(
      PackerSet<entities::ContainerId>{
          container(1), container(3), container(4)});
  createProblem(
      {object_lookup(
          objVector,
          containers,
          universe,
          Assignment(
              universe->getContainers().getInitialAssignment()))} /*objective*/,
      const_expr(0, universe) /*constraint*/);

  auto bestResult = replicaDropMoveType.findBestMove(
      getMovesEvaluator(),
      container(3) /*hotContainer*/,
      getMoveStatsAggregator(),
      getEmptySearchHints(),
      std::numeric_limits<double>::max() /*timelimit*/);

  // container(3) has the presence of both groups. Also, since the object
  // dimension values are 0 and -1, no move will result in a better
  // objective. Hence, we just expect 3 + 4 evaluations, where there are 3
  // moves with objects in group(1) and 4 moves with objects in group(2)
  EXPECT_EQ(7, getTotalMovesEvaluated());

  EXPECT_TRUE(bestResult.isEmpty());
}

CO_TEST_F(ReplicaDropMoveTypeTest, VerifyMoveSetOutOfScopeHotContainer) {
  auto replicaDropMoveType = ReplicaDropMoveType(
      interface::LocalSearchSolverSpec{}, getReplicaDropMoveTypeSpec());

  const auto universe = co_await setUpUniverse(getDefaultAssignment());

  createProblem(
      {const_expr(0, universe)} /*objective*/,
      const_expr(0, universe) /*constraint*/);

  auto bestResult = replicaDropMoveType.findBestMove(
      getMovesEvaluator(),
      container(2) /*hotContainer*/,
      getMoveStatsAggregator(),
      getEmptySearchHints(),
      std::numeric_limits<double>::max() /*timelimit*/);

  // container(2) has only one object, object(4), that belongs to group,
  // group(1). Therefore, we expect 3 moves in total, where each object in the
  // group that is NOT out of scope is moved to container(2)
  EXPECT_EQ(3, getTotalMovesEvaluated());
}

CO_TEST_F(ReplicaDropMoveTypeTest, VerifyMoveSetMultipleOutOfScopeContainers) {
  auto replicaDropMoveType = ReplicaDropMoveType(
      interface::LocalSearchSolverSpec{}, getReplicaDropMoveTypeSpec());

  const auto universe = co_await setUpUniverse({
      {"server1", {"replica1", "replica2"}},
      {"server2", {"replica4"}},
      {"server3", {"replica5", "replica6", "replica7", "replica3"}},
      {"server4", {"replica8"}},
      {"server5", {}},
  });

  createProblem(
      {const_expr(0, universe)} /*objective*/,
      const_expr(0, universe) /*constraint*/);

  auto bestResult = replicaDropMoveType.findBestMove(
      getMovesEvaluator(),
      container(1) /*hotContainer*/,
      getMoveStatsAggregator(),
      getEmptySearchHints(),
      std::numeric_limits<double>::max() /*timelimit*/);

  // container(1) has the presence of only one group, group(1). Out of this,
  // object(4) is already out of replica drop scope (since it is in
  // container(2)), therefore there will be only 3*2 move evaluations (one
  // move each for object(1), object(2), object(3) to container(2) and
  // container(5))
  EXPECT_EQ(6, getTotalMovesEvaluated());
}

TEST_F(ReplicaDropMoveTypeTest, Errors) {
  {
    REBALANCER_EXPECT_RUNTIME_ERROR(
        auto replicaDropMoveType = ReplicaDropMoveType(
            interface::LocalSearchSolverSpec{},
            interface::ReplicaDropMoveTypeSpec()),
        "missing replica drop partition");
  }
  {
    interface::ReplicaDropMoveTypeSpec moveTypeSpec;
    moveTypeSpec.replicaDropPartition() = "replica-set";
    REBALANCER_EXPECT_RUNTIME_ERROR(
        auto replicaDropMoveType = ReplicaDropMoveType(
            interface::LocalSearchSolverSpec{}, moveTypeSpec),
        "missing replica drop scope");
  }
}

} // namespace facebook::rebalancer::packer::tests
