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

#include "algopt/rebalancer/algopt_common/AccessOrderedSetBase.h"

#include <folly/container/F14Map.h>
#include <folly/MapUtil.h>

#include <set>

namespace facebook::algopt {

/**
AccessOrderedSortedSet<T, Comparator> is a wrapper around std::set that has the
following properties:

  i) like std::set, it always maintains the order of elements as determined by
the Comparator

  ii) If there was no insertion into this set between iteration j and iteration
(j+1), then during the (j+1)-th iteration over this set, the iteration resumes
from position p, where p was the first unincremented (and unerased) position
from iteration j and circles back to the most recently incremented position from
iteration j. Slightly more elaborately, suppose {e_1, e_2, ..., e_n} are the
elements in this set and j-th iteration over this set stopped after accessing
element e_k (here "stopped" means that there was an iterator increment after
e_{k} but there was no increment after). Then, (j+1)-th iteration will resume at
element e_{k+1}, the first unincremented position from iteration j (assuming it
wasn't erased after iteration j), go on until e_n, and then circle back to e_1
and will eventually stop at e_k, the most the recently incremented position from
iteration j.

  iii) If there was an insertion into this set between iteration j and iteration
(j+1), then the iteration restarts from the beginning of the set.

AccessOrderedSortedSet supports insert(), erase(), and lookups (contains()) with
with same asymptotic complexity as std::set (i.e., O(log n)).

NOTE: it is NOT safe to iterate over this set while elements are being added or
erased.
*/

template <typename T, class Comparator>
class AccessOrderedSortedSet
    : public AccessOrderedSetBase<T, std::set<T, Comparator>> {
  using SetConstIterator = typename std::set<T, Comparator>::const_iterator;
  using Base = AccessOrderedSetBase<T, std::set<T, Comparator>>;

 public:
  explicit AccessOrderedSortedSet(const Comparator& comparator)
      : Base(std::set<T, Comparator>(comparator)) {}

  AccessOrderedSortedSet(const AccessOrderedSortedSet<T, Comparator>& other)
      : Base(other) {}

  AccessOrderedSortedSet(AccessOrderedSortedSet<T, Comparator>&& other) noexcept
      : Base(std::move(other)) {}

  AccessOrderedSortedSet& operator=(
      const AccessOrderedSortedSet<T, Comparator>& other) {
    Base::operator=(other);
    return *this;
  }

  AccessOrderedSortedSet& operator=(
      AccessOrderedSortedSet<T, Comparator>&& other) noexcept {
    Base::operator=(std::move(other));
    return *this;
  }

  ~AccessOrderedSortedSet() override = default;

 private:
  void insertImpl(const T& element) override {
    auto [_, inserted] = this->elementContainer_.insert(element);
    if (inserted) {
      // always reset the current iterator to the begin after insertion
      this->setCurrIterToBegin();
    }
  }

  void eraseImpl(const SetConstIterator& toEraseIter, const T& /*element*/)
      override {
    this->elementContainer_.erase(toEraseIter);
  }

  bool containsImpl(const T& element) const override {
    return this->elementContainer_.contains(element);
  }

  const std::optional<SetConstIterator> find(const T& element) const override {
    auto it = this->elementContainer_.find(element);
    if (it == this->elementContainer_.end()) {
      return std::nullopt;
    }

    return it;
  }
};

} // namespace facebook::algopt
