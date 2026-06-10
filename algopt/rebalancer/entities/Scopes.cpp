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

#include "algopt/rebalancer/entities/Scopes.h"

namespace facebook::rebalancer::entities {

Scopes::Scopes(Map<ScopeId, std::unique_ptr<Scope>> scopes)
    : scopes_(std::move(scopes)) {}

Scopes::Scopes(const thrift::Scopes& scopes) {
  scopes_.reserve(scopes.scopes()->size());
  for (const auto& [scopeId, scope] : *scopes.scopes()) {
    scopes_.emplace(ScopeId(scopeId), std::make_unique<Scope>(scope));
  }
}

const Scope& Scopes::getScope(ScopeId scopeId) const {
  return *scopes_.at(scopeId);
}

thrift::Scopes Scopes::toThrift() const {
  thrift::Scopes scopes;
  auto& thriftScopes = *scopes.scopes();
  thriftScopes.reserve(scopes_.size());
  for (const auto& [scopeId, scope] : scopes_) {
    thriftScopes.emplace(scopeId.asInt(), scope->toThrift());
  }
  return scopes;
}

} // namespace facebook::rebalancer::entities
