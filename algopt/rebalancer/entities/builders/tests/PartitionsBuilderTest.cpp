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
#include "algopt/rebalancer/entities/builders/PartitionsBuilder.h"
#include "algopt/rebalancer/entities/builders/tests/BuilderTestBase.h"

#include <folly/container/irange.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/portability/GTest.h>

namespace facebook::rebalancer::entities::tests {

class PartitionsBuilderTest : public BuilderTestBase {
 protected:
  void SetUp() override {
    BuilderTestBase::SetUp();
    assignmentBuilder_ = std::make_unique<AssignmentBuilder>(idBuilder_);
    const Map<std::string, std::vector<std::string>> assignment{
        {"c1", {"o1", "o2"}},
        {"c2", {"o3"}},
        {"c3", {}},
    };
    assignmentBuilder_->set(assignment);
  }

  std::unique_ptr<AssignmentBuilder> assignmentBuilder_;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    PartitionsBuilderTest,
    ::testing::Values(1, 3, 20));

CO_TEST_P(PartitionsBuilderTest, AddGroupNameToObjectNames) {
  PartitionsBuilder partitionsBuilder(idBuilder_);
  const auto objects = assignmentBuilder_->getObjects();

  const Map<std::string, std::vector<std::string>> partition1{
      {"group1_1", {"o1", "o2"}},
      {"group2_1", {"o3"}},
      {"group3_1", {}},
  };
  const Map<std::string, std::vector<std::string>> partition2{
      {"group1_2", {"o1"}},
      {"group2_2", {"o2", "o3"}},
  };

  const auto partition1Id = partitionsBuilder.makePartitionId("partition1");
  const auto partition2Id = partitionsBuilder.makePartitionId("partition2");
  co_await partitionsBuilder.add(partition1Id, partition1, objects);
  co_await partitionsBuilder.add(partition2Id, partition2, objects);

  const auto group1Data = co_await partitionsBuilder.get(partition1Id);
  EXPECT_EQ(3, group1Data->groupNameToId->size());
  const auto group11Id = group1Data->getGroupId("group1_1");
  const auto& group11Objects = group1Data->partition->getObjectIds(group11Id);
  EXPECT_EQ(
      toSet<ObjectId>(group11Objects),
      (std::set<ObjectId>{objects->getId("o1"), objects->getId("o2")}));

  const auto group21Id = group1Data->getGroupId("group2_1");
  const auto& group21Objects = group1Data->partition->getObjectIds(group21Id);
  EXPECT_EQ(std::vector<ObjectId>{objects->getId("o3")}, group21Objects);

  const auto group31Id = group1Data->getGroupId("group3_1");
  const auto& group31Objects = group1Data->partition->getObjectIds(group31Id);
  EXPECT_TRUE(group31Objects.empty());

  const auto partition2Data = co_await partitionsBuilder.get(partition2Id);
  EXPECT_EQ(2, partition2Data->groupNameToId->size());
  const auto group12Id = partition2Data->getGroupId("group1_2");
  const auto& group12Objects =
      partition2Data->partition->getObjectIds(group12Id);
  EXPECT_EQ(std::vector<ObjectId>{objects->getId("o1")}, group12Objects);

  const auto group22Id = partition2Data->getGroupId("group2_2");
  const auto& group22Objects =
      partition2Data->partition->getObjectIds(group22Id);
  EXPECT_EQ(
      toSet<ObjectId>(group22Objects),
      (std::set<ObjectId>{objects->getId("o2"), objects->getId("o3")}));

  const auto result = co_await partitionsBuilder.build();
  EXPECT_EQ(2, result.partitionNameToId.size());
  EXPECT_EQ(partition1Id, result.partitionNameToId.at("partition1"));
  EXPECT_EQ(partition2Id, result.partitionNameToId.at("partition2"));

  EXPECT_EQ(2, result.partitionIdToGroupNameToId.size());
  const auto& partition1NameToId =
      *result.partitionIdToGroupNameToId.at(partition1Id);
  EXPECT_EQ(3, partition1NameToId.size());
  EXPECT_TRUE(partition1NameToId.contains("group1_1"));
  EXPECT_TRUE(partition1NameToId.contains("group2_1"));
  EXPECT_TRUE(partition1NameToId.contains("group3_1"));

  EXPECT_EQ(2, result.partitionIdToPartition.size());
  const auto& partition1Result = result.partitionIdToPartition.at(partition1Id);
  EXPECT_EQ(3, partition1Result->getGroupIds().size());
  EXPECT_EQ(3, partition1Result->getGroupToObjectIds().size());
}

CO_TEST_P(PartitionsBuilderTest, AddObjectNameToGroupName) {
  PartitionsBuilder partitionsBuilder(idBuilder_);
  const auto objects = assignmentBuilder_->getObjects();

  const auto partitionId = partitionsBuilder.makePartitionId("region");

  const Map<std::string, std::string> objectToRegion{
      {"o1", "us-east"},
      {"o2", "us-east"},
      {"o3", "us-west"},
  };

  co_await partitionsBuilder.add(partitionId, objectToRegion, objects);

  const auto partitionData = co_await partitionsBuilder.get(partitionId);

  EXPECT_EQ(2, partitionData->groupNameToId->size());
  const auto usEastId = partitionData->getGroupId("us-east");
  const auto& usEastObjects = partitionData->partition->getObjectIds(usEastId);
  EXPECT_EQ(
      toSet<ObjectId>(usEastObjects),
      (std::set<ObjectId>{objects->getId("o1"), objects->getId("o2")}));

  const auto usWestId = partitionData->getGroupId("us-west");
  const auto& usWestObjects = partitionData->partition->getObjectIds(usWestId);
  EXPECT_EQ(std::vector<ObjectId>{objects->getId("o3")}, usWestObjects);

  EXPECT_EQ(2, partitionData->partition->getGroupToObjectIds().size());
  EXPECT_EQ(2, partitionData->partition->getGroupIds().size());
}

CO_TEST_P(PartitionsBuilderTest, ConcurrentAddAndGet) {
  PartitionsBuilder partitionsBuilder(idBuilder_);

  AssignmentBuilder customAssignmentBuilder(idBuilder_);
  customAssignmentBuilder.set(generateInitialAssignment(
      ProblemConfig{.numObjects = 10'000, .numContainers = 333}));
  const auto objects = customAssignmentBuilder.getObjects();

  constexpr int nPartitions = 500;
  std::vector<PartitionId> partitionIds;
  partitionIds.reserve(nPartitions);
  for (const auto i : folly::irange(nPartitions)) {
    partitionIds.push_back(
        partitionsBuilder.makePartitionId(fmt::format("partition_{}", i)));
  }

  constexpr int nGroups = 71;
  const auto groupNameToObjectNames =
      generateGroupNameToObjectNames(nGroups, *objects);
  const auto objectNameToGroupName =
      generateObjectNameToGroupName(nGroups, *objects);

  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve(nPartitions * 2);
  for (const auto i : folly::irange(nPartitions)) {
    if (i % 2 == 0) {
      tasks.push_back(partitionsBuilder.add(
          partitionIds[i], groupNameToObjectNames, objects));
    } else {
      tasks.push_back(partitionsBuilder.add(
          partitionIds[i], objectNameToGroupName, objects));
    }

    tasks.push_back(
        folly::coro::co_invoke(
            [&partitionsBuilder, nGroups, i, partitionId = partitionIds[i]]()
                -> folly::coro::Task<void> {
              executeRandomDelay();
              const auto data = co_await partitionsBuilder.get(partitionId);
              EXPECT_EQ(nGroups, data->groupNameToId->size());
              EXPECT_EQ(nGroups, data->partition->getGroupIds().size());
              executeRandomDelay();
              EXPECT_EQ(
                  partitionId,
                  partitionsBuilder.getPartitionId(
                      fmt::format("partition_{}", i)));
            }));
  }

  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  const auto result = co_await partitionsBuilder.build();

  EXPECT_EQ(nPartitions, result.partitionNameToId.size());
  for (const auto& [partitionId, groupNameToId] :
       result.partitionIdToGroupNameToId) {
    EXPECT_EQ(nGroups, groupNameToId->size());
  }
  for (const auto& [partitionId, partition] : result.partitionIdToPartition) {
    EXPECT_EQ(nGroups, partition->getGroupIds().size());
    EXPECT_EQ(nGroups, partition->getGroupToObjectIds().size());
  }
}

TEST_P(PartitionsBuilderTest, DuplicateMakePartitionIdThrows) {
  PartitionsBuilder partitionsBuilder(idBuilder_);
  std::ignore = partitionsBuilder.makePartitionId("job");
  REBALANCER_EXPECT_RUNTIME_ERROR(
      partitionsBuilder.makePartitionId("job"),
      "Unexpected call to makePartitionId with a previously added name 'job'");
}

CO_TEST_P(PartitionsBuilderTest, Summarize) {
  PartitionsBuilder builder(idBuilder_);
  const auto objects = assignmentBuilder_->getObjects();

  const auto partitionId = builder.makePartitionId("job_partition");
  const Map<std::string, std::vector<std::string>> partition{
      {"job-A", {"o1", "o2"}},
      {"job-B", {"o3"}},
  };
  co_await builder.add(partitionId, partition, objects);

  const auto result = co_await builder.summarize();

  const std::string expected =
      "Partitions:\n"
      "  job_partition [ job-A job-B ]\n";
  EXPECT_EQ(expected, result);
}

CO_TEST_P(PartitionsBuilderTest, BuildAddPartitionAndRebuild) {
  PartitionsBuilder partitionsBuilder(idBuilder_);
  const auto objects = assignmentBuilder_->getObjects();

  const auto partition1Id = partitionsBuilder.makePartitionId("partition1");
  const Map<std::string, std::vector<std::string>> partition1{
      {"group1", {"o1", "o2"}},
  };
  co_await partitionsBuilder.add(partition1Id, partition1, objects);

  const auto result1 = co_await partitionsBuilder.build();
  EXPECT_EQ(1, result1.partitionNameToId.size());
  EXPECT_EQ(partition1Id, result1.partitionNameToId.at("partition1"));
  EXPECT_EQ(1, result1.partitionIdToPartition.size());
  EXPECT_EQ(1, result1.partitionIdToGroupNameToId.size());
  EXPECT_EQ(1, result1.partitionIdToGroupNameToId.at(partition1Id)->size());

  const auto partition2Id = partitionsBuilder.makePartitionId("partition2");
  const Map<std::string, std::vector<std::string>> partition2{
      {"groupA", {"o1"}},
      {"groupB", {"o3"}},
  };
  co_await partitionsBuilder.add(partition2Id, partition2, objects);

  const auto result2 = co_await partitionsBuilder.build();

  // Verify that result1 was not mutated by the second build
  EXPECT_EQ(1, result1.partitionNameToId.size());
  EXPECT_EQ(partition1Id, result1.partitionNameToId.at("partition1"));
  EXPECT_EQ(1, result1.partitionIdToPartition.size());
  EXPECT_EQ(1, result1.partitionIdToGroupNameToId.size());
  EXPECT_EQ(1, result1.partitionIdToGroupNameToId.at(partition1Id)->size());

  EXPECT_EQ(2, result2.partitionNameToId.size());
  EXPECT_EQ(partition2Id, result2.partitionNameToId.at("partition2"));
  EXPECT_EQ(2, result2.partitionIdToPartition.size());
  EXPECT_EQ(2, result2.partitionIdToGroupNameToId.size());
  EXPECT_EQ(2, result2.partitionIdToGroupNameToId.at(partition2Id)->size());
}

} // namespace facebook::rebalancer::entities::tests
