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

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"

#include "algopt/rebalancer/entities/Set.h"

using namespace facebook::rebalancer::entities;
using namespace facebook::rebalancer::interface;

namespace facebook::rebalancer::materializer {

template <typename EntityType>
FilterWrapper<EntityType>::FilterWrapper(
    const entities::Universe& universe,
    const facebook::rebalancer::interface::Filter& filter,
    const std::function<EntityType(std::string)>& nameToId)
    : universe_(universe), filter_(filter) {
  populateAllowAndBlockLists(nameToId);
}

template <typename EntityType>
void FilterWrapper<EntityType>::populateAllowAndBlockLists(
    const std::function<EntityType(std::string)>& nameToId) {
  if (filter_.itemsWhitelist() && filter_.itemsBlacklist()) {
    throw std::runtime_error(
        "Expected at most one of itemsWhitelist or itemsBlacklist to be set.");
  }

  if (filter_.itemsWhitelist()) {
    allowList_ = entities::Set<EntityType>();
    for (auto& itemName : *filter_.itemsWhitelist()) {
      allowList_->insert(nameToId(itemName));
    }
  }

  if (filter_.itemsBlacklist()) {
    blockList_ = entities::Set<EntityType>();
    for (auto& itemName : *filter_.itemsBlacklist()) {
      blockList_->insert(nameToId(itemName));
    }
  }
}

template <typename EntityType>
std::vector<EntityType> FilterWrapper<EntityType>::getFilteredIds(
    const std::vector<EntityType>& allIds) const {
  if (allowList_.has_value()) {
    std::vector<EntityType> ids;
    ids.reserve(allowList_->size());

    for (auto id : *allowList_) {
      ids.push_back(id);
    }
    return ids;
  }

  if (blockList_.has_value()) {
    std::vector<EntityType> ids;
    ids.reserve(allIds.size() - blockList_->size());

    for (auto id : allIds) {
      if (!blockList_->contains(id)) {
        ids.push_back(id);
      }
    }
    return ids;
  }

  return allIds;
}

template <typename EntityType>
bool FilterWrapper<EntityType>::isIncluded(EntityType id) const {
  if (allowList_) {
    return allowList_->contains(id);
  }
  if (blockList_) {
    return blockList_->contains(id) ? false : true;
  }

  return true;
}

ScopeItemFilterWrapper::ScopeItemFilterWrapper(
    const entities::Universe& universe,
    const Filter& filter,
    entities::ScopeId scopeId)
    : FilterWrapper(
          universe,
          filter,
          [&universe, scopeId](const std::string& scopeItemName) {
            return universe.getScopeItemId(scopeId, scopeItemName);
          }),
      scopeId_(scopeId) {
  if (filter_.type() != FilterType::SCOPE_ITEM) {
    throw std::runtime_error(
        "ScopeItemFilterWrapper can only be used when filter type is FilterType::SCOPE_ITEM.");
  }
}

std::vector<entities::ScopeItemId> ScopeItemFilterWrapper::getScopeItemIds()
    const {
  return getFilteredIds(universe_.getScope(scopeId_).getScopeItemIds());
}

std::vector<entities::ScopeItemId>
ScopeItemFilterWrapper::getExcludedScopeItemIds() const {
  std::vector<entities::ScopeItemId> excludedScopeItemIds;
  auto& allScopeItemIds = universe_.getScope(scopeId_).getScopeItemIds();
  for (auto scopeItemId : allScopeItemIds) {
    if (!this->FilterWrapper::isIncluded(scopeItemId)) {
      excludedScopeItemIds.push_back(scopeItemId);
    }
  }

  return excludedScopeItemIds;
}

bool ScopeItemFilterWrapper::isIncluded(
    entities::ScopeItemId scopeItemId) const {
  return this->FilterWrapper::isIncluded(scopeItemId);
}

GroupFilterWrapper::GroupFilterWrapper(
    const entities::Universe& universe,
    const Filter& filter,
    entities::PartitionId partitionId)
    : FilterWrapper(
          universe,
          filter,
          [&universe, partitionId](const std::string& groupName) {
            return universe.getGroupId(partitionId, groupName);
          }),
      partitionId_(partitionId) {
  if (filter_.type() != FilterType::GROUP) {
    throw std::runtime_error(
        "GroupFilterWrapper can only be used when filter type is FilterType::GROUP.");
  }
}

std::vector<entities::GroupId> GroupFilterWrapper::getGroupIds() const {
  return getFilteredIds(universe_.getPartition(partitionId_).getGroupIds());
}

bool GroupFilterWrapper::isIncluded(entities::GroupId groupId) const {
  return this->FilterWrapper::isIncluded(groupId);
}

} // namespace facebook::rebalancer::materializer
