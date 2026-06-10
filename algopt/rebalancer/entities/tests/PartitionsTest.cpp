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
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Partitions.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::entities {

TEST(PartitionsTest, Constructor) {
  const auto partition1Id = PartitionId(1);
  const auto partition2Id = PartitionId(2);
  const auto group1Id = GroupId(10);
  const auto group2Id = GroupId(11);
  const auto object1Id = ObjectId(100);
  const auto object2Id = ObjectId(101);

  auto partition1 = std::make_shared<Partition>(
      Map<GroupId, std::vector<ObjectId>>{{group1Id, {object1Id, object2Id}}});
  auto partition2 = std::make_shared<Partition>(
      Map<GroupId, std::vector<ObjectId>>{{group2Id, {object1Id}}});

  Map<PartitionId, std::shared_ptr<Partition>> partitionIdToPartition;
  partitionIdToPartition.emplace(partition1Id, std::move(partition1));
  partitionIdToPartition.emplace(partition2Id, std::move(partition2));

  const auto partitions = Partitions(std::move(partitionIdToPartition));

  EXPECT_EQ(2, partitions.getPartitionIds().size());

  const auto& p1 = partitions.getPartition(partition1Id);
  EXPECT_EQ(std::set<GroupId>({group1Id}), toSet<GroupId>(p1.getGroupIds()));
  EXPECT_EQ(
      std::vector<ObjectId>({object1Id, object2Id}), p1.getObjectIds(group1Id));

  const auto& p2 = partitions.getPartition(partition2Id);
  EXPECT_EQ(std::set<GroupId>({group2Id}), toSet<GroupId>(p2.getGroupIds()));
  EXPECT_EQ(std::vector<ObjectId>({object1Id}), p2.getObjectIds(group2Id));
}

TEST(PartitionsTest, ToThrift) {
  const auto partitionId = PartitionId(0);
  const auto groupId = GroupId(1);
  const auto objectId = ObjectId(2);

  auto partition = std::make_shared<Partition>(
      Map<GroupId, std::vector<ObjectId>>{{groupId, {objectId}}});

  Map<PartitionId, std::shared_ptr<Partition>> partitionIdToPartition;
  partitionIdToPartition.emplace(partitionId, std::move(partition));

  const Partitions partitions(std::move(partitionIdToPartition));

  const auto thrift = partitions.toThrift();
  EXPECT_EQ(1, thrift.partitions()->size());
  EXPECT_EQ(1, thrift.partitions()->at(0).groups()->size());
}

TEST(PartitionsTest, FromThrift) {
  thrift::Partition partitionThrift;
  partitionThrift.groups() = {{1, {2}}};

  thrift::Partitions partitionsThrift;
  partitionsThrift.partitions() = {{0, partitionThrift}};

  const Partitions partitions(partitionsThrift);

  EXPECT_EQ(
      std::vector<PartitionId>({PartitionId(0)}),
      toVec<PartitionId>(partitions.getPartitionIds()));

  auto& partition = partitions.getPartition(PartitionId(0));
  EXPECT_EQ(
      std::vector<GroupId>({GroupId(1)}),
      toVec<GroupId>(partition.getGroupIds()));
}
} // namespace facebook::rebalancer::entities
