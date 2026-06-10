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
#include <type_traits>

namespace facebook::rebalancer {

template <class Collection1, class Collection2>
class CartesianProduct {
 private:
  using collection1_iterator = typename Collection1::const_iterator;
  using collection2_iterator = typename Collection2::const_iterator;

  // Check if both iterators are random access
  static constexpr bool
      is_random_access = std::is_base_of_v<
                             std::random_access_iterator_tag,
                             typename std::iterator_traits<
                                 collection1_iterator>::iterator_category> &&
      std::is_base_of_v<std::random_access_iterator_tag,
                        typename std::iterator_traits<
                            collection2_iterator>::iterator_category>;

 public:
  // Random access iterator for when both collections support random access
  class random_access_iterator {
   public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::pair<
        typename std::iterator_traits<collection1_iterator>::value_type,
        typename std::iterator_traits<collection2_iterator>::value_type>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    random_access_iterator(
        collection1_iterator first1,
        collection1_iterator last1,
        collection2_iterator first2,
        collection2_iterator last2,
        difference_type index = 0)
        : first1_(first1),
          last1_(last1),
          first2_(first2),
          last2_(last2),
          index_(index),
          size2_(last2 - first2) {}

    random_access_iterator& operator++() {
      ++index_;
      return *this;
    }

    random_access_iterator operator++(int) {
      auto temp = *this;
      ++index_;
      return temp;
    }

    random_access_iterator& operator--() {
      --index_;
      return *this;
    }

    random_access_iterator operator--(int) {
      auto temp = *this;
      --index_;
      return temp;
    }

    random_access_iterator& operator+=(difference_type n) {
      index_ += n;
      return *this;
    }

    random_access_iterator& operator-=(difference_type n) {
      index_ -= n;
      return *this;
    }

    random_access_iterator operator+(difference_type n) const {
      return random_access_iterator(
          first1_, last1_, first2_, last2_, index_ + n);
    }

    random_access_iterator operator-(difference_type n) const {
      return random_access_iterator(
          first1_, last1_, first2_, last2_, index_ - n);
    }

    difference_type operator-(const random_access_iterator& other) const {
      return index_ - other.index_;
    }

    bool operator==(const random_access_iterator& other) const {
      return index_ == other.index_;
    }

    bool operator!=(const random_access_iterator& other) const {
      return index_ != other.index_;
    }

    bool operator<(const random_access_iterator& other) const {
      return index_ < other.index_;
    }

    bool operator<=(const random_access_iterator& other) const {
      return index_ <= other.index_;
    }

    bool operator>(const random_access_iterator& other) const {
      return index_ > other.index_;
    }

    bool operator>=(const random_access_iterator& other) const {
      return index_ >= other.index_;
    }

    value_type operator*() const {
      return std::make_pair(first1_[index_ / size2_], first2_[index_ % size2_]);
    }

    value_type operator[](difference_type n) const {
      difference_type abs_index = index_ + n;
      return std::make_pair(
          first1_[abs_index / size2_], first2_[abs_index % size2_]);
    }

   private:
    collection1_iterator first1_;
    collection1_iterator last1_;
    collection2_iterator first2_;
    collection2_iterator last2_;
    difference_type index_;
    difference_type size2_;
  };

  // Forward-only iterator for non-random access collections
  class forward_iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::pair<
        typename std::iterator_traits<collection1_iterator>::value_type,
        typename std::iterator_traits<collection2_iterator>::value_type>;
    using difference_type = void;
    using pointer = value_type*;
    using reference = value_type&;

    forward_iterator(
        collection1_iterator first1,
        collection1_iterator last1,
        collection2_iterator first2,
        collection2_iterator last2)
        : first1_(std::move(first1)),
          last1_(std::move(last1)),
          first2_(std::move(first2)),
          last2_(std::move(last2)),
          current1_(first1_),
          current2_(first2_) {}

    forward_iterator& operator++() {
      ++current2_;
      if (current2_ == last2_) {
        ++current1_;
        current2_ = first2_;
      }
      return *this;
    }

    bool operator==(const forward_iterator& other) const {
      if (done()) {
        return other.done();
      }
      return current1_ == other.current1_ && current2_ == other.current2_;
    }

    bool operator!=(const forward_iterator& other) const {
      return !(*this == other);
    }

    value_type operator*() const {
      return std::make_pair(*current1_, *current2_);
    }

   private:
    bool done() const {
      return current1_ == last1_ || current2_ == last2_;
    }

    collection1_iterator first1_;
    collection1_iterator last1_;
    collection2_iterator first2_;
    collection2_iterator last2_;

    collection1_iterator current1_;
    collection2_iterator current2_;
  };

  // Choose iterator type based on collection capabilities
  using iterator = std::
      conditional_t<is_random_access, random_access_iterator, forward_iterator>;

  using const_iterator = iterator;

  CartesianProduct(
      collection1_iterator first1,
      collection1_iterator last1,
      collection2_iterator first2,
      collection2_iterator last2)
      : first1_(std::move(first1)),
        last1_(std::move(last1)),
        first2_(std::move(first2)),
        last2_(std::move(last2)) {}

  CartesianProduct(
      const Collection1& collection1,
      const Collection2& collection2)
      : CartesianProduct(
            collection1.begin(),
            collection1.end(),
            collection2.begin(),
            collection2.end()) {}

  iterator begin() const {
    return iterator(first1_, last1_, first2_, last2_);
  }

  iterator end() const {
    if constexpr (is_random_access) {
      const auto size1 = last1_ - first1_;
      const auto size2 = last2_ - first2_;
      return iterator(first1_, last1_, first2_, last2_, size1 * size2);
    } else {
      return iterator(last1_, last1_, last2_, last2_);
    }
  }

  // Get total size (only available for random access collections)
  template <bool B = is_random_access>
  typename std::enable_if_t<B, std::size_t> size() const {
    return (last1_ - first1_) * (last2_ - first2_);
  }

 private:
  collection1_iterator first1_;
  collection1_iterator last1_;
  collection2_iterator first2_;
  collection2_iterator last2_;
};
} // namespace facebook::rebalancer
