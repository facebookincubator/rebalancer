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
#include "algopt/rebalancer/solver/moves/DestinationsToExploreGenerator.h"
#include "algopt/rebalancer/solver/moves/tests/MoveTestBase.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class DestinationsToExploreGeneratorTest : public MoveTestBase {
 protected:
  DestinationsToExploreGeneratorTest() : MoveTestBase("object", "server") {}

  folly::coro::Task<void> setUpProblem(
      const std::vector<int>& nonAcceptingContainerIndices = {}) {
    // the initial assignment does not matter for the tests below
    setInitialAssignment({
        {"server1", {"object1"}},
        {"server2", {"object2"}},
        {"server3", {"object3"}},
        {"server4", {"object4"}},
        {"server5", {}},
    });

    // all containers except container(2) and container(5) are in the same
    // region; container(5) is not in this scope
    co_await addScope(
        "region",
        {{"region1", {"server1", "server3", "server4"}},
         {"region2", {"server2"}}});

    co_await addPartition(
        "job", {{"job1", {"object1", "object3"}}, {"job2", {"object2"}}});

    const auto universe = buildUniverse();

    PackerSet<entities::ContainerId> nonAcceptingContainers;
    for (const int idx : nonAcceptingContainerIndices) {
      nonAcceptingContainers.insert(container(idx));
    }

    createProblem(
        {const_expr(0, universe)} /*objective*/,
        const_expr(0, universe) /*constraint*/,
        std::nullopt /*higherPriorityObjConfig*/,
        nonAcceptingContainers);
  }

  static void verifyContainersAreAsExpected(
      const std::set<entities::ContainerId>& expected,
      const std::vector<entities::ContainerId>& result) {
    EXPECT_EQ(expected.size(), result.size());
    EXPECT_EQ(
        expected,
        std::set<entities::ContainerId>(result.begin(), result.end()));
  }

  static interface::MoveToCurrentScopeItemSpec getMoveToCurrentScopeItemSpec() {
    interface::MoveToCurrentScopeItemSpec spec;
    spec.scopeNameForExploringMovesToCurrentScopeItem() = "region";

    return spec;
  }
};

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    getAcceptingDestinationsTestWithoutNonAccepting) {
  co_await setUpProblem();
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();
  auto moveToCurrentScopeItemSpec = getMoveToCurrentScopeItemSpec();

  {
    auto& containers =
        destinationsGenerator
            .getAcceptingDestinations(moveToCurrentScopeItemSpec, container(1))
            .at(0)
            .get();

    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(3), container(4)};

    EXPECT_EQ(containers.size(), 3);
    EXPECT_EQ(
        expectedContainers,
        std::set<entities::ContainerId>(containers.begin(), containers.end()));
  }
  {
    auto& containers =
        destinationsGenerator
            .getAcceptingDestinations(moveToCurrentScopeItemSpec, container(2))
            .at(0)
            .get();
    EXPECT_EQ(containers.size(), 1);
  }

  {
    // container(5) is not part of "region" scope, so we expect to explore all
    // destinations
    auto containers = destinationsGenerator.getAcceptingDestinations(
        moveToCurrentScopeItemSpec, container(5));
    EXPECT_EQ(containers.size(), 2);
    verifyContainersAreAsExpected(
        std::set<entities::ContainerId>(
            {container(1), container(3), container(4)}),
        containers.at(0).get());

    verifyContainersAreAsExpected(
        std::set<entities::ContainerId>({container(2)}),
        containers.at(1).get());
  }
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    getAcceptingDestinationsTestWithNonAccepting) {
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();
  auto moveToCurrentScopeItemSpec = getMoveToCurrentScopeItemSpec();

  auto& containers =
      destinationsGenerator
          .getAcceptingDestinations(moveToCurrentScopeItemSpec, container(1))
          .at(0)
          .get();

  const std::set<entities::ContainerId> expectedContainers = {
      container(1), container(4)};
  EXPECT_EQ(containers.size(), 2);
  EXPECT_EQ(
      expectedContainers,
      std::set<entities::ContainerId>(containers.begin(), containers.end()));
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    getAcceptingDestinationsWithMoveToScopeItemsSpecAllDefault) {
  co_await setUpProblem({3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();

  // since no list of scopeItems is specified, all scopeItems in region are
  // considered
  interface::ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "region";

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;

  auto object1Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(1), container(1));
  auto object2Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(2), container(2));

  EXPECT_EQ(object1Destinations.size(), 2);
  EXPECT_EQ(object2Destinations.size(), 2);

  {
    auto& object1FirstContainerSet = object1Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, object1FirstContainerSet);
  }

  {
    auto& object1SecondContainerSet = object1Destinations.at(1).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(
        expectedContainers, object1SecondContainerSet);
  }

  {
    auto& object2FirstContainerSet = object2Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, object2FirstContainerSet);
  }

  {
    auto& object2SecondContainerSet = object2Destinations.at(1).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(
        expectedContainers, object2SecondContainerSet);
  }
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    getAcceptingDestinationsWithMoveToScopeItemsSpecDefault) {
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();

  // by default try to all scope items in region
  interface::ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "region";

  // object2 wants only destinations in region2
  interface::ScopeItemList scopeItemList;
  scopeItemList.scopeName() = "region";
  scopeItemList.scopeItems() = {"region2"};

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;
  moveToScopeItemsSpec.objectToScopeItems() = {{"object2", scopeItemList}};

  auto object1Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(1), container(1));
  auto object2Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(2), container(2));

  EXPECT_EQ(object1Destinations.size(), 2);
  EXPECT_EQ(object2Destinations.size(), 1);

  {
    auto& object1FirstContainerSet = object1Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, object1FirstContainerSet);
  }

  {
    auto& object1SecondContainerSet = object1Destinations.at(1).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(
        expectedContainers, object1SecondContainerSet);
  }

  {
    auto& object2FirstContainerSet = object2Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(expectedContainers, object2FirstContainerSet);
  }
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    getAcceptingDestinationsWithMoveToScopeItemsSpecDifferentScopes) {
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();

  // object1 wants only container(1)
  interface::ScopeItemList scopeItemList1;
  scopeItemList1.scopeName() = "server";
  scopeItemList1.scopeItems() = {"server1"};

  // object2 wants only destinations in region2
  interface::ScopeItemList scopeItemList2;
  scopeItemList2.scopeName() = "region";
  scopeItemList2.scopeItems() = {"region2"};

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.objectToScopeItems() = {
      {"object1", scopeItemList1}, {"object2", scopeItemList2}};

  auto object1Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(1), container(1));
  auto object2Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(2), container(2));

  EXPECT_EQ(object1Destinations.size(), 1);
  EXPECT_EQ(object2Destinations.size(), 1);

  {
    auto& object1FirstContainerSet = object1Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {container(1)};
    verifyContainersAreAsExpected(expectedContainers, object1FirstContainerSet);
  }

  {
    auto& object2FirstContainerSet = object2Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(expectedContainers, object2FirstContainerSet);
  }
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    getAcceptingDestinationsWithMoveToScopeItemsSpecSomeEmpty) {
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();

  // object1 wants no destinations
  interface::ScopeItemList scopeItemList1;
  scopeItemList1.scopeName() = "server";
  scopeItemList1.scopeItems() = {};

  // object2 wants only destinations in region2
  interface::ScopeItemList scopeItemList2;
  scopeItemList2.scopeName() = "region";
  scopeItemList2.scopeItems() = {"region2"};

  // object3 wants only container(3), but that is non-accepting
  interface::ScopeItemList scopeItemList3;
  scopeItemList3.scopeName() = "server";
  scopeItemList3.scopeItems() = {"server3"};

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.objectToScopeItems() = {
      {"object1", scopeItemList1},
      {"object2", scopeItemList2},
      {"object3", scopeItemList3}};

  auto object1Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(1), container(1));
  auto object2Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(2), container(2));
  auto object3Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(3), container(3));

  EXPECT_EQ(object1Destinations.size(), 0);
  EXPECT_EQ(object2Destinations.size(), 1);
  EXPECT_EQ(object3Destinations.size(), 1);

  {
    auto& object2FirstContainerSet = object2Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(expectedContainers, object2FirstContainerSet);
  }

  {
    auto& object3FirstContainerSet = object3Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {};
    verifyContainersAreAsExpected(expectedContainers, object3FirstContainerSet);
  }
}

