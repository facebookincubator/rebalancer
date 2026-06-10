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

#include "algopt/rebalancer/entities/ScopeDimension.h"

#include <folly/container/MapUtil.h>

namespace facebook {
namespace rebalancer {
namespace entities {

ScopeDimension::ScopeDimension(
    Map<ScopeItemId, double> values,
    double defaultValue)
    : values_(std::move(values)), defaultValue_(defaultValue) {}

ScopeDimension::ScopeDimension(const thrift::ScopeDimension& scopeDimension)
    : defaultValue_(*scopeDimension.defaultValue()) {
  for (auto& [scopeItemId, value] : *scopeDimension.values()) {
    values_.emplace(ScopeItemId(scopeItemId), value);
  }
}

double ScopeDimension::getValue(ScopeItemId scopeItemId) const {
  return folly::get_default(values_, scopeItemId, defaultValue_);
}

const Map<ScopeItemId, double>& ScopeDimension::getNonDefaultValues() const {
  return values_;
}

double ScopeDimension::getDefaultValue() const {
  return defaultValue_;
}

thrift::ScopeDimension ScopeDimension::toThrift() const {
  thrift::ScopeDimension scopeDimension;
  scopeDimension.values() = asIntsMap<folly::F14FastMap<int, double>>(values_);
  scopeDimension.defaultValue() = defaultValue_;
  return scopeDimension;
}

} // namespace entities
} // namespace rebalancer
} // namespace facebook
