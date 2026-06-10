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

#include <memory>

namespace facebook::rebalancer {

// Abstract iterator to be extended with different implementations.
template <typename Element>
class AbstractIteratorImpl {
 public:
  AbstractIteratorImpl() = default;

  AbstractIteratorImpl(const AbstractIteratorImpl&) = default;
  AbstractIteratorImpl(AbstractIteratorImpl&&) = default;
  AbstractIteratorImpl& operator=(const AbstractIteratorImpl&) = default;
  AbstractIteratorImpl& operator=(AbstractIteratorImpl&&) = default;
  virtual ~AbstractIteratorImpl() = default;

  virtual void operator++() = 0;

  virtual Element operator*() const = 0;

  virtual std::shared_ptr<AbstractIteratorImpl> clone() const = 0;

  bool operator==(const AbstractIteratorImpl& other) const {
    return typeid(*this) == typeid(other) && equal(other);
  }

 protected:
  virtual bool equal(const AbstractIteratorImpl& other) const = 0;
};

// Wrapper of AbstractIteratorImpl that can be allocated on the heap, copied,
// etc.
template <typename Element>
class AbstractIterator
    : public std::iterator<std::input_iterator_tag, Element> {
 public:
  explicit AbstractIterator(
      std::shared_ptr<AbstractIteratorImpl<Element>> iterator)
      : iterator_(std::move(iterator)) {}
  ~AbstractIterator() = default;

  AbstractIterator(const AbstractIterator& other)
      : AbstractIterator(other.iterator_->clone()) {}

  AbstractIterator& operator=(const AbstractIterator& other) {
    iterator_ = other.iterator_->clone();
    return *this;
  }

  AbstractIterator(AbstractIterator&&) = default;
  AbstractIterator& operator=(AbstractIterator&&) = default;

  AbstractIterator& operator++() {
    ++(*iterator_);
    return *this;
  }

  Element operator*() const {
    return *(*iterator_);
  }

  bool operator==(const AbstractIterator& other) const {
    return *iterator_ == *other.iterator_;
  }

  bool operator!=(const AbstractIterator& other) const {
    return !(*this == other);
  }

  std::shared_ptr<const AbstractIterator<Element>> get() const {
    return iterator_;
  }

 private:
  std::shared_ptr<AbstractIteratorImpl<Element>> iterator_;
};

// Container interface for the range between two given iterators.
template <typename Element>
class AbstractContainer {
 public:
  using const_iterator = AbstractIterator<Element>;

 public:
  AbstractContainer(
      std::shared_ptr<const AbstractIteratorImpl<Element>> begin,
      std::shared_ptr<const AbstractIteratorImpl<Element>> end)
      : begin_(std::move(begin)), end_(std::move(end)) {}

  AbstractIterator<Element> begin() const {
    return AbstractIterator<Element>(begin_->clone());
  }

  AbstractIterator<Element> end() const {
    return AbstractIterator<Element>(end_->clone());
  }

 private:
  std::shared_ptr<const AbstractIteratorImpl<Element>> begin_;
  std::shared_ptr<const AbstractIteratorImpl<Element>> end_;
};

} // namespace facebook::rebalancer
