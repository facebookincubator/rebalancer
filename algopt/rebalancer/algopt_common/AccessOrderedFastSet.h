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

namespace facebook::algopt {

/**
AccessOrderedFastSet is a special kind of fast set that has the following
properties:

  i) it always maintains the order of insertion of all the (unerased) elements

  ii) during the (j+1)-th iteration over this set, the iteration resumes from
position p, where p was the first unincremented (and unerased) position from
iteration j and circles back to the most recently incremented position from
iteration j. Slightly more elaborately, suppose {e_1, e_2, ..., e_n} are the
elements in this set and j-th iteration over this set stopped after accessing
element e_k (here "stopped" means that there was an iterator increment after
e_{k} but there was no increment after). Then, (j+1)-th iteration will resume
at element e_{k+1}, the first unincremented position from iteration j
(assuming it wasn't erased after iteration j), go on until e_n, and then
circle back to e_1 and will eventually stop at e_k, the most the recently
incremented position from iteration j.

AccessOrderedFastSet supports insert(), erase(), and lookups (contains()) with
average case O(1), and it is implemented as a combination of std::list and
folly::F14FastMap. The latter is mainly used to speed-up erasing a given element
and the former is used because insert() and erase() do not invalidate any
iterators (except that of deleted elements). Note that std::list is a doubly
linked list and can add and erase iterators in O(1) time.

NOTE: it is NOT safe to iterate over this set while elements are being added or
erased.
*/

template <typename T>
class AccessOrderedFastSet : public AccessOrderedSetBase<T, std::list<T>> {
  using ListConstIterator = typename std::list<T>::const_iterator;
  using Base = AccessOrderedSetBase<T, std::list<T>>;

 public:
  explicit AccessOrderedFastSet() : Base(std::list<T>()) {
    populateElementToIterator();
  }

  AccessOrderedFastSet(const AccessOrderedFastSet<T>& other) : Base(other) {
    populateElementToIterator();
  }

  AccessOrderedFastSet(AccessOrderedFastSet<T>&& other) noexcept
      : Base(std::move(other)) {
    populateElementToIterator();
  }

  AccessOrderedFastSet& operator=(const AccessOrderedFastSet<T>& other) {
    Base::operator=(other);
    populateElementToIterator();
    return *this;
  }

  AccessOrderedFastSet& operator=(AccessOrderedFastSet<T>&& other) noexcept {
    Base::operator=(std::move(other));
    populateElementToIterator();
    return *this;
  }

  ~AccessOrderedFastSet() override = default;

 private:
  void insertImpl(const T& element) override {
    // ensure that there are no duplicates
    if (elementToIterator_.contains(element)) {
      return;
    }

    auto it =
        this->elementContainer_.insert(this->elementContainer_.end(), element);
    elementToIterator_.emplace(element, std::move(it));

    // after first element is inserted, change currIter to point to the first
    // element
    if (this->size() == 1) {
      this->setCurrIterToBegin();
    }
  }

  void eraseImpl(const ListConstIterator& toEraseIter, const T& element)
      override {
    this->elementContainer_.erase(toEraseIter);
    elementToIterator_.erase(element);
  }

  bool containsImpl(const T& element) const override {
    return elementToIterator_.contains(element);
  }

  const std::optional<ListConstIterator> find(const T& element) const override {
    auto iterPtr = folly::get_ptr(elementToIterator_, element);
    if (!iterPtr) {
      return std::nullopt;
    }
    return *iterPtr;
  }

  void populateElementToIterator() {
    elementToIterator_.reserve(this->elementContainer_.size());
    for (auto it = this->elementContainer_.begin();
         it != this->elementContainer_.end();
         ++it) {
      elementToIterator_.emplace(*it, it);
    }
  }

 private:
  folly::F14FastMap<T, const ListConstIterator> elementToIterator_;
};

} // namespace facebook::algopt
