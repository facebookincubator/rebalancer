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

#include "algopt/rebalancer/solver/iterators/ObjectPotentialsMerge.h"

namespace facebook::rebalancer {

ObjectPotentialsMergeIterator::ObjectPotentialsMergeIterator()
    : descending_(false) {}

ObjectPotentialsMergeIterator::ObjectPotentialsMergeIterator(
    std::vector<std::pair<
        AbstractIterator<ObjectPotential>,
        AbstractIterator<ObjectPotential>>> inputs,
    bool descending)
    : inputs_(std::move(inputs)),
      descending_(descending),
      sortedHeads_(makeCompareFunction(descending)) {
  int index = 0;
  for (auto& [begin, end] : inputs_) {
    if (begin != end) {
      sortedHeads_.push(
          ObjectPotentialWithIndex{.objectPotential = *begin, .index = index});
    }
    ++index;
  }
}

void ObjectPotentialsMergeIterator::operator++() {
  objectIdsSeen_.insert(sortedHeads_.top().objectPotential.objectId);
  while (!sortedHeads_.empty() &&
         objectIdsSeen_.contains(sortedHeads_.top().objectPotential.objectId)) {
    const int index = sortedHeads_.top().index;
    sortedHeads_.pop();
    auto& [begin, end] = inputs_.at(index);
    ++begin;
    if (begin != end) {
      sortedHeads_.push(
          ObjectPotentialWithIndex{.objectPotential = *begin, .index = index});
    }
  }
}

ObjectPotential ObjectPotentialsMergeIterator::operator*() const {
  if (sortedHeads_.empty()) {
    throw std::runtime_error(
        "ObjectPotentialsMergeIterator has reached the end");
  }
  return sortedHeads_.top().objectPotential;
}

std::shared_ptr<AbstractIteratorImpl<ObjectPotential>>
ObjectPotentialsMergeIterator::clone() const {
  return std::make_shared<ObjectPotentialsMergeIterator>(inputs_, descending_);
}

bool ObjectPotentialsMergeIterator::equal(
    const AbstractIteratorImpl<ObjectPotential>& other) const {
  auto cast = dynamic_cast<const ObjectPotentialsMergeIterator&>(other);
  if (sortedHeads_.empty() && cast.sortedHeads_.empty()) {
    // both at the end
    return true;
  }
  if (sortedHeads_.empty() || cast.sortedHeads_.empty()) {
    // only one at the end
    return false;
  }
  // whether both iterators point at the same item
  return sortedHeads_.top() == cast.sortedHeads_.top();
}

bool ObjectPotentialsMergeIterator::ObjectPotentialWithIndex::operator==(
    const ObjectPotentialWithIndex& other) const {
  return objectPotential == other.objectPotential && index == other.index;
}

std::function<bool(
    const ObjectPotentialsMergeIterator::ObjectPotentialWithIndex&,
    const ObjectPotentialsMergeIterator::ObjectPotentialWithIndex&)>
ObjectPotentialsMergeIterator::makeCompareFunction(bool descending) {
  return [descending](
             const ObjectPotentialWithIndex& a,
             const ObjectPotentialWithIndex& b) {
    if (a.objectPotential == b.objectPotential) {
      return a.index > b.index;
    }
    const bool aSmaller = a.objectPotential < b.objectPotential;
    return descending ? aSmaller : !aSmaller;
  };
}

AbstractContainer<ObjectPotential> makeObjectPotentialsMergeContainer(
    const std::vector<AbstractContainer<ObjectPotential>>& inputs,
    bool descending) {
  std::vector<std::pair<
      AbstractIterator<ObjectPotential>,
      AbstractIterator<ObjectPotential>>>
      iterators;
  iterators.reserve(inputs.size());
  for (auto& input : inputs) {
    iterators.emplace_back(input.begin(), input.end());
  }
  return AbstractContainer<ObjectPotential>(
      std::make_shared<ObjectPotentialsMergeIterator>(
          std::move(iterators), descending),
      std::make_shared<ObjectPotentialsMergeIterator>());
}

} // namespace facebook::rebalancer