CO_TEST_F(DestinationsToExploreGeneratorTest, groupToScopeItems) {
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();

  // object1 and 3 wants destinations in region1
  interface::ScopeItemList scopeItemList1;
  scopeItemList1.scopeName() = "region";
  scopeItemList1.scopeItems() = {"region1"};

  // object2 wants only destinations in region2
  interface::ScopeItemList scopeItemList2;
  scopeItemList2.scopeName() = "region";
  scopeItemList2.scopeItems() = {"region2"};

  interface::GroupToScopeItemList groupToScopeItems;
  groupToScopeItems.groupToScopeItemList() = {
      {"job1", scopeItemList1}, {"job2", scopeItemList2}};
  groupToScopeItems.partitionName() = "job";

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.scopeItemsPerGroups() = groupToScopeItems;

  auto object1Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(1), container(1));
  auto object2Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(2), container(2));
  auto object3Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(3), container(3));

  EXPECT_EQ(object1Destinations.size(), 1);
  EXPECT_EQ(object2Destinations.size(), 1);
  EXPECT_EQ(object3Destinations.size(), 1);

  {
    auto& object1FirstContainerSet = object1Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, object1FirstContainerSet);
  }

  {
    auto& object2FirstContainerSet = object2Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(expectedContainers, object2FirstContainerSet);
  }

  {
    auto& object3FirstContainerSet = object3Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, object3FirstContainerSet);
  }
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    objectToScopeItemHighPriorityThanGroupToScopeItems) {
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();

  // object1 and 3 wants destinations in region1
  interface::ScopeItemList scopeItemList1;
  scopeItemList1.scopeName() = "region";
  scopeItemList1.scopeItems() = {"region1"};

  // object2 wants only destinations in region2
  interface::ScopeItemList scopeItemList2;
  scopeItemList2.scopeName() = "region";
  scopeItemList2.scopeItems() = {"region2"};

  // object3 does not want any destinations
  interface::ScopeItemList scopeItemList3;
  scopeItemList3.scopeName() = "region";
  scopeItemList3.scopeItems() = {};

  interface::GroupToScopeItemList groupToScopeItems;
  groupToScopeItems.groupToScopeItemList() = {
      {"job1", scopeItemList1}, {"job2", scopeItemList2}};
  groupToScopeItems.partitionName() = "job";

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.scopeItemsPerGroups() = groupToScopeItems;
  moveToScopeItemsSpec.objectToScopeItems() = {{"object3", scopeItemList3}};

  auto object1Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(1), container(1));
  auto object2Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(2), container(2));
  auto object3Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(3), container(3));

  EXPECT_EQ(object1Destinations.size(), 1);
  EXPECT_EQ(object2Destinations.size(), 1);
  EXPECT_EQ(object3Destinations.size(), 0);

  {
    auto& object1FirstContainerSet = object1Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, object1FirstContainerSet);
  }

  {
    auto& object2FirstContainerSet = object2Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(expectedContainers, object2FirstContainerSet);
  }
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    exploreCurrentScopeItemAsDefault) {
  // In contrast to getAcceptingDestinationsWithMoveToScopeItemsSpecAllDefault
  // test, we explore only the current current scope item as default here.
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();
  interface::ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "region";
  defaultScopeItems.exploreCurrentScopeItem() = true;

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;

  auto object1Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(1), container(1));
  auto object2Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(2), container(2));

  EXPECT_EQ(object1Destinations.size(), 1);
  EXPECT_EQ(object2Destinations.size(), 1);

  {
    auto& object1FirstContainerSet = object1Destinations.at(0).get();
    // container(1) is in region1, so only region1 is explored
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, object1FirstContainerSet);
  }

  {
    auto& object2FirstContainerSet = object2Destinations.at(0).get();
    // container(2) is in region2, so only region2 is explored
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(expectedContainers, object2FirstContainerSet);
  }
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    exploreAllScopeItemsObjectSpecific) {
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();
  interface::ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "region";
  defaultScopeItems.exploreCurrentScopeItem() = true;

  interface::ScopeItemList object2ScopeItems;
  object2ScopeItems.scopeName() = "region";
  object2ScopeItems.scopeItems() = {"region1", "region2"};

  interface::ScopeItemList object3ScopeItems;
  object3ScopeItems.scopeName() = "region";
  object3ScopeItems.exploreCurrentScopeItem() = true;

  interface::ScopeItemList object4ScopeItems;
  object4ScopeItems.scopeName() = "region";
  object4ScopeItems.scopeItems() = {"region2"};

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;
  moveToScopeItemsSpec.objectToScopeItems() = {
      {"object4", object4ScopeItems},
      {"object2", object2ScopeItems},
      {"object3", object3ScopeItems}};

  auto object1Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(1), container(1));
  auto object2Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(2), container(2));
  auto object3Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(3), container(3));

  // please note that object4 is not in the initial assignment, but we assume
  // all input is correct
  auto object4Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(4), container(1));

  EXPECT_EQ(object1Destinations.size(), 1);
  EXPECT_EQ(object2Destinations.size(), 2);
  EXPECT_EQ(object3Destinations.size(), 1);
  EXPECT_EQ(object4Destinations.size(), 1);

  {
    auto& object1FirstContainerSet = object1Destinations.at(0).get();
    // container(1) is in region1, so only region1 is explored
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, object1FirstContainerSet);
  }

  {
    auto& object2FirstContainerSet = object2Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, object2FirstContainerSet);
  }
  {
    auto& object2SecondContainerSet = object2Destinations.at(1).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(
        expectedContainers, object2SecondContainerSet);
  }
  {
    auto& object3FirstContainerSet = object3Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, object3FirstContainerSet);
  }
  {
    auto& object4FirstContainerSet = object4Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(expectedContainers, object4FirstContainerSet);
  }
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    getAcceptingDestinationsWithContainerIdOnly) {
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();

  // Create a MoveToScopeItemsSpec with empty objectToScopeItems
  interface::ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "region";

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;
  // objectToScopeItems is empty by default

  // Call the method that takes only MoveToScopeItemsSpec and hotContainerId
  auto containerDestinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, container(1));

  // Should return destinations for all scope items in the region
  EXPECT_EQ(containerDestinations.size(), 2);

  {
    auto& firstContainerSet = containerDestinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, firstContainerSet);
  }

  {
    auto& secondContainerSet = containerDestinations.at(1).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(expectedContainers, secondContainerSet);
  }
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    getAcceptingDestinationsExploreCurrentScopeItemNotInScope) {
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();

  // Create a ScopeItemList with exploreCurrentScopeItem set to true
  interface::ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "region";
  defaultScopeItems.exploreCurrentScopeItem() = true;

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;

  // Use container(5) which is NOT in the "region" scope
  // This should trigger the code path where scopeItemId.has_value() is false
  auto object1Destinations = destinationsGenerator.getAcceptingDestinations(
      moveToScopeItemsSpec, object(1), container(5));

  // Since container(5) is not in the region scope, it should explore all
  // scope items in the region (similar to the default behavior)
  EXPECT_EQ(object1Destinations.size(), 2);

  {
    auto& firstContainerSet = object1Destinations.at(0).get();
    const std::set<entities::ContainerId> expectedContainers = {
        container(1), container(4)};
    verifyContainersAreAsExpected(expectedContainers, firstContainerSet);
  }

  {
    auto& secondContainerSet = object1Destinations.at(1).get();
    const std::set<entities::ContainerId> expectedContainers = {container(2)};
    verifyContainersAreAsExpected(expectedContainers, secondContainerSet);
  }
}

CO_TEST_F(
    DestinationsToExploreGeneratorTest,
    getAcceptingDestinationsWithContainerIdOnlyThrowsWhenObjectToScopeItemsNotEmpty) {
  co_await setUpProblem(/*nonAcceptingContainerIndices=*/{3});
  auto& problem = getProblem();
  auto& destinationsGenerator = problem.getDestinationsGenerator();

  // Create a MoveToScopeItemsSpec with NON-empty objectToScopeItems
  interface::ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "region";

  interface::ScopeItemList objectScopeItems;
  objectScopeItems.scopeName() = "region";
  objectScopeItems.scopeItems() = {"region1"};

  interface::MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;
  moveToScopeItemsSpec.objectToScopeItems() = {{"object1", objectScopeItems}};

  // Call the method that takes only MoveToScopeItemsSpec and hotContainerId
  // This should throw an exception because objectToScopeItems is not empty
  EXPECT_THROW(
      destinationsGenerator.getAcceptingDestinations(
          moveToScopeItemsSpec, container(1)),
      std::runtime_error);
}

} // namespace facebook::rebalancer::packer::tests
