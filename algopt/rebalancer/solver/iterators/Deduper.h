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
#include "algopt/rebalancer/solver/utils/Util.h"

namespace facebook::rebalancer {

// Yields the same elements as the underlying iterator, but skips duplicates.
template <typename Element>
class DeduperIterator : public AbstractIteratorImpl<Element> {
 public:
  DeduperIterator(
      AbstractIterator<Element> current,
      AbstractIterator<Element> end)
      : current_(current), end_(end) {}

  void operator++() {
    seen_.insert(*current_);
    while (current_ != end_ && seen_.contains(*current_)) {
      ++current_;
    }
  }

  Element operator*() const {
    return *current_;
  }

  std::shared_ptr<AbstractIteratorImpl<Element>> clone() const {
    auto other = std::make_shared<DeduperIterator>(current_, end_);
    other->seen_ = seen_;
    return other;
  }

 private:
  bool equal(const AbstractIteratorImpl<Element>& other) const {
    const auto& cast = dynamic_cast<const DeduperIterator&>(other);
    return current_ == cast.current_;
  }

 private:
  AbstractIterator<Element> current_;
  AbstractIterator<Element> end_;
  PackerSet<Element> seen_;
};

// Creates a deduped container from another container.
template <typename Element>
AbstractContainer<Element> makeDeduperContainer(
    AbstractContainer<Element> container) {
  return AbstractContainer<Element>(
      std::make_shared<DeduperIterator<Element>>(
          container.begin(), container.end()),
      std::make_shared<DeduperIterator<Element>>(
          container.end(), container.end()));
}

} // namespace facebook::rebalancer
