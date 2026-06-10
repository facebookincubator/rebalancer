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

#include "algopt/rebalancer/entities/builders/DimensionsBuilder.h"

#include <folly/coro/Collect.h>

namespace facebook::rebalancer::entities {

DimensionId DimensionsBuilder::makeObjectDimensionId(const std::string& name) {
  const auto id = getOrMakeDimensionId(name);
  const auto registered = objectDimensionIdToData_.registerKey(id);
  if (!registered) {
    throw std::runtime_error(
        fmt::format(
            "Unexpected call to makeObjectDimensionId with a previously added dimension name '{}'",
            name));
  }
  return id;
}

DimensionId DimensionsBuilder::makeScopeDimensionId(
    const std::string& name,
    ScopeId scopeId) {
  const auto dimensionId = getOrMakeDimensionId(name);
  const auto registered =
      scopeDimensionIdToData_.registerKey(std::make_pair(dimensionId, scopeId));
  if (!registered) {
    throw std::runtime_error(
        fmt::format(
            "Unexpected call to makeScopeDimensionId with a previously added dimension name '{}'",
            name));
  }
  return dimensionId;
}

DimensionId DimensionsBuilder::getOrMakeDimensionId(const std::string& name) {
  return dimensionNameToId_.withWLock([&](auto& dimensionNameToId) {
    const auto dimPtr = folly::get_ptr(dimensionNameToId, name);
    if (dimPtr) {
      return *dimPtr;
    }

    auto [it, _] =
        dimensionNameToId.emplace(name, idBuilder_.next<DimensionId>());
    return it->second;
  });
}

folly::coro::Task<DimensionsBuilderResult> DimensionsBuilder::build() const {
  DimensionsBuilderResult result;

  auto objectDimRead =
      [&result](const DimensionId id, const ObjectDimensionData& data) {
        result.dimensionIdToObjectDimension.emplace(id, data.dimension);
      };
  auto scopeDimRead = [&result](
                          const DimensionScopeIdPair& idPair,
                          const ScopeDimensionData& data) {
    const auto& [dimensionId, scopeId] = idPair;
    result.scopeIdToDimensionIdToDimension[scopeId].emplace(
        dimensionId, data.dimension);
  };

  co_await folly::coro::collectAll(
      objectDimensionIdToData_.waitAndReadFromEach(objectDimRead),
      scopeDimensionIdToData_.waitAndReadFromEach(scopeDimRead));

  result.dimensionNameToId = *dimensionNameToId_.rlock();

  co_return result;
}

folly::coro::Task<std::string> DimensionsBuilder::summarize() const {
  const auto dimensionNames = dimensionNameToId_.rlock();
  if (dimensionNames->empty()) {
    co_return "";
  }

  Map<DimensionId, std::string> dimensionIdToName;
  for (const auto& [name, id] : *dimensionNames) {
    dimensionIdToName[id] = name;
  }

  std::string objectDimensions;
  for (const auto dimId : objectDimensionIdToData_.keys()) {
    if (const auto* name = folly::get_ptr(dimensionIdToName, dimId)) {
      objectDimensions += fmt::format("    {}\n", *name);
    }
  }

  std::string scopeDimensions;
  for (const auto& [dimId, _] : scopeDimensionIdToData_.keys()) {
    if (const auto* name = folly::get_ptr(dimensionIdToName, dimId)) {
      scopeDimensions += fmt::format("    {}\n", *name);
    }
  }

  co_return fmt::format(
      "Dimensions:\n"
      "  Object Dimensions:\n{}"
      "  Scope Dimensions:\n{}",
      objectDimensions,
      scopeDimensions);
}

} // namespace facebook::rebalancer::entities
