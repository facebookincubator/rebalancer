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

#include "algopt/rebalancer/solver/iterators/Abstract.h"

namespace facebook::rebalancer {

// Implementation of an AbstractIterator which wraps an STL-compliant iterator.
template <typename Container, typename Iterator>
class StlWrapperIterator
    : public AbstractIteratorImpl<typename Container::value_type> {
 public:
  StlWrapperIterator(
      std::shared_ptr<const Container> container,
      Iterator iterator)
      : container_(container), iterator_(iterator) {}

  void operator++() {
    ++iterator_;
  }

  typename Container::value_type operator*() const {
    return *iterator_;
  }

  std::shared_ptr<AbstractIteratorImpl<typename Container::value_type>> clone()
      const {
    return std::make_shared<StlWrapperIterator>(container_, iterator_);
  }

 private:
  bool equal(
      const AbstractIteratorImpl<typename Container::value_type>& other) const {
    const auto& cast = dynamic_cast<const StlWrapperIterator&>(other);
    return iterator_ == cast.iterator_;
  }

 private:
  std::shared_ptr<const Container> container_;
  Iterator iterator_;
};

// Wraps an STL-compliant container as an AbstractContainer
template <typename Container>
AbstractContainer<typename Container::value_type> makeStlWrapperContainer(
    Container container) {
  auto pointer = std::make_shared<Container>(std::move(container));
  return AbstractContainer<typename Container::value_type>(
      std::make_shared<
          StlWrapperIterator<Container, typename Container::const_iterator>>(
          pointer, pointer->begin()),
      std::make_shared<
          StlWrapperIterator<Container, typename Container::const_iterator>>(
          pointer, pointer->end()));
}

} // namespace facebook::rebalancer
