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
#include "algopt/rebalancer/entities/builders/IdBuilder.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/ObjectDimension.h"
#include "algopt/rebalancer/entities/ScopeDimension.h"

#include <folly/coro/Task.h>

namespace facebook::rebalancer::entities {

using DimensionScopeIdPair = std::pair<DimensionId, ScopeId>;

struct ObjectDimensionData {
  std::shared_ptr<const ObjectDimension> dimension;
};

struct ScopeDimensionData {
  std::shared_ptr<const ScopeDimension> dimension;
};

struct DimensionsBuilderResult {
  Map<std::string, DimensionId> dimensionNameToId;
  Map<DimensionId, std::shared_ptr<const ObjectDimension>>
      dimensionIdToObjectDimension;
  Map<ScopeId, Map<DimensionId, std::shared_ptr<const ScopeDimension>>>
      scopeIdToDimensionIdToDimension;
};

class DimensionsBuilder {
 public:
  explicit DimensionsBuilder(IdBuilder& idBuilder) : idBuilder_(idBuilder) {}

  folly::coro::Task<void> add(DimensionId id, ObjectDimensionData dimension) {
    co_return objectDimensionIdToData_.set(id, std::move(dimension));
  }

  folly::coro::Task<void>
  add(DimensionId dimensionId, ScopeId scopeId, ScopeDimensionData dimension) {
    co_return scopeDimensionIdToData_.set(
        std::make_pair(dimensionId, scopeId), std::move(dimension));
  }

  DimensionId makeObjectDimensionId(const std::string& name);
  DimensionId makeScopeDimensionId(const std::string& name, ScopeId scopeId);

  DimensionId getDimensionId(const std::string& name) const {
    return dimensionNameToId_.rlock()->at(name);
  }

  folly::coro::Task<DimensionsBuilderResult> build() const;

  folly::coro::Task<std::string> summarize() const;

 private:
  DimensionId getOrMakeDimensionId(const std::string& name);

 private:
  IdBuilder& idBuilder_;
  folly::Synchronized<Map<std::string, DimensionId>> dimensionNameToId_;
  AsyncValueMap<DimensionId, ObjectDimensionData> objectDimensionIdToData_;
  AsyncValueMap<DimensionScopeIdPair, ScopeDimensionData>
      scopeDimensionIdToData_;
};

} // namespace facebook::rebalancer::entities

template <>
struct fmt::formatter<facebook::rebalancer::entities::DimensionScopeIdPair>
    : fmt::formatter<std::string> {
  auto format(
      const facebook::rebalancer::entities::DimensionScopeIdPair& p,
      fmt::format_context& ctx) const {
    return fmt::formatter<std::string>::format(
        fmt::format("(dimensionId: {}, scopeId: {})", p.first, p.second), ctx);
  }
};
