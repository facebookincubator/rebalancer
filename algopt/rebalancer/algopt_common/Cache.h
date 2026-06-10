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

#include <folly/container/MapUtil.h>

#include <optional>

namespace facebook::algopt {

template <typename Map>
class Cache {
 public:
  using Key = typename Map::key_type;
  using Value = typename Map::mapped_type;

  template <class Function>
  const Value& getSavedOrCompute(const Key& key, const Function& onMiss) {
    auto valuePtr = folly::get_ptr(cache_, key);
    if (valuePtr) {
      return *valuePtr;
    }
    return cache_.emplace(key, onMiss()).first->second;
  }

  // decltype(auto) because it automatically returns a const& if the underlying
  // map's at() returns a const& or a copy if its at() returns a copy
  decltype(auto) at(const Key& key) const {
    return cache_.at(key);
  }

  std::optional<Value> atOrNullopt(const Key& key) const {
    auto it = cache_.find(key);
    if (it == cache_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  bool update(const Key& key, const Value& val) {
    return cache_.insert_or_assign(key, val).second;
  }

 private:
  Map cache_;
};

} // namespace facebook::algopt
