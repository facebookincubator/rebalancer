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

#include <functional>
#include <set>
#include <utility>

namespace facebook::algopt {

// InsertOrderedFastSet is similar to an unordered set, but with the additional
// property that the insertion order is maintained and no erase is allowed. It
// is implemented as a combination of a vector and a fast set.
template <class K>
class InsertOrderedFastSet {
 public:
  using const_iterator = typename std::vector<K>::const_iterator;

  InsertOrderedFastSet() = default;

  bool contains(const K& key) const {
    return set_.contains(key);
  }

  const_iterator begin() const {
    return vec_.cbegin();
  }

  const_iterator end() const {
    return vec_.cend();
  }

  size_t size() const {
    return vec_.size();
  }

  void emplace(const K& key) {
    set_.emplace(key);
    vec_.push_back(key);
  }

 private:
  SetImpl<K> set_;
  std::vector<K> vec_;
};

} // namespace facebook::algopt
