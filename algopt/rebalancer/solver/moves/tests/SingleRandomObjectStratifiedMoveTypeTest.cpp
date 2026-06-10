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
#include "algopt/rebalancer/solver/moves/SingleRandomObjectStratifiedMoveType.h"
#include "algopt/rebalancer/solver/moves/tests/MoveTestBase.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class SingleRandomObjectStratifiedMoveTypeTest : public MoveTestBase {
 protected:
  SingleRandomObjectStratifiedMoveTypeTest()
      : MoveTestBase("object", "container") {}

  folly::coro::Task<std::shared_ptr<const entities::Universe>> setUpUniverse() {
    setInitialAssignment({
        {"container1", {"object1", "object2", "object3", "object4"}},
        {"container2", {"object5", "object6", "object7", "object8"}},
        {"container3", {"object9", "object10", "object11", "object12"}},
        {"container4", {"object13", "object14", "object15", "object16"}},
        {"container5", {"object17", "object18", "object19", "object20"}},
    });

    co_await addPartition(
        "group",
        {
            {"group1", {"object1", "object6", "object11", "object16"}},
            {"group2", {"object2", "object7", "object12", "object17"}},
            {"group3", {"object3", "object8", "object13", "object18"}},
            {"group4", {"object4", "object9", "object14", "object19"}},
            {"group5", {"object5", "object10", "object15", "object20"}},
        });

    co_return buildUniverse();
  }

  static std::shared_ptr<PackerSet<entities::ContainerId>> getAllContainers(
      const entities::Universe& universe) {
    const auto allContainerIds = universe.getContainers().getContainerIds();
    return std::make_shared<PackerSet<entities::ContainerId>>(
        allContainerIds.begin(), allContainerIds.end());
  }
};

CO_TEST_F(SingleRandomObjectStratifiedMoveTypeTest, Basic) {
  interface::SingleRandomObjectStratifiedMoveTypeSpec spec;
  interface::GroupList groupList;
  groupList.partitionName() = "group";
  interface::ObjectsFromGroupsSpec objectsFromGroupsSpec;
  objectsFromGroupsSpec.groupList() = groupList;
  interface::ObjectsToExploreOptions objectsToExploreOptions;
  objectsToExploreOptions.set_objectsFromGroupsSpec(objectsFromGroupsSpec);
  spec.objectsToExploreOptions() = objectsToExploreOptions;
  interface::SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 15;

  spec.stratifiedSampleSize() = sampleSize;
  SingleRandomObjectStratifiedMoveType singleRandomObjectStratifiedMoveType(
      interface::LocalSearchSolverSpec{}, spec);

  const auto universe = co_await setUpUniverse();
  createProblem(
      /*objectiveTuple=*/{object_lookup(
          makeAllUnequalObjectVector(/*objectCount=*/9),
          getAllContainers(*universe),
          universe,
          Assignment(universe->getContainers().getInitialAssignment()))},
      /*constraint=*/const_expr(0, universe));

  auto bestResult = singleRandomObjectStratifiedMoveType.findBestMove(
      getMovesEvaluator(),
      container(2) /*hot container*/,
      getMoveStatsAggregator(),
      getEmptySearchHints(),
      std::numeric_limits<double>::max());

  EXPECT_EQ(10, getTotalMovesEvaluated());
  EXPECT_EQ(1, bestResult.getMoveSet().size());
  EXPECT_EQ(
      container(2), bestResult.getMoveSet().at(0).getDestinationContainer());
}

CO_TEST_F(SingleRandomObjectStratifiedMoveTypeTest, NoUsefulMoves) {
  const auto universe = co_await setUpUniverse();
  createProblem(
      /*objectiveTuple=*/
      {object_lookup(
          makeAllUnequalObjectVector(/*objectCount=*/15),
          getAllContainers(*universe),
          universe,
          Assignment(universe->getContainers().getInitialAssignment()))},
      /*constraint=*/const_expr(0, universe));

  interface::SingleRandomObjectStratifiedMoveTypeSpec spec;
  interface::GroupList groupList;
  groupList.partitionName() = "group";
  interface::ObjectsFromGroupsSpec objectsFromGroupsSpec;
  objectsFromGroupsSpec.groupList() = groupList;
  interface::ObjectsToExploreOptions objectsToExploreOptions;
  objectsToExploreOptions.set_objectsFromGroupsSpec(objectsFromGroupsSpec);
  spec.objectsToExploreOptions() = objectsToExploreOptions;
  interface::SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 15;

  spec.stratifiedSampleSize() = sampleSize;
  SingleRandomObjectStratifiedMoveType singleRandomObjectStratifiedMoveType(
      interface::LocalSearchSolverSpec{}, spec);

  auto bestResult = singleRandomObjectStratifiedMoveType.findBestMove(
      getMovesEvaluator(),
      container(5) /*hot container*/,
      getMoveStatsAggregator(),
      getEmptySearchHints(),
      std::numeric_limits<double>::max());
  EXPECT_EQ(
      bestResult.getValue().toString(), bestResult.getOldValue().toString());
  EXPECT_EQ(15, getTotalMovesEvaluated());
  EXPECT_EQ(1, bestResult.getMoveSet().size());
}

CO_TEST_F(SingleRandomObjectStratifiedMoveTypeTest, SampleSizeLargerThanTotal) {
  interface::SingleRandomObjectStratifiedMoveTypeSpec spec;
  interface::GroupList groupList;
  groupList.partitionName() = "group";
  interface::ObjectsFromGroupsSpec objectsFromGroupsSpec;
  objectsFromGroupsSpec.groupList() = groupList;
  interface::ObjectsToExploreOptions objectsToExploreOptions;
  objectsToExploreOptions.set_objectsFromGroupsSpec(objectsFromGroupsSpec);
  spec.objectsToExploreOptions() = objectsToExploreOptions;
  interface::SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 1000;

  spec.stratifiedSampleSize() = sampleSize;
  SingleRandomObjectStratifiedMoveType singleRandomObjectStratifiedMoveType(
      interface::LocalSearchSolverSpec{}, spec);

  const auto universe = co_await setUpUniverse();
  createProblem(
      /*objectiveTuple=*/{object_lookup(
          makeAllUnequalObjectVector(/*objectCount=*/9),
          getAllContainers(*universe),
          universe,
          Assignment(universe->getContainers().getInitialAssignment()))},
      /*constraint=*/const_expr(0, universe));

  auto bestResult = singleRandomObjectStratifiedMoveType.findBestMove(
      getMovesEvaluator(),
      container(2) /*hot container*/,
      getMoveStatsAggregator(),
      getEmptySearchHints(),
      std::numeric_limits<double>::max());

  // should still only evaluate 10 moves even though sample size is 1000
  EXPECT_EQ(10, getTotalMovesEvaluated());
  EXPECT_EQ(1, bestResult.getMoveSet().size());
  EXPECT_EQ(
      container(2), bestResult.getMoveSet().at(0).getDestinationContainer());
}

} // namespace facebook::rebalancer::packer::tests
