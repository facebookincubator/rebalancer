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

#include "algopt/rebalancer/interface/ObjectCountExpander.h"

#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

namespace {

// Standard scenario: host0 gets taskA x2 + taskB x1, host1 gets taskA x1 +
// taskC x3.
ContainerAssignment makeStandardCompactAssignment() {
  ContainerAssignment compactAssignment;
  auto& objectCounts = *compactAssignment.objectsPerContainer();
  objectCounts["host0"]["taskA"] = 2;
  objectCounts["host0"]["taskB"] = 1;
  objectCounts["host1"]["taskA"] = 1;
  objectCounts["host1"]["taskC"] = 3;
  return compactAssignment;
}

} // namespace

// -- constructor / expandAssignment --

TEST(ObjectCountExpanderTest, ExpandsAcrossContainersWithSharedObjects) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());
  const auto& expanded = expander.getExpandedAssignment();

  EXPECT_EQ(expanded.at("host0").size(), 3);
  EXPECT_EQ(expanded.at("host1").size(), 4);

  const std::vector<std::string> expectedTaskA{
      "taskA__0", "taskA__1", "taskA__2"};
  EXPECT_EQ(expander.getObjectToSyntheticNames().at("taskA"), expectedTaskA);

  for (const auto& [objectName, synthetics] :
       expander.getObjectToSyntheticNames()) {
    for (const auto& syntheticName : synthetics) {
      EXPECT_EQ(expander.getSyntheticToObject().at(syntheticName), objectName);
    }
  }
}

TEST(ObjectCountExpanderTest, ThrowsOnZeroCount) {
  ContainerAssignment compactAssignment;
  (*compactAssignment.objectsPerContainer())["host0"]["taskA"] = 0;
  EXPECT_THROW(ObjectCountExpander{compactAssignment}, std::invalid_argument);
}

TEST(ObjectCountExpanderTest, ThrowsOnNegativeCount) {
  ContainerAssignment compactAssignment;
  (*compactAssignment.objectsPerContainer())["host0"]["taskA"] = -1;
  EXPECT_THROW(ObjectCountExpander{compactAssignment}, std::invalid_argument);
}

// -- expandAvoidMovingSpec --

TEST(ObjectCountExpanderTest, ExpandAvoidMovingSpecExpandsKnownObjects) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  AvoidMovingSpec spec;
  spec.name() = "avoid-spec";
  spec.objects() = {"taskA"};

  const auto expanded = expander.expandAvoidMovingSpec(spec);

  EXPECT_EQ(*expanded.name(), "avoid-spec");
  EXPECT_EQ(expanded.objects()->size(), 3);
}

TEST(ObjectCountExpanderTest, ExpandAvoidMovingSpecThrowsOnUnknownObject) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  AvoidMovingSpec spec;
  spec.name() = "avoid-spec";
  spec.objects() = {"taskA", "unknownTask"};

  EXPECT_THROW(expander.expandAvoidMovingSpec(spec), std::invalid_argument);
}

// -- expandMovesInProgressSpec --

TEST(ObjectCountExpanderTest, ExpandMovesInProgressSpecExpandsKnownObjects) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  MovesInProgressSpec spec;
  spec.name() = "moves-spec";

  MoveInProgress knownMove;
  knownMove.objName() = "taskA";
  knownMove.toContainer() = "host2";

  spec.moves() = {knownMove};

  const auto expanded = expander.expandMovesInProgressSpec(spec);

  EXPECT_EQ(*expanded.name(), "moves-spec");
  ASSERT_EQ(expanded.moves()->size(), 3);
  for (const auto& m : *expanded.moves()) {
    EXPECT_TRUE(m.objName()->starts_with("taskA__"));
    EXPECT_EQ(*m.toContainer(), "host2");
  }
}

TEST(ObjectCountExpanderTest, ExpandMovesInProgressSpecThrowsOnUnknownObject) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  MovesInProgressSpec spec;
  spec.name() = "moves-spec";

  MoveInProgress unknownMove;
  unknownMove.objName() = "unknownTask";
  unknownMove.toContainer() = "host3";

  spec.moves() = {unknownMove};

  EXPECT_THROW(expander.expandMovesInProgressSpec(spec), std::invalid_argument);
}

TEST(ObjectCountExpanderTest, MaybeExpandSpecDispatchesToSupportedSpecLogic) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  MovesInProgressSpec spec;
  spec.name() = "moves-spec";

  MoveInProgress move;
  move.objName() = "taskA";
  move.toContainer() = "host2";
  spec.moves() = {move};

  const auto expanded = expander.maybeExpandSpec(spec);

  EXPECT_EQ(*expanded.name(), "moves-spec");
  ASSERT_EQ(expanded.moves()->size(), 3);
  for (const auto& expandedMove : *expanded.moves()) {
    EXPECT_TRUE(expandedMove.objName()->starts_with("taskA__"));
    EXPECT_EQ(*expandedMove.toContainer(), "host2");
  }
}

TEST(ObjectCountExpanderTest, MaybeExpandSpecThrowsForUnsupportedFlowSpec) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  FlowSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";

  ObjectPair pair;
  pair.object1() = "taskA";
  pair.object2() = "taskB";
  spec.pairs() = {pair};

  EXPECT_THROW(expander.maybeExpandSpec(spec), std::invalid_argument);
}

// -- compressSolution --

