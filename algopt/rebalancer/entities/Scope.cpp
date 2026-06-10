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

#include "algopt/rebalancer/entities/Scope.h"

#include "algopt/rebalancer/entities/GraphEmbeddingUtils.h"

#include <folly/container/MapUtil.h>

namespace facebook {
namespace rebalancer {
namespace entities {

Scope::Scope(
    const std::shared_ptr<const Map<ScopeItemId, Set<ContainerId>>>&
        scopeItemIdToContainerIds,
    Map<DimensionId, std::shared_ptr<const ScopeDimension>> dimensions)
    : dimensions_(std::move(dimensions)) {
  for (auto& [scopeItemId, containerIds] : *scopeItemIdToContainerIds) {
    for (auto containerId : containerIds) {
      auto inserted =
          containerIdToScopeItemId_.emplace(containerId, scopeItemId).second;
      if (!inserted) {
        throw std::runtime_error(
            fmt::format(
                "containerId {} appears as part of more than one scope item",
                containerId));
      }
    }

    scopeItemIds_.push_back(scopeItemId);
    scopeItems_.emplace(
        scopeItemId,
        std::shared_ptr<const Set<ContainerId>>(
            scopeItemIdToContainerIds, &containerIds));
  }
}

Scope::Scope(const thrift::Scope& scope) {
  const auto& thriftScopeItems = *scope.scopeItems();
  scopeItemIds_.reserve(thriftScopeItems.size());
  scopeItems_.reserve(thriftScopeItems.size());
  for (const auto& [scopeItemIdInt, containers] : thriftScopeItems) {
    auto scopeItemId = ScopeItemId(scopeItemIdInt);
    auto containerIds = std::make_shared<Set<ContainerId>>();
    containerIds->reserve(containers.size());
    for (auto containerIdInt : containers) {
      auto containerId = ContainerId(containerIdInt);
      containerIds->insert(containerId);
      auto inserted =
          containerIdToScopeItemId_.emplace(containerId, scopeItemId).second;
      if (!inserted) {
        throw std::runtime_error(
            fmt::format(
                "containerId {} appears as part of more than one scope item",
                containerId));
      }
    }
    scopeItems_.emplace(scopeItemId, std::move(containerIds));
    scopeItemIds_.push_back(scopeItemId);
  }
  dimensions_.reserve(scope.dimensions()->size());
  for (auto& [dimensionId, dimension] : *scope.dimensions()) {
    dimensions_.emplace(
        DimensionId(dimensionId), std::make_shared<ScopeDimension>(dimension));
  }
}

const std::vector<ScopeItemId>& Scope::getScopeItemIds() const {
  return scopeItemIds_;
}

std::optional<entities::ScopeItemId> Scope::getScopeItemId(
    ContainerId containerId) const {
  auto scopeItemIdPtr = folly::get_ptr(containerIdToScopeItemId_, containerId);
  if (scopeItemIdPtr == nullptr) {
    return std::nullopt;
  }
  return *scopeItemIdPtr;
}

const Set<ContainerId>& Scope::getContainerIds(ScopeItemId scopeItemId) const {
  return *scopeItems_.at(scopeItemId);
}

const std::shared_ptr<const Set<ContainerId>> Scope::getContainerIdsPtr(
    ScopeItemId scopeItemId) const {
  return scopeItems_.at(scopeItemId);
}

const ScopeDimension& Scope::getDimension(DimensionId dimensionId) const {
  return *folly::get_ref_default(dimensions_, dimensionId, defaultDimension_);
}

thrift::Scope Scope::toThrift() const {
  thrift::Scope scope;
  auto& thriftScopeItems = *scope.scopeItems();
  thriftScopeItems.reserve(scopeItems_.size());
  for (const auto& [scopeItemId, containerIds] : scopeItems_) {
    thriftScopeItems.emplace(scopeItemId.asInt(), asIntsVec(*containerIds));
  }

  auto& thriftScopeDimensions = *scope.dimensions();
  thriftScopeDimensions.reserve(dimensions_.size());
  for (const auto& [dimensionId, dimension] : dimensions_) {
    thriftScopeDimensions.emplace(dimensionId.asInt(), dimension->toThrift());
  }
  return scope;
}

std::vector<thrift::GraphVertex> Scope::toEmbedding(
    const std::vector<DimensionId>& dimIds) const {
  // Precompute dimension references once (O(M)) instead of per scope item
  // (O(N*M)), since getDimension() does a hash-map lookup.
  std::vector<const ScopeDimension*> dims;
  dims.reserve(dimIds.size());
  for (const auto dimId : dimIds) {
    dims.push_back(&getDimension(dimId));
  }

  std::vector<thrift::GraphVertex> vertices;
  vertices.reserve(scopeItemIds_.size());

  for (const auto scopeItemId : scopeItemIds_) {
    thrift::GraphVertex vertex;
    vertex.type() = thrift::VertexType::SCOPE_ITEM;
    vertex.entityId() = scopeItemId.asInt();

    auto& embedding = *vertex.embedding();
    embedding.reserve(kNumVertexTypes + dimIds.size());
    initOneHotEmbedding(embedding, thrift::VertexType::SCOPE_ITEM);

    for (const auto* dim : dims) {
      embedding.push_back(dim->getValue(scopeItemId));
    }

    vertices.push_back(std::move(vertex));
  }
  return vertices;
}

} // namespace entities
} // namespace rebalancer
} // namespace facebook
