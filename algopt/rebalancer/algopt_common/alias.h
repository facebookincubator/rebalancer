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

#include <ostream>

#ifndef REBALANCER_OSS_BUILD
#include "multifeed/shared/MFHash.h"
#else
#include <folly/container/F14Map.h>
#include <folly/container/F14Set.h>
#endif

namespace facebook::algopt {

#ifndef REBALANCER_OSS_BUILD
template <class Key, class Value>
using MapImpl = multifeed::MFHashMap<Key, Value>;

template <class Value>
using SetImpl = multifeed::MFHashSet<Value>;
#else
template <class Key, class Value>
using MapImpl = folly::F14FastMap<Key, Value>;

template <class Value>
using SetImpl = folly::F14FastSet<Value>;
#endif

template <class K, class V>
std::ostream& operator<<(std::ostream& os, const MapImpl<K, V>& m) {
  os << "{";
  bool first = true;
  for (const auto& p : m) {
    if (first) {
      first = false;
    } else {
      os << ", ";
    }
    os << p.first << ": " << p.second;
  }
  os << "}";
  return os;
}

template <class T>
std::ostream& operator<<(std::ostream& os, const SetImpl<T>& m) {
  os << "[";
  bool first = true;
  for (const auto& p : m) {
    if (first) {
      first = false;
    } else {
      os << ", ";
    }
    os << p;
  }
  os << "]";
  return os;
}

} // namespace facebook::algopt
