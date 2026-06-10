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

#include "algopt/rebalancer/algopt_common/alias.h"

#include <folly/container/MapUtil.h>

#include <functional>
#include <initializer_list>
#include <set>
#include <utility>

namespace facebook::algopt {

// ValueSortedMap is a key-value map which can be iterated efficiently using
// a user-defined order of key-values.
template <class K, class V, class Compare = std::less<std::pair<K, V>>>
class ValueSortedMap {
 public:
  using const_iterator =
      typename std::set<std::pair<K, V>, Compare>::const_iterator;
  using const_reverse_iterator =
      typename std::set<std::pair<K, V>, Compare>::const_reverse_iterator;
  using key_type = K;
  using mapped_type = V;

  ValueSortedMap() = default;

  explicit ValueSortedMap(std::initializer_list<std::pair<K, V>> initList) {
    for (auto& [key, value] : initList) {
      assign(key, value);
    }
  }

  bool contains(const K& key) const {
    return map.contains(key);
  }

  const V& at(const K& key) const {
    return map.at(key);
  }

  const_iterator begin() const {
    return set.begin();
  }

  const_reverse_iterator rbegin() const {
    return set.rbegin();
  }

  const_iterator end() const {
    return set.end();
  }

  const_reverse_iterator rend() const {
    return set.rend();
  }

  size_t size() const {
    return map.size();
  }

  const V* FOLLY_NULLABLE getPtrIfExists(const K& key) const {
    return folly::get_ptr(map, key);
  }

  bool operator==(const ValueSortedMap& other) const {
    return set == other.set;
  }

  void clear() {
    map.clear();
    set.clear();
  }

  void erase(const K& key) {
    auto it = map.find(key);
    if (it != map.end()) {
      set.erase(*it);
      map.erase(it);
    }
  }

  void assign(const K& key, V value) {
    auto it = map.find(key);
    if (it != map.end()) {
      if constexpr (std::is_copy_assignable_v<V>) {
        // extract from set which is expected to have the key-value pair
        auto handle = set.extract(*it);
        handle.value().second = value;
        set.insert(std::move(handle));
      } else {
        set.erase(*it);
        set.emplace(key, value);
      }

      if constexpr (std::is_move_assignable_v<V>) {
        it->second = std::move(value);
      } else {
        map.erase(it);
        map.emplace(key, std::move(value));
      }
    } else {
      set.emplace(key, value);
      map.emplace(key, std::move(value));
    }
  }

 private:
  MapImpl<K, V> map;
  std::set<std::pair<K, V>, Compare> set;
};

} // namespace facebook::algopt
