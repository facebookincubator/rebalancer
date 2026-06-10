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
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"

namespace facebook {
namespace rebalancer {
namespace entities {

class ScopeDimension {
 public:
  ScopeDimension(Map<ScopeItemId, double> values, double defaultValue);
  explicit ScopeDimension(const thrift::ScopeDimension& scopeDimension);

  // Delete the copy and assignment constructors to prevent accidental copies.
  ScopeDimension(const ScopeDimension&) = delete;
  ScopeDimension& operator=(const ScopeDimension&) = delete;
  // Other operators are as usual.
  ScopeDimension(ScopeDimension&&) = default;
  ScopeDimension& operator=(ScopeDimension&&) = default;
  ~ScopeDimension() = default;

  double getValue(ScopeItemId scopeItemId) const;
  const Map<ScopeItemId, double>& getNonDefaultValues() const;
  double getDefaultValue() const;

  thrift::ScopeDimension toThrift() const;

 private:
  Map<ScopeItemId, double> values_;
  double defaultValue_;
};

} // namespace entities
} // namespace rebalancer
} // namespace facebook
