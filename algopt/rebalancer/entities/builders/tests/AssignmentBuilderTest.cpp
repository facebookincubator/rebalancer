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
#include "algopt/rebalancer/entities/builders/AssignmentBuilder.h"
#include "algopt/rebalancer/entities/builders/tests/BuilderTestBase.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/portability/GTest.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::entities::tests {

class AssignmentBuilderTest : public BuilderTestBase {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    AssignmentBuilderTest,
    ::testing::Values(1));

TEST_P(AssignmentBuilderTest, Set) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
      {"c3", {}},
  };
  builder.set(assignment);

  const auto& objects = builder.getObjects();
  EXPECT_EQ(objects->numObjects, 3);
  EXPECT_EQ(objects->objectNameToId.size(), 3);

  const auto& containers = builder.getContainers();
  EXPECT_EQ(containers->containerIdToObjectIds.size(), 3);
  EXPECT_EQ(containers->containerNameToId.size(), 3);

  const auto c1Id = containers->getId("c1");
  const auto c2Id = containers->getId("c2");
  const auto c3Id = containers->getId("c3");
  const auto o1Id = objects->getId("o1");
  const auto o2Id = objects->getId("o2");
  const auto o3Id = objects->getId("o3");

  EXPECT_TRUE(containers->containerIdToObjectIds.contains(c1Id));
  EXPECT_TRUE(containers->containerIdToObjectIds.contains(c2Id));
  EXPECT_TRUE(containers->containerIdToObjectIds.contains(c3Id));

  const auto c1Objects = std::set<ObjectId>{o1Id, o2Id};
  const auto actualC1Objects =
      toSet<ObjectId>(containers->containerIdToObjectIds.at(c1Id));
  EXPECT_EQ(c1Objects, actualC1Objects);

  const auto c2Objects = std::set<ObjectId>{o3Id};
  const auto actualC2Objects =
      toSet<ObjectId>(containers->containerIdToObjectIds.at(c2Id));
  EXPECT_EQ(c2Objects, actualC2Objects);

  EXPECT_EQ(0, containers->containerIdToObjectIds.at(c3Id).size());
}

TEST_P(AssignmentBuilderTest, IdsAreUnique) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
  };
  builder.set(assignment);

  const auto& objects = builder.getObjects();
  const auto& containers = builder.getContainers();

  std::set<EntityIdType> objectIds;
  for (const auto& [_, id] : objects->objectNameToId) {
    objectIds.insert(id.asIndex());
  }
  EXPECT_EQ(objectIds.size(), 3);

  std::set<EntityIdType> containerIds;
  for (const auto& [_, id] : containers->containerNameToId) {
    containerIds.insert(id.asIndex());
  }
  EXPECT_EQ(containerIds.size(), 2);
}

CO_TEST_P(AssignmentBuilderTest, BuildCopiesDataToResult) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
  };
  builder.set(assignment);

  const auto result = co_await builder.build();

  EXPECT_EQ(result.numObjects, 3);
  EXPECT_EQ(result.objectNameToId.size(), 3);
  EXPECT_EQ(result.containerNameToId.size(), 2);
  EXPECT_EQ(result.containerIdToObjectIds.size(), 2);

  // getObjects() and getContainers() should still work after build()
  EXPECT_EQ(builder.getObjects()->numObjects, 3);
  EXPECT_EQ(builder.getContainers()->containerIdToObjectIds.size(), 2);
}

TEST_P(AssignmentBuilderTest, GetBeforeSetThrows) {
  AssignmentBuilder builder(idBuilder_);
  REBALANCER_EXPECT_RUNTIME_ERROR(
      std::ignore = builder.getObjects(),
      "Expect objects to be set. Did you set initialAssignment?");

  REBALANCER_EXPECT_RUNTIME_ERROR(
      std::ignore = builder.getContainers(),
      "Expect containers to be set. Did you set initialAssignment?");
}

TEST_P(AssignmentBuilderTest, SetWithDuplicateObjectThrows) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"container1", {"obj1", "obj2"}},
      {"container2", {"obj2", "obj3"}},
  };

  REBALANCER_EXPECT_RUNTIME_ERROR(
      builder.set(assignment), "Duplicate object 'obj2' in assignment");
}

CO_TEST_P(AssignmentBuilderTest, Summarize) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
      {"c3", {}},
  };
  builder.set(assignment);

  const auto result = co_await builder.summarize();

  const std::string expected =
      "  #Objects: 3\n"
      "  #Containers: 3\n";
  EXPECT_EQ(expected, result);
}

