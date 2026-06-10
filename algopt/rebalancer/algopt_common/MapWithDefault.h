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

#include "folly/container/MapUtil.h"

#include <optional>

namespace facebook::algopt {

template <typename Map>
class MapWithDefault {
 public:
  using Key = typename Map::key_type;
  using Value = typename Map::mapped_type;

  explicit MapWithDefault(std::optional<Value> defaultValue)
      : defaultValue_(std::move(defaultValue)) {}

  /* implicit */ MapWithDefault(Map map) : map_(std::move(map)) {}

  /* implicit */ MapWithDefault(
      std::initializer_list<std::pair<Key, Value>>&& init) {
    map_.insert(
        std::move_iterator(init.begin()), std::move_iterator(init.end()));
  }

  MapWithDefault(Map map, Value defaultValue)
      : map_(std::move(map)), defaultValue_(std::move(defaultValue)) {}

  MapWithDefault& operator=(Map&& map) {
    map_ = std::move(map);
    return *this;
  }

  decltype(auto) at(const Key& key) const {
    return defaultValue_ ? folly::get_ref_default(map_, key, *defaultValue_)
                         : map_.at(key);
  }

  decltype(auto) operator[](Key&& key) {
    // if there is no defaultValue or map already contains the key, then just
    // use map's [] operator
    // if there is a defaultValue and map does not contain the key, then insert
    // defaultValue to be consitent with at()
    if (defaultValue_.has_value() && !map_.contains(key)) {
      return map_[std::forward<Key>(key)] = defaultValue_.value();
    }

    return map_[std::forward<Key>(key)];
  }

  const Value* getPtr(const Key& key) const {
    return defaultValue_ ? &folly::get_ref_default(map_, key, *defaultValue_)
                         : folly::get_ptr(map_, key);
  }

  const std::optional<Value>& getDefault() const {
    return defaultValue_;
  }

  const Map& getMap() const {
    return map_;
  }

 private:
  Map map_;
  std::optional<Value> defaultValue_ = std::nullopt;
};

} // namespace facebook::algopt
