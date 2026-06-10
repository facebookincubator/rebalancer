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

#include "algopt/rebalancer/solver/moves/DestinationsToExploreGenerator.h"

namespace facebook::rebalancer {

DestinationsToExploreGenerator::DestinationsToExploreGenerator(
    const PackerSet<entities::ContainerId>& nonAcceptingContainers,
    const entities::Universe& universe)
    : nonAcceptingContainers_(nonAcceptingContainers), universe_(universe) {}

ReferenceList<const std::vector<entities::ContainerId>>
DestinationsToExploreGenerator::getAcceptingDestinations(
    const interface::MoveToCurrentScopeItemSpec& moveToCurrentScopeItem,
    entities::ContainerId hotContainerId) {
  auto& scopeName =
      *moveToCurrentScopeItem.scopeNameForExploringMovesToCurrentScopeItem();

  auto scopeId = universe_.getScopeId(scopeName);
  auto& scope = universe_.getScope(scopeId);
  auto scopeItemId = scope.getScopeItemId(hotContainerId);
  if (!scopeItemId.has_value()) {
    // if hotContainer is not in the given scope, we explore all containers
    // in the given scope
    interface::ScopeItemList scopeItemList;
    scopeItemList.scopeName() = scopeName;
    return getAcceptingContainersList(scopeItemList, hotContainerId);
  }

  return ReferenceList<const std::vector<entities::ContainerId>>{
      getAcceptingContainers(scopeItemId.value(), scope)};
}

ReferenceList<const std::vector<entities::ContainerId>>
DestinationsToExploreGenerator::getAcceptingDestinations(
    const interface::MoveToScopeItemsSpec& moveToScopeItems,
    const entities::ObjectId hotObject,
    const entities::ContainerId hotContainerId) {
  auto& objectToScopeItems = *moveToScopeItems.objectToScopeItems();
  auto& hotObjectName = universe_.getEntityName(hotObject);

  // if an object is specified in objectToScopeItems, use that.
  auto scopeItemListPtr = folly::get_ptr(objectToScopeItems, hotObjectName);
  if (scopeItemListPtr) {
    return getAcceptingContainersList(*scopeItemListPtr, hotContainerId);
  }

  // if a group is specified in groupToScopeItems and hotObject belongs to that
  // group, then use that.
  auto& defaultScopeItems = *moveToScopeItems.defaultScopeItems();
  auto& scopeItemsPerGroup = *moveToScopeItems.scopeItemsPerGroups();
  auto& groupToScopeItem = *scopeItemsPerGroup.groupToScopeItemList();
  if (!groupToScopeItem.empty()) {
    auto& partitionName = *scopeItemsPerGroup.partitionName();
    auto partitionId = universe_.getPartitionId(partitionName);
    auto& partition = universe_.getPartition(partitionId);
    auto& objectIdToGroupIds = partition.getObjectIdToGroupIds();
    auto groupIdsPtr = folly::get_ptr(objectIdToGroupIds, hotObject);
    if (groupIdsPtr && groupIdsPtr->size() > 0) {
      if (groupIdsPtr->size() > 1) {
        throw std::runtime_error(
            fmt::format(
                "groupToScopeItemList in MoveToScopeItemsSpec is not supported when objects (e.g., '{}') belong to multiple groups in partition '{}'",
                hotObjectName,
                partitionName));
      }

      auto& onlyGroupId = groupIdsPtr->at(0);
      auto& scopeItemList = folly::get_ref_default(
          groupToScopeItem,
          universe_.getEntityName(onlyGroupId),
          defaultScopeItems);
      return getAcceptingContainersList(scopeItemList, hotContainerId);
    }
  }

  // use defaultScopeItems if there is no specialization for hotObject
  return getAcceptingContainersList(defaultScopeItems, hotContainerId);
}

ReferenceList<const std::vector<entities::ContainerId>>
DestinationsToExploreGenerator::getAcceptingDestinations(
    const interface::MoveToScopeItemsSpec& moveToScopeItems,
    const entities::ContainerId hotContainerId) {
  if (!moveToScopeItems.objectToScopeItems()->empty()) {
    throw std::runtime_error(
        "this function requires that objectToScopeItems is empty");
  }
  return getAcceptingContainersList(
      *moveToScopeItems.defaultScopeItems(), hotContainerId);
}

ReferenceList<const std::vector<entities::ContainerId>>
DestinationsToExploreGenerator::getAcceptingContainersList(
    const interface::ScopeItemList& scopeItemList,
    const entities::ContainerId hotContainerId) {
  ReferenceList<const std::vector<entities::ContainerId>> destinations;
  auto& scopeName = *scopeItemList.scopeName();
  auto scopeId = universe_.getScopeId(scopeName);
  auto& scope = universe_.getScope(scopeId);
  if (scopeItemList.scopeItems().has_value()) {
    auto& scopeItemNames = scopeItemList.scopeItems().value();
    for (auto& scopeItemName : scopeItemNames) {
      auto scopeItemId = universe_.getScopeItemId(scopeId, scopeItemName);
      destinations.emplace_back(getAcceptingContainers(scopeItemId, scope));
    }
  } else if (scopeItemList.exploreCurrentScopeItem().value()) {
    const auto scopeItemId = scope.getScopeItemId(hotContainerId);
    if (!scopeItemId.has_value()) {
      interface::ScopeItemList newScopeItemList;
      newScopeItemList.scopeName() = scopeName;
      return getAcceptingContainersList(newScopeItemList, hotContainerId);
    } else {
      return ReferenceList<const std::vector<entities::ContainerId>>{
          getAcceptingContainers(scopeItemId.value(), scope)};
    }
  } else {
    // if scopeItems are not explicitly listed, all scopeItems in the specified
    // scopeName are taken
    auto& scopeItemIds = scope.getScopeItemIds();
    for (auto scopeItemId : scopeItemIds) {
      destinations.emplace_back(getAcceptingContainers(scopeItemId, scope));
    }
  }
  return destinations;
}

const std::vector<entities::ContainerId>&
DestinationsToExploreGenerator::getAcceptingContainers(
    entities::ScopeItemId scopeItemId,
    const entities::Scope& scope) {
  return scopeItemToAcceptingDestinations_.getSavedOrCompute(
      scopeItemId, [&]() {
        auto& containerIds = scope.getContainerIds(scopeItemId);
        std::vector<entities::ContainerId> destinations;
        std::copy_if(
            containerIds.begin(),
            containerIds.end(),
            std::back_inserter(destinations),
            [this](entities::ContainerId containerId) {
              return !nonAcceptingContainers_.contains(containerId);
            });

        return destinations;
      });
}

} // namespace facebook::rebalancer
