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

#include "algopt/rebalancer/entities/GraphEmbeddingUtils.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Partition.h"
#include "algopt/rebalancer/entities/tests/Utils.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace facebook::rebalancer::entities::tests {

TEST(PartitionTest, ToThrift) {
  const Partition partition({{GroupId(0), {ObjectId(1)}}});

  auto thrift = partition.toThrift();

  const std::map<int, std::vector<int>> expectedGroups({{0, {1}}});
  EXPECT_EQ(expectedGroups, toStdMap(*thrift.groups()));
  EXPECT_TRUE(partition.isDisjoint());
}

TEST(PartitionTest, FromThrift) {
  thrift::Partition thrift;
  thrift.groups() = {{0, {1, 2}}, {1, {1, 4}}};

  const Partition partition(thrift);

  auto& groupIds = partition.getGroupIds();
  EXPECT_EQ(
      std::set<GroupId>({GroupId(0), GroupId(1)}),
      std::set<GroupId>(groupIds.begin(), groupIds.end()));

  auto& objectIds = partition.getObjectIds(GroupId(0));
  EXPECT_EQ(
      std::set<ObjectId>({ObjectId(1), ObjectId(2)}),
      std::set<ObjectId>(objectIds.begin(), objectIds.end()));

  auto& computedObjectToGroupIds = partition.getObjectIdToGroupIds();
  Map<ObjectId, std::set<GroupId>> expectedObjectToGroupIds = {
      {ObjectId(1), {GroupId(0), GroupId(1)}},
      {ObjectId(2), {GroupId(0)}},
      {ObjectId(4), {GroupId(1)}},
  };
  for (auto& [objectId, computedGroupIds] : computedObjectToGroupIds) {
    EXPECT_EQ(
        expectedObjectToGroupIds.at(objectId),
        std::set<GroupId>(computedGroupIds.begin(), computedGroupIds.end()));
  }
  EXPECT_FALSE(partition.isDisjoint());
}

TEST(PartitionTest, ToEmbedding) {
  const Partition partition(
      {{GroupId(0), {ObjectId(1), ObjectId(2)}}, {GroupId(1), {ObjectId(3)}}});

  auto vertices = partition.toEmbedding();
  std::sort(
      vertices.begin(),
      vertices.end(),
      [](const thrift::GraphVertex& a, const thrift::GraphVertex& b) {
        return *a.entityId() < *b.entityId();
      });

  std::vector<double> groupOneHot(kNumVertexTypes, 0.0);
  groupOneHot[static_cast<std::size_t>(thrift::VertexType::GROUP)] = 1.0;

  thrift::GraphVertex expectedVertex0;
  expectedVertex0.type() = thrift::VertexType::GROUP;
  expectedVertex0.entityId() = 0;
  expectedVertex0.embedding() = groupOneHot;

  thrift::GraphVertex expectedVertex1;
  expectedVertex1.type() = thrift::VertexType::GROUP;
  expectedVertex1.entityId() = 1;
  expectedVertex1.embedding() = groupOneHot;

  const std::vector<thrift::GraphVertex> expected = {
      expectedVertex0, expectedVertex1};
  EXPECT_EQ(expected, vertices);
}

} // namespace facebook::rebalancer::entities::tests
