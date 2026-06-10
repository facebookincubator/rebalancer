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

#include "algopt/rebalancer/entities/Set.h"
#include "algopt/rebalancer/entities/Universe.h"

namespace facebook::rebalancer::materializer {

template <typename EntityType>
class FilterWrapper {
 public:
  // returns true if both allowList and blockList are unset
  bool empty() const {
    return !allowList_.has_value() && !blockList_.has_value();
  }

 protected:
  FilterWrapper(
      const entities::Universe& universe,
      const facebook::rebalancer::interface::Filter& filter,
      const std::function<EntityType(std::string)>& nameToId);

  std::vector<EntityType> getFilteredIds(
      const std::vector<EntityType>& allIds) const;

  bool isIncluded(EntityType id) const;

 private:
  void populateAllowAndBlockLists(
      const std::function<EntityType(std::string)>& nameToId);

 protected:
  const entities::Universe& universe_;
  const facebook::rebalancer::interface::Filter& filter_;

 private:
  std::optional<entities::Set<EntityType>> allowList_;
  std::optional<entities::Set<EntityType>> blockList_;
};

class ScopeItemFilterWrapper : public FilterWrapper<entities::ScopeItemId> {
 public:
  ScopeItemFilterWrapper(
      const entities::Universe& universe,
      const facebook::rebalancer::interface::Filter& filter,
      entities::ScopeId scopeId);

  std::vector<entities::ScopeItemId> getScopeItemIds() const;

  std::vector<entities::ScopeItemId> getExcludedScopeItemIds() const;

  bool isIncluded(entities::ScopeItemId scopeItemId) const;

 private:
  entities::ScopeId scopeId_;
};

class GroupFilterWrapper : public FilterWrapper<entities::GroupId> {
 public:
  GroupFilterWrapper(
      const entities::Universe& universe,
      const facebook::rebalancer::interface::Filter& filter,
      entities::PartitionId partitionId);

  std::vector<entities::GroupId> getGroupIds() const;

  bool isIncluded(entities::GroupId groupIdId) const;

 private:
  entities::PartitionId partitionId_;
};

} // namespace facebook::rebalancer::materializer
