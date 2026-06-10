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

#include "algopt/rebalancer/entities/Identifiers.h"

#include <folly/Portability.h>

namespace facebook::rebalancer::entities {

template <class T>
void initialize(
    const std::vector<std::string>& names,
    const std::vector<int>& ids,
    Map<std::string, T>& nameToId) {
  nameToId.reserve(ids.size());
  for (const auto id : ids) {
    nameToId.emplace(names.at(id), T(id));
  }
}

template <class T1, class T2>
void initialize(
    const std::vector<std::string>& t1Names,
    const std::vector<std::string>& t2Names,
    const folly::F14FastMap<int, std::vector<int>>& ids,
    Map<std::string, T1>& nameToId1,
    Map<T1, std::shared_ptr<const Map<std::string, T2>>>& id1ToNameToId2) {
  nameToId1.reserve(ids.size());
  id1ToNameToId2.reserve(ids.size());
  for (const auto& [id1, ids2] : ids) {
    nameToId1.emplace(t1Names.at(id1), T1(id1));
    Map<std::string, T2> nameToId2;
    initialize(t2Names, ids2, nameToId2);
    id1ToNameToId2.emplace(
        T1(id1),
        std::make_shared<const Map<std::string, T2>>(std::move(nameToId2)));
  }
}

IdStore::IdStore(const thrift::IdStore& idStore) {
  // Old-format bundles populated the deprecated `names` field (and not the
  // per-type `objectNames` etc.). Reject them — callers must convert via
  // BackwardCompatabilityUtils::densifyEntityIds first.
  FOLLY_PUSH_WARNING
  FOLLY_GNU_DISABLE_WARNING("-Wdeprecated-declarations")
  // NOLINTNEXTLINE(facebook-hte-Deprecated)
  const bool hasOldNames = !idStore.names()->empty();
  FOLLY_POP_WARNING
  const bool missingNewNames =
      idStore.objectNames()->empty() && !idStore.objectIds()->empty();
  if (hasOldNames || missingNewNames) {
    throw std::runtime_error(
        "IdStore thrift deserialization received an old-format bundle. "
        "Convert via BackwardCompatabilityUtils::densifyEntityIds first.");
  }

  objectNames_ = *idStore.objectNames();
  containerNames_ = *idStore.containerNames();
  scopeNames_ = *idStore.scopeNames();
  scopeItemNames_ = *idStore.scopeItemNames();
  partitionNames_ = *idStore.partitionNames();
  groupNames_ = *idStore.groupNames();
  dimensionNames_ = *idStore.dimensionNames();
  goalNames_ = *idStore.goalNames();
  constraintNames_ = *idStore.constraintNames();
  routingConfigNames_ = *idStore.routingConfigNames();

  initialize(objectNames_, *idStore.objectIds(), objectIds_);
  initialize(containerNames_, *idStore.containerIds(), containerIds_);
  initialize(
      scopeNames_,
      scopeItemNames_,
      *idStore.scopeItemIds(),
      scopeIds_,
      scopeItemIds_);
  initialize(
      partitionNames_,
      groupNames_,
      *idStore.groupIds(),
      partitionNameToId_,
      groupIds_);
  initialize(dimensionNames_, *idStore.dimensionIds(), dimensionIds_);
  initialize(goalNames_, *idStore.goalIds(), goalIds_);
  initialize(constraintNames_, *idStore.constraintIds(), constraintIds_);
  initialize(
      routingConfigNames_, *idStore.routingConfigIds(), routingConfigIds_);
}

IdStore::IdStore(IdStoreConfig&& config)
    : objectNames_(std::move(config.objectNames)),
      containerNames_(std::move(config.containerNames)),
      scopeNames_(std::move(config.scopeNames)),
      scopeItemNames_(std::move(config.scopeItemNames)),
      partitionNames_(std::move(config.partitionNames)),
      groupNames_(std::move(config.groupNames)),
      dimensionNames_(std::move(config.dimensionNames)),
      goalNames_(std::move(config.goalNames)),
      constraintNames_(std::move(config.constraintNames)),
      routingConfigNames_(std::move(config.routingConfigNames)),
      objectIds_(std::move(config.objectNameToId)),
      containerIds_(std::move(config.containerNameToId)),
      scopeIds_(std::move(config.scopeNameToId)),
      scopeItemIds_(std::move(config.scopeIdToScopeItemNameToId)),
      partitionNameToId_(std::move(config.partitionNameToId)),
      groupIds_(std::move(config.partitionIdToGroupNameToId)),
      dimensionIds_(std::move(config.dimensionNameToId)),
      goalIds_(std::move(config.goalNameToId)),
      constraintIds_(std::move(config.constraintNameToId)),
      routingConfigIds_(std::move(config.routingConfigNameToId)) {}

template <class T>
std::vector<int> getIds(const Map<std::string, T>& nameToId) {
  std::vector<int> ids;
  ids.reserve(nameToId.size());
  for (auto& [_, id] : nameToId) {
    ids.push_back(id.asInt());
  }
  return ids;
}

template <class T1, class T2>
folly::F14FastMap<int, std::vector<int>> getIds(
    const Map<T1, std::shared_ptr<const Map<std::string, T2>>>&
        id1ToNameToId2) {
  folly::F14FastMap<int, std::vector<int>> ids;
  ids.reserve(id1ToNameToId2.size());
  for (auto& [id1, nameToId2] : id1ToNameToId2) {
    ids.emplace(static_cast<EntityIdType>(id1), getIds(*nameToId2));
  }
  return ids;
}

thrift::IdStore IdStore::toThrift() const {
  thrift::IdStore idStore;
  idStore.objectIds() = getIds(objectIds_);
  idStore.containerIds() = getIds(containerIds_);
  idStore.scopeItemIds() = getIds(scopeItemIds_);
  idStore.groupIds() = getIds(groupIds_);
  idStore.dimensionIds() = getIds(dimensionIds_);
  idStore.goalIds() = getIds(goalIds_);
  idStore.constraintIds() = getIds(constraintIds_);
  idStore.routingConfigIds() = getIds(routingConfigIds_);
  idStore.objectNames() = objectNames_;
  idStore.containerNames() = containerNames_;
  idStore.scopeNames() = scopeNames_;
  idStore.scopeItemNames() = scopeItemNames_;
  idStore.partitionNames() = partitionNames_;
  idStore.groupNames() = groupNames_;
  idStore.dimensionNames() = dimensionNames_;
  idStore.goalNames() = goalNames_;
  idStore.constraintNames() = constraintNames_;
  idStore.routingConfigNames() = routingConfigNames_;
  return idStore;
}

} // namespace facebook::rebalancer::entities
