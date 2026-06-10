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

#include <iterator>

namespace facebook::rebalancer {

template <class T>
class Range {
 public:
  using value_type = T;

 public:
  class iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using value_type = T;
    using difference_type = T;
    using pointer = T*;
    using reference = T&;

    explicit iterator(T value) : value_(std::move(value)) {}

    iterator& operator++() {
      ++value_;
      return *this;
    }

    bool operator==(const iterator& other) const {
      return value_ == other.value_;
    }

    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

    T operator*() const {
      return value_;
    }

   private:
    T value_;
  };

  using const_iterator = iterator;

  Range(T first, T last) : first_(std::move(first)), last_(std::move(last)) {}

  iterator begin() const {
    return iterator(first_);
  }

  iterator end() const {
    return iterator(last_);
  }

 private:
  T first_;
  T last_;
};

} // namespace facebook::rebalancer
