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

#include <map>
#include <memory>

namespace facebook::rebalancer::entities::tests {

template <class Map>
std::map<typename Map::key_type, typename Map::mapped_type> toStdMap(
    const Map& x) {
  return std::map<typename Map::key_type, typename Map::mapped_type>(
      x.begin(), x.end());
}

inline ObjectIdToDoubleMap makeObjectIdToDoubleMap(
    const Map<ObjectId, double>& values,
    double defaultValue,
    std::size_t totalSize) {
  return ObjectIdToDoubleMap(values, defaultValue, totalSize);
}

inline std::shared_ptr<const ObjectIdToDoubleMap> makeSharedObjectIdToDoubleMap(
    const Map<ObjectId, double>& values,
    double defaultValue,
    std::size_t totalSize) {
  return std::make_shared<const ObjectIdToDoubleMap>(
      makeObjectIdToDoubleMap(values, defaultValue, totalSize));
}

} // namespace facebook::rebalancer::entities::tests
