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

#pragma once

#include "algopt/rebalancer/entities/builders/AssignmentBuilder.h"
#include "algopt/rebalancer/entities/builders/AsyncValueMap.h"
#include "algopt/rebalancer/entities/builders/IdBuilder.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/Partition.h"

#include <folly/container/F14Map.h>
#include <folly/coro/Task.h>
#include <folly/futures/SharedPromise.h>

namespace facebook::rebalancer::entities {

struct PartitionData {
  GroupId getGroupId(const std::string& groupName) const {
    return groupNameToId->at(groupName);
  }

  std::shared_ptr<const Map<std::string, GroupId>> groupNameToId;
  std::shared_ptr<Partition> partition;
};

struct PartitionsBuilderResult {
  Map<std::string, PartitionId> partitionNameToId;
  Map<PartitionId, std::shared_ptr<const Map<std::string, GroupId>>>
      partitionIdToGroupNameToId;
  Map<PartitionId, std::shared_ptr<Partition>> partitionIdToPartition;
};

class PartitionsBuilder {
 public:
  explicit PartitionsBuilder(IdBuilder& idBuilder) : idBuilder_(idBuilder) {}

  template <typename GroupToObjects>
  folly::coro::Task<void> add(
      PartitionId partitionId,
      GroupToObjects groupNameToObjectNames,
      std::shared_ptr<const ObjectsData> objects)
    requires IsIterableOverPairs<
        GroupToObjects,
        std::string,
        std::vector<std::string>>;

  template <typename ObjectToGroup>
  folly::coro::Task<void> add(
      PartitionId partitionId,
      ObjectToGroup objectNameToGroupName,
      std::shared_ptr<const ObjectsData> objects)
    requires IsIterableOverPairs<ObjectToGroup, std::string, std::string>;

  folly::coro::Task<std::shared_ptr<const PartitionData>> get(
      PartitionId partitionId) const {
    co_return co_await partitionIdToData_.get(partitionId);
  }

  PartitionId makePartitionId(const std::string& partitionName);

  PartitionId getPartitionId(const std::string& partitionName) const {
    return partitionNameToId_.rlock()->at(partitionName);
  }

  folly::coro::Task<PartitionsBuilderResult> build() const;

  folly::coro::Task<std::string> summarize() const;

 private:
  IdBuilder& idBuilder_;
  AsyncValueMap<PartitionId, PartitionData> partitionIdToData_;
  folly::Synchronized<Map<std::string, PartitionId>> partitionNameToId_;
};

// implementations
template <typename GroupToObjects>
folly::coro::Task<void> PartitionsBuilder::add(
    PartitionId partitionId,
    GroupToObjects groupNameToObjectNames,
    std::shared_ptr<const ObjectsData> objects)
  requires IsIterableOverPairs<
      GroupToObjects,
      std::string,
      std::vector<std::string>>
{
  const auto nGroups = groupNameToObjectNames.size();
  Map<std::string, GroupId> groupNameToId;
  Map<GroupId, std::vector<ObjectId>> groupIdToObjectIds;
  groupNameToId.reserve(nGroups);
  groupIdToObjectIds.reserve(nGroups);
  for (const auto& [groupName, objectNames] : groupNameToObjectNames) {
    const auto groupId = idBuilder_.next<GroupId>();
    groupNameToId.emplace(groupName, groupId);
    std::vector<ObjectId> objectIds;
    objectIds.reserve(objectNames.size());
    for (const auto& objectName : objectNames) {
      objectIds.push_back(objects->getId(objectName));
    }
    groupIdToObjectIds.emplace(groupId, std::move(objectIds));
  }

  partitionIdToData_.set(
      partitionId,
      PartitionData{
          .groupNameToId = std::make_shared<const Map<std::string, GroupId>>(
              std::move(groupNameToId)),
          .partition =
              std::make_shared<Partition>(std::move(groupIdToObjectIds)),
      });

  co_return;
}

template <typename ObjectToGroup>
folly::coro::Task<void> PartitionsBuilder::add(
    PartitionId partitionId,
    ObjectToGroup objectNameToGroupName,
    std::shared_ptr<const ObjectsData> objects)
  requires IsIterableOverPairs<ObjectToGroup, std::string, std::string>
{
  Map<std::string, GroupId> groupNameToId;
  Map<GroupId, std::vector<ObjectId>> groupIdToObjectIds;
  for (auto&& [objectName, groupName] : objectNameToGroupName) {
    const auto objectId = objects->getId(objectName);
    const auto groupIdPtr = folly::get_ptr(groupNameToId, groupName);
    if (!groupIdPtr) {
      const auto groupId = idBuilder_.next<GroupId>();
      groupNameToId.emplace(std::move(groupName), groupId);
      groupIdToObjectIds[groupId].push_back(objectId);
    } else {
      groupIdToObjectIds[*groupIdPtr].push_back(objectId);
    }
  }
  partitionIdToData_.set(
      partitionId,
      PartitionData{
          .groupNameToId = std::make_shared<const Map<std::string, GroupId>>(
              std::move(groupNameToId)),
          .partition =
              std::make_shared<Partition>(std::move(groupIdToObjectIds)),
      });

  co_return;
}

} // namespace facebook::rebalancer::entities