CO_TEST_P(AssignmentBuilderTest, BuildAndRebuild) {
  AssignmentBuilder builder(idBuilder_);
  Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
  };
  builder.set(assignment);

  const auto result1 = co_await builder.build();
  EXPECT_EQ(3, result1.numObjects);
  EXPECT_EQ(3, result1.objectNameToId.size());
  EXPECT_EQ(2, result1.containerNameToId.size());
  EXPECT_EQ(2, result1.containerIdToObjectIds.size());

  assignment = {
      {"c1", {"o1"}},
      {"c2", {"o2", "o3"}},
  };
  builder.set(assignment);

  const auto result2 = co_await builder.build();

  // Verify that result1 was not mutated by the second build
  EXPECT_EQ(3, result1.numObjects);
  EXPECT_EQ(3, result1.objectNameToId.size());
  EXPECT_EQ(2, result1.containerNameToId.size());
  EXPECT_EQ(2, result1.containerIdToObjectIds.size());

  // Verify that result2 is correct
  EXPECT_EQ(3, result2.numObjects);
  EXPECT_EQ(3, result2.objectNameToId.size());
  EXPECT_EQ(2, result2.containerNameToId.size());
  EXPECT_EQ(2, result2.containerIdToObjectIds.size());

  // Verify that IDs are preserved across rebuilds
  EXPECT_EQ(result1.objectNameToId, result2.objectNameToId);
  EXPECT_EQ(result1.containerNameToId, result2.containerNameToId);

  // Verify that result1 has the old assignment and result2 has the new one
  const auto c1Id = result1.containerNameToId.at("c1");
  const auto c2Id = result1.containerNameToId.at("c2");
  const auto o1Id = result1.objectNameToId.at("o1");
  const auto o2Id = result1.objectNameToId.at("o2");
  const auto o3Id = result1.objectNameToId.at("o3");

  const std::set<ObjectId> expectedC1Old{o1Id, o2Id};
  const std::set<ObjectId> expectedC2Old{o3Id};
  EXPECT_EQ(
      expectedC1Old, toSet<ObjectId>(result1.containerIdToObjectIds.at(c1Id)));
  EXPECT_EQ(
      expectedC2Old, toSet<ObjectId>(result1.containerIdToObjectIds.at(c2Id)));

  const std::set<ObjectId> expectedC1New{o1Id};
  const std::set<ObjectId> expectedC2New{o2Id, o3Id};
  EXPECT_EQ(
      expectedC1New, toSet<ObjectId>(result2.containerIdToObjectIds.at(c1Id)));
  EXPECT_EQ(
      expectedC2New, toSet<ObjectId>(result2.containerIdToObjectIds.at(c2Id)));
}

TEST_P(AssignmentBuilderTest, AddingNewObjectThrows) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
  };
  builder.set(assignment);

  const Map<std::string, std::vector<std::string>> moreObjects{
      {"c1", {"o1", "o4"}},
      {"c2", {"o3"}},
  };
  REBALANCER_EXPECT_RUNTIME_ERROR(
      builder.set(moreObjects),
      "AssignmentBuilder::set: re-set must use the same set of objects and containers as the initial call (unknown object 'o4')");
}

TEST_P(AssignmentBuilderTest, AddingNewContainerThrows) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
  };
  builder.set(assignment);

  const Map<std::string, std::vector<std::string>> moreContainers{
      {"c1", {"o1", "o2"}},
      {"c9", {"o3"}},
  };
  REBALANCER_EXPECT_RUNTIME_ERROR(
      builder.set(moreContainers),
      "AssignmentBuilder::set: re-set must use the same set of objects and containers as the initial call (unknown container 'c9')");
}

TEST_P(AssignmentBuilderTest, RemovingObjectThrows) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
  };
  builder.set(assignment);

  const Map<std::string, std::vector<std::string>> fewerObjects{
      {"c1", {"o1"}},
      {"c2", {"o3"}},
  };
  REBALANCER_EXPECT_RUNTIME_ERROR(
      builder.set(fewerObjects),
      "AssignmentBuilder::set: re-set must use the same set of objects and containers as the initial call (missing object 'o2')");
}

TEST_P(AssignmentBuilderTest, RemovingContainerThrows) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
  };
  builder.set(assignment);

  const Map<std::string, std::vector<std::string>> fewerContainers{
      {"c1", {"o1", "o2", "o3"}},
  };
  REBALANCER_EXPECT_RUNTIME_ERROR(
      builder.set(fewerContainers),
      "AssignmentBuilder::set: re-set must use the same set of objects and containers as the initial call (missing container 'c2')");
}

TEST_P(AssignmentBuilderTest, DuplicateObjectThrows) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
  };
  builder.set(assignment);

  const Map<std::string, std::vector<std::string>> duplicateObject{
      {"c1", {"o1", "o3"}},
      {"c2", {"o3"}},
  };
  REBALANCER_EXPECT_RUNTIME_ERROR(
      builder.set(duplicateObject), "Duplicate object 'o3' in assignment");
}

TEST_P(AssignmentBuilderTest, DuplicateContainerThrows) {
  AssignmentBuilder builder(idBuilder_);
  const Map<std::string, std::vector<std::string>> assignment{
      {"c1", {"o1", "o2"}},
      {"c2", {"o3"}},
  };
  builder.set(assignment);

  const std::vector<std::pair<std::string, std::vector<std::string>>>
      duplicateContainer{
          {"c1", {"o1", "o2"}},
          {"c1", {"o3"}},
      };
  REBALANCER_EXPECT_RUNTIME_ERROR(
      builder.set(duplicateContainer),
      "Duplicate container 'c1' in assignment");
}

} // namespace facebook::rebalancer::entities::tests
