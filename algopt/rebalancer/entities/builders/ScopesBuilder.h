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
#include "algopt/rebalancer/entities/Set.h"

#include <folly/container/F14Map.h>
#include <folly/coro/Collect.h>
#include <folly/coro/Task.h>
#include <folly/futures/SharedPromise.h>

namespace facebook::rebalancer::entities {

struct ScopeData {
  ScopeItemId getScopeItemId(const std::string& scopeItemName) const {
    return scopeItemNameToId->at(scopeItemName);
  }

  std::shared_ptr<const Map<std::string, ScopeItemId>> scopeItemNameToId;
  std::shared_ptr<const Map<ScopeItemId, Set<ContainerId>>>
      scopeItemIdToContainerIds;
};

struct ScopesBuilderResult {
  Map<std::string, ScopeId> scopeNameToId;
  Map<ScopeId, std::shared_ptr<const Map<std::string, ScopeItemId>>>
      scopeIdToScopeItemNameToId;
  Map<ScopeId, std::shared_ptr<const Map<ScopeItemId, Set<ContainerId>>>>
      scopeIdToScopeItemIdToContainerIds;
};

class ScopesBuilder {
 public:
  explicit ScopesBuilder(IdBuilder& idBuilder) : idBuilder_(idBuilder) {}

  template <typename ScopeItemToContainers>
  folly::coro::Task<void> add(
      ScopeId scopeId,
      ScopeItemToContainers scopeItemToContainers,
      std::shared_ptr<const ContainersData> containers)
    requires IsIterableOverPairs<
        ScopeItemToContainers,
        std::string,
        std::vector<std::string>>;

  template <typename ContainerToScopeItem>
  folly::coro::Task<void> add(
      ScopeId scopeId,
      ContainerToScopeItem containerToScopeItem,
      std::shared_ptr<const ContainersData> containers)
    requires IsIterableOverPairs<ContainerToScopeItem, std::string, std::string>
  ;

  folly::coro::Task<std::shared_ptr<const ScopeData>> get(ScopeId id) const {
    co_return co_await scopeIdToData_.get(id);
  }

  ScopeId makeScopeId(const std::string& scopeName);

  ScopeId getScopeId(const std::string& scopeName) const {
    return scopeNameToId_.rlock()->at(scopeName);
  }

  folly::coro::Task<ScopesBuilderResult> build() const;

  folly::coro::Task<std::string> summarize() const;

 private:
  IdBuilder& idBuilder_;
  AsyncValueMap<ScopeId, ScopeData> scopeIdToData_;
  folly::Synchronized<Map<std::string, ScopeId>> scopeNameToId_;
};

template <typename ScopeItemToContainers>
folly::coro::Task<void> ScopesBuilder::add(
    ScopeId scopeId,
    ScopeItemToContainers scopeItemToContainerNames,
    std::shared_ptr<const ContainersData> containers)
  requires IsIterableOverPairs<
      ScopeItemToContainers,
      std::string,
      std::vector<std::string>>
{
  Map<ScopeItemId, Set<ContainerId>> scopeItemIdToContainerIds;
  Map<std::string, ScopeItemId> scopeItemNameToId;
  const auto numScopeItems =
      static_cast<decltype(scopeItemNameToId)::size_type>(
          scopeItemToContainerNames.size());
  scopeItemNameToId.reserve(numScopeItems);
  scopeItemIdToContainerIds.reserve(numScopeItems);
  for (const auto& [scopeItemName, containerNames] :
       scopeItemToContainerNames) {
    const auto scopeItemId = idBuilder_.next<ScopeItemId>();
    Set<ContainerId> scopeContainers;
    scopeContainers.reserve(containerNames.size());
    for (const auto& containerName : containerNames) {
      const auto containerId = containers->getId(containerName);
      scopeContainers.insert(containerId);
    }
    scopeItemNameToId.emplace(scopeItemName, scopeItemId);
    scopeItemIdToContainerIds.emplace(scopeItemId, std::move(scopeContainers));
  }

  scopeIdToData_.set(
      scopeId,
      ScopeData{
          .scopeItemNameToId =
              std::make_shared<const Map<std::string, ScopeItemId>>(
                  std::move(scopeItemNameToId)),
          .scopeItemIdToContainerIds =
              std::make_shared<const Map<ScopeItemId, Set<ContainerId>>>(
                  std::move(scopeItemIdToContainerIds)),
      });

  co_return;
}

template <typename ContainerToScopeItem>
folly::coro::Task<void> ScopesBuilder::add(
    ScopeId scopeId,
    ContainerToScopeItem containerToScopeItem,
    std::shared_ptr<const ContainersData> containers)
  requires IsIterableOverPairs<ContainerToScopeItem, std::string, std::string>
{
  Map<ScopeItemId, Set<ContainerId>> scopeItemIdToContainerIds;
  Map<std::string, ScopeItemId> scopeItemNameToId;
  for (const auto& [containerName, scopeItemName] : containerToScopeItem) {
    const auto containerId = containers->getId(containerName);
    const auto scopeItemPtr = folly::get_ptr(scopeItemNameToId, scopeItemName);
    if (!scopeItemPtr) {
      const auto scopeItemId = idBuilder_.next<ScopeItemId>();
      scopeItemNameToId.emplace(scopeItemName, scopeItemId);
      scopeItemIdToContainerIds[scopeItemId].insert(containerId);
    } else {
      scopeItemIdToContainerIds[*scopeItemPtr].insert(containerId);
    }
  }

  scopeIdToData_.set(
      scopeId,
      ScopeData{
          .scopeItemNameToId =
              std::make_shared<const Map<std::string, ScopeItemId>>(
                  std::move(scopeItemNameToId)),
          .scopeItemIdToContainerIds =
              std::make_shared<const Map<ScopeItemId, Set<ContainerId>>>(
                  std::move(scopeItemIdToContainerIds)),
      });

  co_return;
}

} // namespace facebook::rebalancer::entities
