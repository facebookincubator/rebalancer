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

#include <functional>
#include <iterator>

namespace facebook::rebalancer {

// Map-union iterator: given 2 ordered maps and a default value, it traverses
// the union in order. It yields triplets {key, value1, value2} ordered by key.
template <class MapIterator>
class MapUnion {
 public:
  using map_key_type = typename MapIterator::value_type::first_type;
  using map_value_type = typename MapIterator::value_type::second_type;

  class iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::tuple<map_key_type, map_value_type, map_value_type>;
    using difference_type = void;
    using pointer = value_type*;
    using reference = value_type&;

    iterator(
        MapIterator first1,
        MapIterator last1,
        MapIterator first2,
        MapIterator last2,
        map_value_type defaultValue,
        std::function<bool(map_key_type, map_key_type)> lessThan)
        : first1_(std::move(first1)),
          last1_(std::move(last1)),
          first2_(std::move(first2)),
          last2_(std::move(last2)),
          defaultValue_(std::move(defaultValue)),
          lessThan_(std::move(lessThan)),
          current1_(first1_),
          current2_(first2_) {}

    iterator& operator++() {
      if (current1_ == last1_) {
        ++current2_;
      } else if (current2_ == last2_) {
        ++current1_;
      } else {
        auto& key1 = current1_->first;
        auto& key2 = current2_->first;
        if (lessThan_(key1, key2)) {
          ++current1_;
        } else if (lessThan_(key2, key1)) {
          ++current2_;
        } else {
          // key 1 == key2
          ++current1_;
          ++current2_;
        }
      }
      return *this;
    }

    bool operator==(const iterator& other) const {
      return current1_ == other.current1_ && current2_ == other.current2_;
    }

    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

    value_type operator*() const {
      if (current1_ == last1_) {
        return std::make_tuple(
            current2_->first, defaultValue_, current2_->second);
      } else if (current2_ == last2_) {
        return std::make_tuple(
            current1_->first, current1_->second, defaultValue_);
      } else {
        auto& [key1, value1] = *current1_;
        auto& [key2, value2] = *current2_;
        if (lessThan_(key1, key2)) {
          return std::make_tuple(key1, value1, defaultValue_);
        } else if (lessThan_(key2, key1)) {
          return std::make_tuple(key2, defaultValue_, value2);
        } else {
          // key 1 == key2
          return std::make_tuple(key1, value1, value2);
        }
      }
    }

   private:
    MapIterator first1_;
    MapIterator last1_;
    MapIterator first2_;
    MapIterator last2_;
    map_value_type defaultValue_;
    std::function<bool(map_key_type, map_key_type)> lessThan_;

    MapIterator current1_;
    MapIterator current2_;
  };

  using const_iterator = iterator;

  MapUnion(
      MapIterator first1,
      MapIterator last1,
      MapIterator first2,
      MapIterator last2,
      map_value_type defaultValue,
      std::function<bool(map_key_type, map_key_type)> lessThan)
      : first1_(std::move(first1)),
        last1_(std::move(last1)),
        first2_(std::move(first2)),
        last2_(std::move(last2)),
        defaultValue_(std::move(defaultValue)),
        lessThan_(std::move(lessThan)) {}

  iterator begin() const {
    return iterator(first1_, last1_, first2_, last2_, defaultValue_, lessThan_);
  }

  iterator end() const {
    return iterator(last1_, last1_, last2_, last2_, defaultValue_, lessThan_);
  }

 private:
  MapIterator first1_;
  MapIterator last1_;
  MapIterator first2_;
  MapIterator last2_;
  map_value_type defaultValue_;
  std::function<bool(map_key_type, map_key_type)> lessThan_;
};

} // namespace facebook::rebalancer
