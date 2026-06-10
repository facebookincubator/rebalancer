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

#include "algopt/rebalancer/entities/builders/AsyncValueMap.h"
#include "algopt/rebalancer/entities/Identifiers.h"

#include <folly/container/F14Map.h>
#include <folly/coro/Task.h>
#include <folly/futures/SharedPromise.h>
#include <folly/Synchronized.h>

namespace facebook::rebalancer::entities {

class IdBuilder {
 public:
  template <typename Id>
  Id next() {
    return Id(counterFor<Id>()++);
  }

  template <typename Id>
  EntityIdType getCount() const {
    return counterFor<Id>();
  }

  template <typename Id, typename Data>
  [[nodiscard]] Id makeIdFromName(
      const std::string& name,
      folly::Synchronized<Map<std::string, Id>>& nameToId,
      AsyncValueMap<Id, Data>& idToData,
      const std::string_view funcName) {
    return nameToId.withWLock([&](auto& lockedNameToId) -> Id {
      if (lockedNameToId.contains(name)) [[unlikely]] {
        throw std::runtime_error(
            fmt::format(
                "Unexpected call to {} with a previously added name '{}'",
                funcName,
                name));
      }

      auto id = next<Id>();
      const auto registered = idToData.registerKey(id);
      if (!registered) [[unlikely]] {
        throw std::runtime_error(
            fmt::format(
                "Unexpected duplicate call to registerKey() w.r.t. function '{}' and entity name '{}'",
                funcName,
                name));
      }

      lockedNameToId.emplace(name, id);
      return id;
    });
  }

 private:
  template <typename Id>
  std::atomic<EntityIdType>& counterFor() {
    if constexpr (std::is_same_v<Id, ObjectId>) {
      return nextObjectId_;
    } else if constexpr (std::is_same_v<Id, ContainerId>) {
      return nextContainerId_;
    } else if constexpr (std::is_same_v<Id, ScopeId>) {
      return nextScopeId_;
    } else if constexpr (std::is_same_v<Id, ScopeItemId>) {
      return nextScopeItemId_;
    } else if constexpr (std::is_same_v<Id, PartitionId>) {
      return nextPartitionId_;
    } else if constexpr (std::is_same_v<Id, GroupId>) {
      return nextGroupId_;
    } else if constexpr (std::is_same_v<Id, DimensionId>) {
      return nextDimensionId_;
    } else if constexpr (std::is_same_v<Id, GoalId>) {
      return nextGoalId_;
    } else if constexpr (std::is_same_v<Id, ConstraintId>) {
      return nextConstraintId_;
    } else if constexpr (std::is_same_v<Id, RoutingConfigId>) {
      return nextRoutingConfigId_;
    } else {
      static_assert(sizeof(Id) == 0, "Unknown entity Id type");
    }
  }

  template <typename Id>
  const std::atomic<EntityIdType>& counterFor() const {
    return const_cast<IdBuilder*>(this)->counterFor<Id>();
  }

  std::atomic<EntityIdType> nextObjectId_{0};
  std::atomic<EntityIdType> nextContainerId_{0};
  std::atomic<EntityIdType> nextScopeId_{0};
  std::atomic<EntityIdType> nextScopeItemId_{0};
  std::atomic<EntityIdType> nextPartitionId_{0};
  std::atomic<EntityIdType> nextGroupId_{0};
  std::atomic<EntityIdType> nextDimensionId_{0};
  std::atomic<EntityIdType> nextGoalId_{0};
  std::atomic<EntityIdType> nextConstraintId_{0};
  std::atomic<EntityIdType> nextRoutingConfigId_{0};
};

} // namespace facebook::rebalancer::entities