TEST(ObjectCountExpanderTest, RoundTripWithMovementPreservesCounts) {
  ContainerAssignment inputCompactAssignment;
  auto& objectCounts = *inputCompactAssignment.objectsPerContainer();
  objectCounts["host0"]["taskA"] = 3;
  objectCounts["host0"]["taskB"] = 1;

  const ObjectCountExpander expander(inputCompactAssignment);
  const auto& expanded = expander.getExpandedAssignment();

  // Build initial assignment: all objects in host0
  folly::F14FastMap<std::string, std::string> initialAssignment;
  for (const auto& [container, objects] : expanded) {
    for (const auto& obj : objects) {
      initialAssignment[obj] = container;
    }
  }

  // Move one taskA synthetic from host0 to host1
  auto assignment = initialAssignment;
  const auto& taskASynthetics =
      expander.getObjectToSyntheticNames().at("taskA");
  assignment[taskASynthetics[0]] = "host1";

  AssignmentSolution solution;
  solution.assignment() = std::move(assignment);
  solution.initialAssignment() = std::move(initialAssignment);

  expander.compressSolution(solution);

  const auto& finalCompactAssignment =
      *solution.compactAssignment()->objectsPerContainer();
  EXPECT_EQ(finalCompactAssignment.at("host0").at("taskA"), 2);
  EXPECT_EQ(finalCompactAssignment.at("host0").at("taskB"), 1);
  EXPECT_EQ(finalCompactAssignment.at("host1").at("taskA"), 1);

  const auto& initialCompactAssignment =
      *solution.compactAssignmentInitial()->objectsPerContainer();
  EXPECT_EQ(initialCompactAssignment.at("host0").at("taskA"), 3);
  EXPECT_EQ(initialCompactAssignment.at("host0").at("taskB"), 1);

  // Legacy per-object fields should be cleared.
  EXPECT_TRUE(solution.assignment()->empty());
  EXPECT_TRUE(solution.initialAssignment()->empty());
}

TEST(ObjectCountExpanderTest, CompressThrowsOnUnknownSyntheticName) {
  ContainerAssignment compactAssignment;
  (*compactAssignment.objectsPerContainer())["host0"]["taskA"] = 1;

  const ObjectCountExpander expander(compactAssignment);

  AssignmentSolution solution;
  solution.assignment() =
      folly::F14FastMap<std::string, std::string>{{"bogus_name", "host0"}};
  solution.initialAssignment() =
      folly::F14FastMap<std::string, std::string>{{"taskA__0", "host0"}};

  EXPECT_THROW(expander.compressSolution(solution), std::runtime_error);
}

// -- expandObjectMap --

TEST(ObjectCountExpanderTest, ExpandObjectMapExpandsDimension) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  const folly::F14FastMap<std::string, double> objectToCpu{
      {"taskA", 10.0}, {"taskB", 20.0}, {"taskC", 30.0}};
  const auto expanded = expander.expandObjectMap(objectToCpu);

  // taskA has 3 synthetics, taskB has 1, taskC has 3 = 7 total
  EXPECT_EQ(expanded.size(), 7);
  EXPECT_EQ(expanded.at("taskA__0"), 10.0);
  EXPECT_EQ(expanded.at("taskA__2"), 10.0);
  EXPECT_EQ(expanded.at("taskB__0"), 20.0);
  EXPECT_EQ(expanded.at("taskC__0"), 30.0);
  EXPECT_EQ(expanded.at("taskC__2"), 30.0);
}

TEST(ObjectCountExpanderTest, ExpandObjectMapExpandsPartition) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  const folly::F14FastMap<std::string, std::string> objectToGroup{
      {"taskA", "web"}, {"taskC", "cache"}};
  const auto expanded = expander.expandObjectMap(objectToGroup);

  // taskA has 3 synthetics, taskC has 3 = 6 total
  EXPECT_EQ(expanded.size(), 6);
  EXPECT_EQ(expanded.at("taskA__0"), "web");
  EXPECT_EQ(expanded.at("taskA__2"), "web");
  EXPECT_EQ(expanded.at("taskC__1"), "cache");
}

TEST(ObjectCountExpanderTest, ExpandObjectMapExpandsVectorDimension) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  const folly::F14FastMap<std::string, std::vector<double>> objectToValues{
      {"taskA", {10.0, 20.0}}, {"taskB", {5.0}}};
  const auto expanded = expander.expandObjectMap(objectToValues);

  // taskA has 3 synthetics, taskB has 1 = 4 total
  EXPECT_EQ(expanded.size(), 4);
  const std::vector<double> expectedTaskA{10.0, 20.0};
  EXPECT_EQ(expanded.at("taskA__0"), expectedTaskA);
  EXPECT_EQ(expanded.at("taskA__2"), expectedTaskA);
  const std::vector<double> expectedTaskB{5.0};
  EXPECT_EQ(expanded.at("taskB__0"), expectedTaskB);
}

TEST(ObjectCountExpanderTest, ExpandObjectMapThrowsOnUnknownObject) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  const folly::F14FastMap<std::string, double> objectToCpu{
      {"taskA", 10.0}, {"unknownTask", 20.0}};
  EXPECT_THROW(expander.expandObjectMap(objectToCpu), std::invalid_argument);
}

TEST(ObjectCountExpanderTest, ExpandObjectMapAcceptsStdMap) {
  const ObjectCountExpander expander(makeStandardCompactAssignment());

  const std::map<std::string, double> objectToCpu{
      {"taskA", 10.0}, {"taskB", 20.0}};
  const auto expanded = expander.expandObjectMap(objectToCpu);

  EXPECT_EQ(expanded.size(), 4);
  EXPECT_EQ(expanded.at("taskA__0"), 10.0);
  EXPECT_EQ(expanded.at("taskA__2"), 10.0);
  EXPECT_EQ(expanded.at("taskB__0"), 20.0);
}
