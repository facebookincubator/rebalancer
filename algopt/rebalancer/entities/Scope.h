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

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/ScopeDimension.h"
#include "algopt/rebalancer/entities/Set.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/GraphState_types.h"

#include <memory>
#include <vector>

namespace facebook {
namespace rebalancer {
namespace entities {

class Scope {
 public:
  Scope(
      const std::shared_ptr<const Map<ScopeItemId, Set<ContainerId>>>&
          scopeItemIdToContainerIds,
      Map<DimensionId, std::shared_ptr<const ScopeDimension>> dimensions);

  explicit Scope(const thrift::Scope& scope);

  // Delete the copy and assignment constructors to prevent accidental copies.
  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;
  // Other operators are as usual.
  Scope(Scope&&) = default;
  Scope& operator=(Scope&&) = default;
  ~Scope() = default;

  const std::vector<ScopeItemId>& getScopeItemIds() const;

  std::optional<entities::ScopeItemId> getScopeItemId(
      ContainerId containerId) const;

  auto getDimensionIds() const {
    return std::views::keys(dimensions_);
  }

  const Set<ContainerId>& getContainerIds(ScopeItemId scopeItemId) const;

  const std::shared_ptr<const Set<ContainerId>> getContainerIdsPtr(
      ScopeItemId scopeItemId) const;

  const ScopeDimension& getDimension(DimensionId dimensionId) const;

  thrift::Scope toThrift() const;

  std::vector<thrift::GraphVertex> toEmbedding(
      const std::vector<DimensionId>& dimIds) const;

 private:
  Map<ScopeItemId, std::shared_ptr<const Set<ContainerId>>> scopeItems_;
  Map<ContainerId, ScopeItemId> containerIdToScopeItemId_;
  std::vector<ScopeItemId> scopeItemIds_;
  Map<DimensionId, std::shared_ptr<const ScopeDimension>> dimensions_;

  inline const static std::shared_ptr<const ScopeDimension> defaultDimension_ =
      std::make_shared<ScopeDimension>(Map<ScopeItemId, double>(), 1.0);
};

} // namespace entities
} // namespace rebalancer
} // namespace facebook
