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

#include "algopt/rebalancer/algopt_common/Timer.h"

#include <iterator>
#include <memory>

namespace facebook::rebalancer {

template <class Collection>
class Timeout {
 public:
  using collection_iterator = typename Collection::const_iterator;
  using value_type = typename collection_iterator::value_type;

  class iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using value_type = typename collection_iterator::value_type;
    using difference_type = void;
    using pointer = value_type*;
    using reference = value_type&;

    iterator(
        collection_iterator current,
        collection_iterator on_timeout,
        double timeout,
        std::shared_ptr<const facebook::algopt::Timer> timer)
        : current_(std::move(current)),
          on_timeout_(std::move(on_timeout)),
          timeout_(timeout),
          timer_(timer) {
      if (timer_->getSeconds() >= timeout_) {
        current_ = on_timeout_;
      }
    }

    iterator& operator++() {
      if (timer_->getSeconds() < timeout_) {
        ++current_;
      } else {
        current_ = on_timeout_;
      }
      return *this;
    }

    bool operator==(const iterator& other) const {
      return current_ == other.current_;
    }

    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

    value_type operator*() const {
      return *current_;
    }

   private:
    collection_iterator current_;
    collection_iterator on_timeout_;
    double timeout_;
    std::shared_ptr<const facebook::algopt::Timer> timer_;
  };

  using const_iterator = iterator;

  Timeout(collection_iterator first, collection_iterator last, double timeout)
      : first_(std::move(first)),
        last_(std::move(last)),
        timeout_(timeout),
        timer_(std::make_shared<facebook::algopt::Timer>()) {}

  Timeout(const Collection& collection, double timeout)
      : Timeout(collection.begin(), collection.end(), timeout) {}

  void start_timer() {
    timer_->start();
  }

  iterator begin() const {
    return iterator(first_, last_, timeout_, timer_);
  }

  iterator end() const {
    return iterator(last_, last_, timeout_, timer_);
  }

 private:
  collection_iterator first_;
  collection_iterator last_;
  double timeout_;
  std::shared_ptr<facebook::algopt::Timer> timer_;
};

} // namespace facebook::rebalancer
