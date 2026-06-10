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

#include <deque>
#include <map>
#include <stdexcept>

namespace facebook::algopt {

/**
BucketedPriorityQueue is special kind of priority queue that has priority
buckets. The queue prioritizes elements that are at the highest priority bucket.
Within each bucket, there is no priority and the current implementation just
follows FIFO order--i.e, returns the first element in the queue.

Note that unlike standard priority queues where insertion takes `O(log N)` where
`N` is the number of elements, in BucketedPriorityQueue insertion is `O(log L)`,
where `L` is the number of distinct priority buckets. So, this is useful in
cases where `L << N`. Like standard priority queues, accessing the top element
is `O(1)` and so is popping an element.
*/
template <typename PriorityType, typename ElementType>
class BucketedPriorityQueue {
  using PriorityBucketToElementsContainer = std::
      map<PriorityType, std::deque<ElementType>, std::greater<PriorityType>>;

 public:
  [[nodiscard]] const ElementType& top() const {
    // return first element in the first non-empty priorityBucket
    throwIfEmpty();
    return priorityBucketToElements_.cbegin()->second.front();
  }

  void pop() {
    // remove the first element in the first non-empty priorityBucket
    throwIfEmpty();
    auto it = priorityBucketToElements_.begin();
    auto& queue = it->second;
    queue.pop_front();

    if (queue.empty()) {
      priorityBucketToElements_.erase(it);
    }
  }

  template <typename... Args>
  void emplace(const PriorityType& priorityBucket, Args&&... args) {
    priorityBucketToElements_[priorityBucket].emplace_back(
        std::forward<Args>(args)...);
  }

  [[nodiscard]] bool empty() const {
    return priorityBucketToElements_.empty();
  }

  void clear() {
    priorityBucketToElements_.clear();
  }

 private:
  void throwIfEmpty() const {
    if (empty()) [[unlikely]] {
      throw std::runtime_error("cannot call top()/pop() when queue is empty");
    }
  }

 private:
  PriorityBucketToElementsContainer priorityBucketToElements_;
};

} // namespace facebook::algopt
