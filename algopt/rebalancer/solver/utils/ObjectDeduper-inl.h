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

namespace facebook {
namespace rebalancer {

template <class ObjectIterator>
ObjectDeduperIterator<ObjectIterator>::ObjectDeduperIterator(
    const EquivalenceSets* equivalenceSets,
    ObjectIterator firstObject,
    ObjectIterator lastObject)
    : equivalenceSets_(equivalenceSets),
      currentObject_(firstObject),
      lastObject_(lastObject) {}

template <class ObjectIterator>
ObjectDeduperIterator<ObjectIterator>&
ObjectDeduperIterator<ObjectIterator>::operator++() {
  lookup_.insert(equivalenceSets_->at(*currentObject_));
  ++currentObject_;
  while (currentObject_ != lastObject_) {
    auto eqSet = equivalenceSets_->at(*currentObject_);
    if (!lookup_.contains(eqSet)) {
      return *this;
    }
    ++currentObject_;
  }
  return *this;
}

template <class ObjectIterator>
bool ObjectDeduperIterator<ObjectIterator>::operator==(
    const ObjectDeduperIterator& rhs) const {
  return currentObject_ == rhs.currentObject_;
}

template <class ObjectIterator>
bool ObjectDeduperIterator<ObjectIterator>::operator!=(
    const ObjectDeduperIterator& rhs) const {
  return currentObject_ != rhs.currentObject_;
}

template <class ObjectIterator>
entities::ObjectId ObjectDeduperIterator<ObjectIterator>::operator*() const {
  return *currentObject_;
}

template <class ObjectIterator>
GenericObjectDeduper<ObjectIterator>::GenericObjectDeduper(
    const EquivalenceSets* equivalenceSets,
    ObjectIterator firstObject,
    ObjectIterator lastObject)
    : equivalenceSets_(equivalenceSets),
      firstObject_(firstObject),
      lastObject_(lastObject) {}

template <class ObjectIterator>
ObjectDeduperIterator<ObjectIterator>
GenericObjectDeduper<ObjectIterator>::begin() const {
  return ObjectDeduperIterator<ObjectIterator>(
      equivalenceSets_, firstObject_, lastObject_);
}

template <class ObjectIterator>
ObjectDeduperIterator<ObjectIterator>
GenericObjectDeduper<ObjectIterator>::end() const {
  return ObjectDeduperIterator<ObjectIterator>(
      equivalenceSets_, lastObject_, lastObject_);
}
} // namespace rebalancer
} // namespace facebook
