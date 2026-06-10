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

template <class Collection>
class Filter {
 public:
  using collection_iterator = typename Collection::const_iterator;
  using filter_function =
      std::function<bool(const typename collection_iterator::value_type&)>;

  class iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using value_type = typename collection_iterator::value_type;
    using difference_type = void;
    using pointer = value_type*;
    using reference = value_type&;

    iterator(
        collection_iterator first,
        collection_iterator last,
        filter_function accept)
        : first_(std::move(first)),
          last_(std::move(last)),
          accept_(std::move(accept)) {
      filter();
    }

    iterator& operator++() {
      ++first_;
      filter();
      return *this;
    }

    bool operator==(const iterator& other) const {
      return first_ == other.first_;
    }

    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

    value_type operator*() const {
      return *first_;
    }

   private:
    void filter() {
      while (first_ != last_ && !accept_(*first_)) {
        ++first_;
      }
    }

    collection_iterator first_;
    collection_iterator last_;
    filter_function accept_;
  };

  using const_iterator = iterator;

  Filter(
      collection_iterator first,
      collection_iterator last,
      filter_function accept)
      : first_(std::move(first)),
        last_(std::move(last)),
        accept_(std::move(accept)) {}

  Filter(const Collection& collection, filter_function accept)
      : Filter(collection.begin(), collection.end(), std::move(accept)) {}

  iterator begin() const {
    return iterator(first_, last_, accept_);
  }

  iterator end() const {
    return iterator(last_, last_, accept_);
  }

 private:
  collection_iterator first_;
  collection_iterator last_;
  filter_function accept_;
};

} // namespace facebook::rebalancer
