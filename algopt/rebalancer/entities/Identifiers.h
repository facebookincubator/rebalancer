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

#include "algopt/rebalancer/algopt_common/CompressedIdMap.h"
#include "algopt/rebalancer/algopt_common/Utils.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"

#include <fmt/core.h>

#include <cstdint>
#include <string>
#include <vector>

namespace facebook::rebalancer::entities {

using EntityIdType = std::uint32_t;

template <typename Tag>
class EntityId {
 public:
  explicit constexpr EntityId(EntityIdType id) : id_(id) {}

  explicit constexpr operator EntityIdType() const noexcept {
    return id_;
  }

  explicit constexpr operator std::size_t() const noexcept {
    return static_cast<std::size_t>(id_);
  }

  constexpr bool operator==(const EntityId& other) const {
    return id_ == other.id_;
  }

  constexpr bool operator!=(const EntityId& other) const {
    return id_ != other.id_;
  }

  constexpr bool operator<(const EntityId& other) const {
    return id_ < other.id_;
  }

  constexpr bool operator>(const EntityId& other) const {
    return id_ > other.id_;
  }

  constexpr bool operator<=(const EntityId& other) const {
    return id_ <= other.id_;
  }

  constexpr bool operator>=(const EntityId& other) const {
    return id_ >= other.id_;
  }

  friend inline std::ostream& operator<<(std::ostream& os, const EntityId& id) {
    os << id.id_;
    return os;
  }

  constexpr int asInt() const {
    return static_cast<int>(id_);
  }

  constexpr EntityIdType asIndex() const {
    return id_;
  }

 private:
  EntityIdType id_;
};

template <typename>
struct is_entity_id : std::false_type {};

template <typename Tag>
struct is_entity_id<EntityId<Tag>> : std::true_type {};

using ObjectId = EntityId<struct ObjectTag>;
using ContainerId = EntityId<struct ContainerTag>;
using ScopeId = EntityId<struct ScopeTag>;
using ScopeItemId = EntityId<struct ScopeItemTag>;
using PartitionId = EntityId<struct PartitionTag>;
using GroupId = EntityId<struct GroupTag>;
using DimensionId = EntityId<struct DimensionTag>;
using GoalId = EntityId<struct GoalTag>;
using ConstraintId = EntityId<struct ConstraintTag>;
using RoutingConfigId = EntityId<struct RoutingConfigTag>;
using EquivalenceSetId = EntityId<struct EquivalenceSetTag>;

using ObjectIdToDoubleMap = facebook::algopt::CompressedIdMap<ObjectId, double>;

// Group ids of a partition are generally not contiguous, so a CompressedIdMap
// (which is dense over a contiguous id range) is a poor fit. Use the hash-based
// Map instead.
using GroupIdToDoubleMap = Map<GroupId, double>;

template <typename IdsContainer>
std::vector<int> asIntsVec(const IdsContainer& ids) {
  std::vector<int> idsInt;
  idsInt.reserve(ids.size());
  std::transform(
      ids.begin(), ids.end(), std::back_inserter(idsInt), [](const auto& id) {
        return id.asInt();
      });
  return idsInt;
}

template <typename IdIntToValueMap, typename IdToValueMap>
IdIntToValueMap asIntsMap(const IdToValueMap& idToValue) {
  IdIntToValueMap idIntToValue;
  if constexpr (requires { idToValue.nonDefaultSize(); }) {
    algopt::utils::reserveIfPossible(idIntToValue, idToValue.nonDefaultSize());
  } else {
    algopt::utils::reserveIfPossible(idIntToValue, idToValue.size());
  }
  algopt::utils::forEachNonDefault(
      idToValue, [&](const auto& id, const auto& value) {
        idIntToValue.emplace(id.asInt(), value);
      });
  return idIntToValue;
}

} // namespace facebook::rebalancer::entities

namespace std {

template <typename Tag>
struct hash<facebook::rebalancer::entities::EntityId<Tag>> {
  std::size_t operator()(
      facebook::rebalancer::entities::EntityId<Tag> id) const noexcept {
    return std::hash<facebook::rebalancer::entities::EntityIdType>{}(
        static_cast<facebook::rebalancer::entities::EntityIdType>(id));
  }
};

} // namespace std

namespace folly {

template <typename Tag>
struct hasher<facebook::rebalancer::entities::EntityId<Tag>> {
  constexpr std::size_t operator()(
      facebook::rebalancer::entities::EntityId<Tag> id) const noexcept {
    return folly::hasher<facebook::rebalancer::entities::EntityIdType>{}(
        static_cast<facebook::rebalancer::entities::EntityIdType>(id));
  }
};

} // namespace folly

template <typename T>
struct fmt::formatter<
    T,
    std::enable_if_t<
        facebook::rebalancer::entities::is_entity_id<T>::value,
        char>> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  static auto format(const T& id, FormatContext& ctx) {
    return fmt::format_to(ctx.out(), "{}", id.asInt());
  }
};

