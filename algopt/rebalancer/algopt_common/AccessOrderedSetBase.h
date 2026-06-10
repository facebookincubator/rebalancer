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

#include <folly/container/F14Map.h>
#include <folly/MapUtil.h>
#include <folly/SynchronizedPtr.h>

#include <list>
#include <memory>
#include <set>

namespace facebook::algopt {

template <typename T, typename Container>
concept IsListContainer =
    std::same_as<typename Container::iterator, typename std::list<T>::iterator>;

template <typename T, typename Container>
concept IsSetContainer = requires {
  typename Container::key_compare;
  requires std::same_as<
      typename Container::iterator,
      typename std::set<T, typename Container::key_compare>::iterator>;
};

template <typename T, typename Container>
concept StableIterContainers =
    IsListContainer<T, Container> || IsSetContainer<T, Container>;

// This is a base class for AccessOrderedFastSet and AccessOrderedSortedSet. See
// comments in those files for more details.
template <typename T, typename Container>
  requires StableIterContainers<T, Container>
class AccessOrderedSetBase {
  using ContainerConstIterator = typename Container::const_iterator;
  using ContainerConstIteratorPtr =
      std::shared_ptr<typename Container::const_iterator>;

 public:
  class Iterator;
  using const_iterator = Iterator;

  void insert(const T& element) {
    return insertImpl(element);
  }

  void erase(const T& element) {
    const auto toEraseIterPtr = find(element);
    if (!toEraseIterPtr) {
      // element not in elementContainer
      return;
    }

    updateCurrIterBeforeErase(*toEraseIterPtr);
    eraseImpl(*toEraseIterPtr, element);

    return;
  }

  bool contains(const T& element) const {
    return containsImpl(element);
  }

  Iterator begin() const {
    return Iterator(*currIter_.rlock(), size(), this);
  }

  Iterator end() const {
    return Iterator(
        elementContainer_.end(), /*nElementsToIterateOver=*/0, this);
  }

  size_t size() const {
    return elementContainer_.size();
  }

  bool empty() const {
    return elementContainer_.empty();
  }

 protected:
  virtual void insertImpl(const T& element) = 0;
  virtual void eraseImpl(
      const ContainerConstIterator& toEraseIter,
      const T& element) = 0;
  virtual bool containsImpl(const T& element) const = 0;
  virtual const std::optional<ContainerConstIterator> find(
      const T& element) const = 0;

  void updateCurrIterBeforeErase(const ContainerConstIterator& toEraseIter) {
    currIter_.withWLock([&toEraseIter, this](auto& wLockedCurrIter) {
      // if currIter_ points to an element that is going to be erased, then we
      // need to change it before it gets invalidated
      if (toEraseIter == wLockedCurrIter) {
        auto nextIter = std::next(toEraseIter);
        if (nextIter == elementContainer_.end() && size() >= 2) {
          // if the last element is going to be erased, and there is at least
          // one other element (i.e., size() >= 2), then set currIter_ to the
          // first element
          wLockedCurrIter = elementContainer_.begin();
        } else {
          // implies that either the container is going to be empty after
          // deletion or there is a valid next element that will not be erased
          wLockedCurrIter = nextIter;
        }
      }
    });
  }

  // constructors and assignment operators
  explicit AccessOrderedSetBase(Container elementContainer)
      : elementContainer_(std::move(elementContainer)) {
    setCurrIterToBegin();
  }

  AccessOrderedSetBase(const AccessOrderedSetBase<T, Container>& other)
      : elementContainer_(other.elementContainer_) {
    setCurrIterToBegin();
  }

  AccessOrderedSetBase(AccessOrderedSetBase<T, Container>&& other) noexcept
      : elementContainer_(std::move(other.elementContainer_)) {
    setCurrIterToBegin();
  }

  AccessOrderedSetBase& operator=(const AccessOrderedSetBase& other) {
    elementContainer_ = other.elementContainer_;
    setCurrIterToBegin();
    return *this;
  }

  AccessOrderedSetBase& operator=(AccessOrderedSetBase&& other) noexcept {
    elementContainer_ = std::move(other.elementContainer_);
    setCurrIterToBegin();
    return *this;
  }

  virtual ~AccessOrderedSetBase() = default;

  void setCurrIterToBegin() {
    // note that when elementContainer_ is empty, begin() = end(); otherwise
    // we set currIter to the first element
    setCurrIter(elementContainer_.begin());
  }

 private:
  void setCurrIter(ContainerConstIterator inner) const {
    currIter_.withWLock(
        [&](auto& wLockedCurrIter) { wLockedCurrIter = inner; });
  }

 protected:
  Container elementContainer_;
  // mutable because each time begin() is called, we get
  // AccessOrderedSetBase::Iterator, and when the iterator is incremented
  // currIter_ is eventually updated
  mutable folly::Synchronized<ContainerConstIterator> currIter_;
};

template <typename T, typename Container>
  requires StableIterContainers<T, Container>
class AccessOrderedSetBase<T, Container>::Iterator
    : public std::iterator<std::input_iterator_tag, T> {
 public:
  Iterator(
      ContainerConstIterator innerIter,
      size_t nElementsToIterateOver,
      const AccessOrderedSetBase<T, Container>* accessOrderedSetPtr)
      : innerIter_(innerIter),
        nElementsToIterateOver_(nElementsToIterateOver),
        accessOrderedSetPtr_(accessOrderedSetPtr) {}

  ~Iterator() {
    // it is safe to just update the "where-i-was" iterator (i.e., currentIter)
    // once the iterator is "done" iterating. Notifying on each increment is
    // expensive because of cost of acquiring a mutex.
    if (wasIncremented_ && nElementsToIterateOver_ > 0) {
      // iterator was incremented and did not look at all the elements,
      // so we need to update the "where-i-was" iterator (i.e., currentIterator)
      // in the parent datastructure
      accessOrderedSetPtr_->setCurrIter(innerIter_);
    }
  }

  Iterator& operator++() {
    --nElementsToIterateOver_;

    wasIncremented_ = true;

    // update innerIter_ and  circle back to begin if we reach end
    ++innerIter_;
    if (innerIter_ == accessOrderedSetPtr_->elementContainer_.end()) {
      innerIter_ = accessOrderedSetPtr_->elementContainer_.begin();
    }
    return *this;
  }

  T operator*() const {
    return *innerIter_;
  }

  bool operator==(const Iterator& other) const {
    return (
        nElementsToIterateOver_ == other.nElementsToIterateOver_ &&
        accessOrderedSetPtr_ == other.accessOrderedSetPtr_);
  }

  bool operator!=(const Iterator& other) const {
    return !operator==(other);
  }

  Iterator(const Iterator& other)
      : Iterator(
            other.innerIter_,
            other.nElementsToIterateOver_,
            other.accessOrderedSetPtr_) {}

  Iterator& operator=(const Iterator& other) {
    innerIter_ = other.innerIter_;
    nElementsToIterateOver_ = other.nElementsToIterateOver_;
    accessOrderedSetPtr_ = other.accessOrderedSetPtr_;
    return *this;
  }

  Iterator& operator=(Iterator&& other) = default;
  Iterator(Iterator&& other) = default;

 private:
  ContainerConstIterator innerIter_;
  size_t nElementsToIterateOver_;
  const AccessOrderedSetBase<T, Container>* accessOrderedSetPtr_;
  bool wasIncremented_ = false;
};

} // namespace facebook::algopt
