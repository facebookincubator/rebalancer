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

#include "algopt/rebalancer/entities/Partition.h"

#include "algopt/rebalancer/entities/GraphEmbeddingUtils.h"

#include <algorithm>

namespace facebook {
namespace rebalancer {
namespace entities {

namespace {
bool computeIsDisjoint(
    const Map<ObjectId, std::vector<GroupId>>& objectIdToGroupIds) {
  return std::none_of(
      objectIdToGroupIds.begin(),
      objectIdToGroupIds.end(),
      [](const auto& pair) { return pair.second.size() > 1; });
}
} // namespace

Partition::Partition(Map<GroupId, std::vector<ObjectId>> groups) {
  Map<ObjectId, std::vector<GroupId>> objectIdToGroupIds;
  for (auto& [groupId, objectIds] : groups) {
    groupIds_.push_back(groupId);

    for (auto objectId : objectIds) {
      objectIdToGroupIds[objectId].push_back(groupId);
    }
  }

  groups_ = std::move(groups);
  isDisjoint_ = computeIsDisjoint(objectIdToGroupIds);
  objectIdToGroupIds_ =
      std::make_shared<const Map<ObjectId, std::vector<GroupId>>>(
          std::move(objectIdToGroupIds));
}

Partition::Partition(const thrift::Partition& partition) {
  const auto& thriftGroups = *partition.groups();
  groupIds_.reserve(thriftGroups.size());
  groups_.reserve(thriftGroups.size());
  Map<ObjectId, std::vector<GroupId>> objectIdToGroupIds;
  for (const auto& [groupIdInt, objects] : thriftGroups) {
    auto groupId = GroupId(groupIdInt);

    std::vector<ObjectId> objectIds;
    objectIds.reserve(objects.size());
    for (auto objectIdInt : objects) {
      auto objectId = ObjectId(objectIdInt);
      objectIds.push_back(objectId);
      objectIdToGroupIds[objectId].push_back(groupId);
    }

    groups_.emplace(groupId, std::move(objectIds));
    groupIds_.push_back(groupId);
  }

  isDisjoint_ = computeIsDisjoint(objectIdToGroupIds);
  objectIdToGroupIds_ =
      std::make_shared<const Map<ObjectId, std::vector<GroupId>>>(
          std::move(objectIdToGroupIds));
}

const Map<GroupId, std::vector<ObjectId>>& Partition::getGroupToObjectIds()
    const {
  return groups_;
}

const std::vector<GroupId>& Partition::getGroupIds() const {
  return groupIds_;
}

const std::vector<ObjectId>& Partition::getObjectIds(GroupId groupId) const {
  return groups_.at(groupId);
}

const Map<ObjectId, std::vector<GroupId>>& Partition::getObjectIdToGroupIds()
    const {
  return *objectIdToGroupIds_;
}

const std::shared_ptr<const Map<ObjectId, std::vector<GroupId>>>&
Partition::getObjectIdToGroupIdsPtr() const {
  return objectIdToGroupIds_;
}

thrift::Partition Partition::toThrift() const {
  thrift::Partition partition;
  auto& thriftGroups = *partition.groups();
  thriftGroups.reserve(groups_.size());
  for (const auto& [groupId, objectIds] : groups_) {
    thriftGroups.emplace(groupId.asInt(), asIntsVec(objectIds));
  }
  return partition;
}

bool Partition::isDisjoint() const {
  return isDisjoint_;
}

std::vector<thrift::GraphVertex> Partition::toEmbedding() const {
  std::vector<thrift::GraphVertex> vertices;
  vertices.reserve(groupIds_.size());

  for (const auto groupId : groupIds_) {
    thrift::GraphVertex vertex;
    vertex.type() = thrift::VertexType::GROUP;
    vertex.entityId() = groupId.asInt();

    auto& embedding = *vertex.embedding();
    initOneHotEmbedding(embedding, thrift::VertexType::GROUP);

    vertices.push_back(std::move(vertex));
  }
  return vertices;
}

} // namespace entities
} // namespace rebalancer
} // namespace facebook
