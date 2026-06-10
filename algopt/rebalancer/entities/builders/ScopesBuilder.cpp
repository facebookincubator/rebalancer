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

#include "algopt/rebalancer/entities/builders/ScopesBuilder.h"

#include <fmt/core.h>

namespace facebook::rebalancer::entities {

namespace {
constexpr static std::string_view kMakeScopeId{"makeScopeId"};
}

ScopeId ScopesBuilder::makeScopeId(const std::string& scopeName) {
  return idBuilder_.makeIdFromName(
      scopeName, scopeNameToId_, scopeIdToData_, kMakeScopeId);
}

folly::coro::Task<ScopesBuilderResult> ScopesBuilder::build() const {
  ScopesBuilderResult result;
  co_await scopeIdToData_.waitAndReadFromEach(
      [&](ScopeId id, const ScopeData& data) {
        result.scopeIdToScopeItemNameToId.emplace(id, data.scopeItemNameToId);
        result.scopeIdToScopeItemIdToContainerIds.emplace(
            id, data.scopeItemIdToContainerIds);
      });
  result.scopeNameToId = *scopeNameToId_.rlock();

  co_return result;
}

folly::coro::Task<std::string> ScopesBuilder::summarize() const {
  const auto scopeNames = scopeNameToId_.rlock();
  if (scopeNames->empty()) {
    co_return "";
  }
  std::string result = "Scopes:\n";
  for (const auto& [scopeName, scopeId] : *scopeNames) {
    const auto data = co_await scopeIdToData_.get(scopeId);
    result += fmt::format("  {} [", scopeName);
    for (const auto& [itemName, _] : *data->scopeItemNameToId) {
      result += fmt::format(" {}", itemName);
    }
    result += " ]\n";
  }
  co_return result;
}

} // namespace facebook::rebalancer::entities
