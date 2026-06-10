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

#include "algopt/rebalancer/algopt_common/AssociativeFixedMap.h"
#include "algopt/rebalancer/algopt_common/AssociativeMap.h"

#include <functional>

namespace facebook::algopt {

// AssociativeHybridMap combines the superior performance of initialization
// of AssociativeFixedMap with the flexibility of adding and removing keys of
// AssociativeMap.
template <class K, class V>
class AssociativeHybridMap {
 public:
  AssociativeHybridMap(std::function<V(V, V)> operation, V neutral)
      : operation(operation),
        neutral(neutral),
        fixed(operation, neutral),
        overflow(operation, neutral) { // O(1)
  }

  void init(std::vector<std::pair<K, V>> items) { // O(n * log(n))
    fixed.init(std::move(items));
    overflow.clear();
  }

  void update(const K& key, V value) { // O(log(n))
    if (fixed.count(key)) {
      fixed.update(key, std::move(value));
    } else {
      overflow.update(key, std::move(value));
    }
  }

  void remove(const K& key) { // O(log(n))
    if (fixed.count(key)) {
      fixed.update(key, neutral);
    } else {
      overflow.remove(key);
    }
  }

  const V query() const { // O(1)
    return operation(fixed.query(), overflow.query());
  }

  // Get value of aggregating all keys less than k
  V query_lt(const K& key) const { // O(log(n))
    return operation(fixed.query_lt(key), overflow.query_lt(key));
  }

  // Get value of aggregating all keys less than or equal to k
  V query_le(const K& key) const { // O(log(n))
    return operation(fixed.query_le(key), overflow.query_le(key));
  }

 private:
  std::function<V(V, V)> operation;
  V neutral;
  AssociativeFixedMap<K, V> fixed;
  AssociativeMap<K, V> overflow;
};

template <class K, class V>
class SumHybridMap : public AssociativeHybridMap<K, V> {
 public:
  SumHybridMap() : AssociativeHybridMap<K, V>(std::plus<V>(), V(0)) {}
};

} // namespace facebook::algopt
