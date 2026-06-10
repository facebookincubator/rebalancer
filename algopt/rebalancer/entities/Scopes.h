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
#include "algopt/rebalancer/entities/Scope.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"

#include <memory>

namespace facebook::rebalancer::entities {

class Scopes {
 public:
  Scopes() = default;
  explicit Scopes(Map<ScopeId, std::unique_ptr<Scope>> scopes);
  explicit Scopes(const thrift::Scopes& scopes);

  // Delete the copy and assignment constructors to prevent accidental copies.
  Scopes(const Scopes&) = delete;
  Scopes& operator=(const Scopes&) = delete;
  // Other operators are as usual.
  Scopes(Scopes&&) = default;
  Scopes& operator=(Scopes&&) = default;
  ~Scopes() = default;

  auto getScopeIds() const {
    return std::views::keys(scopes_);
  }

  const Scope& getScope(ScopeId scopeId) const;

  thrift::Scopes toThrift() const;

 private:
  Map<ScopeId, std::unique_ptr<Scope>> scopes_;
};

} // namespace facebook::rebalancer::entities
