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

#include <folly/hash/Hash.h>

namespace std {
template <class T>
struct hash<vector<T>> {
  size_t operator()(const vector<T>& k) const noexcept {
    size_t val = 0;
    for (auto& it : k) {
      val = hash<size_t>{}(hash<T>{}(it) ^ val);
    }
    return val;
  }
};
} // namespace std

namespace facebook {
namespace rebalancer {

template <class ObjectToValueMap>
void EquivalenceSets::mappingMerge(const ObjectToValueMap& mapping) {
  using ValueT = typename ObjectToValueMap::mapped_type;
  if constexpr (requires { mapping.nonDefaultSize(); }) {
    if (mapping.nonDefaultSize() == 0) {
      return;
    }
  } else {
    if (mapping.empty()) {
      return;
    }
  }
  std::vector<std::vector<entities::ObjectId>> eqSets;
  PackerMap<ValueT, int> valueToEqSetId;
  algopt::utils::forEachNonDefault(
      mapping, [&](const auto& obj, const auto& val) {
        const auto [it, inserted] =
            valueToEqSetId.try_emplace(val, eqSets.size());
        if (inserted) {
          // first time seeing this value, create a new equivalence set
          eqSets.emplace_back();
        }
        // it->second is the index of the equivalence set for this value
        eqSets[it->second].emplace_back(obj);
      });

  for (const auto& eqSet : eqSets) {
    combine(eqSet);
  }
}

template <class ObjectToValueMap>
void EquivalenceSets::mappingMerge(
    ExprId exprId,
    const ObjectToValueMap& mapping) {
  if (!seenExprIds_.insert(exprId).second) {
    return;
  }
  mappingMerge(mapping);
}
} // namespace rebalancer
} // namespace facebook
