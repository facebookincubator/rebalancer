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

#include <type_traits>

namespace facebook::rebalancer {

template <typename InputElement, typename OutputElement, typename Function>
class TransformIterator : public AbstractIteratorImpl<OutputElement> {
 public:
  TransformIterator(AbstractIterator<InputElement> iterator, Function function)
      : iterator_(std::move(iterator)), function_(std::move(function)) {}

  void operator++() {
    ++iterator_;
  }

  OutputElement operator*() const {
    return function_(*iterator_);
  }

  std::shared_ptr<AbstractIteratorImpl<OutputElement>> clone() const {
    return std::make_shared<TransformIterator>(iterator_, function_);
  }

 private:
  bool equal(const AbstractIteratorImpl<OutputElement>& other) const {
    const auto& cast = dynamic_cast<const TransformIterator&>(other);
    return iterator_ == cast.iterator_;
  }

  AbstractIterator<InputElement> iterator_;
  Function function_;
};

template <typename InputElement, typename Function>
auto makeTransformContainer(
    AbstractContainer<InputElement> container,
    Function function) {
  using OutputElement = std::invoke_result_t<Function, InputElement>;
  return AbstractContainer<OutputElement>(
      std::make_shared<
          TransformIterator<InputElement, OutputElement, Function>>(
          container.begin(), function),
      std::make_shared<
          TransformIterator<InputElement, OutputElement, Function>>(
          container.end(), function));
}

} // namespace facebook::rebalancer