namespace facebook::rebalancer::entities {

struct IdStoreConfig {
  std::vector<std::string> objectNames;
  std::vector<std::string> containerNames;
  std::vector<std::string> scopeNames;
  std::vector<std::string> scopeItemNames;
  std::vector<std::string> partitionNames;
  std::vector<std::string> groupNames;
  std::vector<std::string> dimensionNames;
  std::vector<std::string> goalNames;
  std::vector<std::string> constraintNames;
  std::vector<std::string> routingConfigNames;
  Map<std::string, ObjectId> objectNameToId;
  Map<std::string, ContainerId> containerNameToId;
  Map<std::string, ScopeId> scopeNameToId;
  Map<ScopeId, std::shared_ptr<const Map<std::string, ScopeItemId>>>
      scopeIdToScopeItemNameToId;
  Map<std::string, DimensionId> dimensionNameToId;
  Map<std::string, PartitionId> partitionNameToId;
  Map<PartitionId, std::shared_ptr<const Map<std::string, GroupId>>>
      partitionIdToGroupNameToId;
  Map<std::string, GoalId> goalNameToId;
  Map<std::string, ConstraintId> constraintNameToId;
  Map<std::string, RoutingConfigId> routingConfigNameToId;
};

class IdStore {
 public:
  IdStore() = default; // TO BE DEPRECATED
  explicit IdStore(IdStoreConfig&& config);
  explicit IdStore(const thrift::IdStore& idStore);

  template <typename Id>
  const std::string& getEntityName(Id id) const {
    return namesFor<Id>().at(id.asIndex());
  }

  inline ObjectId getObjectId(const std::string& objectName) const {
    return objectIds_.at(objectName);
  }

  inline ContainerId getContainerId(const std::string& containerName) const {
    return containerIds_.at(containerName);
  }

  inline ScopeId getScopeId(const std::string& scopeName) const {
    return scopeIds_.at(scopeName);
  }

  inline ScopeItemId getScopeItemId(
      ScopeId scopeId,
      const std::string& scopeItemName) const {
    return scopeItemIds_.at(scopeId)->at(scopeItemName);
  }

  inline PartitionId getPartitionId(const std::string& partitionName) const {
    return partitionNameToId_.at(partitionName);
  }

  inline GroupId getGroupId(PartitionId partitionId, const std::string& groupId)
      const {
    return groupIds_.at(partitionId)->at(groupId);
  }

  inline DimensionId getDimensionId(const std::string& dimensionName) const {
    return dimensionIds_.at(dimensionName);
  }

  inline GoalId getGoalId(const std::string& goalName) const {
    return goalIds_.at(goalName);
  }

  inline ConstraintId getConstraintId(const std::string& constraintName) const {
    return constraintIds_.at(constraintName);
  }

  inline RoutingConfigId getRoutingConfigId(
      const std::string& configName) const {
    return routingConfigIds_.at(configName);
  }

  inline bool existsPartition(const std::string& partitionName) const {
    return partitionNameToId_.contains(partitionName);
  }

  thrift::IdStore toThrift() const;

 private:
  template <typename Id>
  const std::vector<std::string>& namesFor() const {
    if constexpr (std::is_same_v<Id, ObjectId>) {
      return objectNames_;
    } else if constexpr (std::is_same_v<Id, ContainerId>) {
      return containerNames_;
    } else if constexpr (std::is_same_v<Id, ScopeId>) {
      return scopeNames_;
    } else if constexpr (std::is_same_v<Id, ScopeItemId>) {
      return scopeItemNames_;
    } else if constexpr (std::is_same_v<Id, PartitionId>) {
      return partitionNames_;
    } else if constexpr (std::is_same_v<Id, GroupId>) {
      return groupNames_;
    } else if constexpr (std::is_same_v<Id, DimensionId>) {
      return dimensionNames_;
    } else if constexpr (std::is_same_v<Id, GoalId>) {
      return goalNames_;
    } else if constexpr (std::is_same_v<Id, ConstraintId>) {
      return constraintNames_;
    } else if constexpr (std::is_same_v<Id, RoutingConfigId>) {
      return routingConfigNames_;
    } else {
      static_assert(sizeof(Id) == 0, "Unknown entity Id type");
    }
  }

  std::vector<std::string> objectNames_;
  std::vector<std::string> containerNames_;
  std::vector<std::string> scopeNames_;
  std::vector<std::string> scopeItemNames_;
  std::vector<std::string> partitionNames_;
  std::vector<std::string> groupNames_;
  std::vector<std::string> dimensionNames_;
  std::vector<std::string> goalNames_;
  std::vector<std::string> constraintNames_;
  std::vector<std::string> routingConfigNames_;
  Map<std::string, ObjectId> objectIds_;
  Map<std::string, ContainerId> containerIds_;
  Map<std::string, ScopeId> scopeIds_;
  Map<ScopeId, std::shared_ptr<const Map<std::string, ScopeItemId>>>
      scopeItemIds_;
  Map<std::string, PartitionId> partitionNameToId_;
  Map<PartitionId, std::shared_ptr<const Map<std::string, GroupId>>> groupIds_;
  Map<std::string, DimensionId> dimensionIds_;
  Map<std::string, GoalId> goalIds_;
  Map<std::string, ConstraintId> constraintIds_;
  Map<std::string, RoutingConfigId> routingConfigIds_;
};

} // namespace facebook::rebalancer::entities
