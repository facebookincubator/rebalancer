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

#include "algopt/rebalancer/algopt_common/ValueSortedMap.h"

#include <folly/container/irange.h>

#include <stdexcept>
#include <utility>

namespace facebook::algopt {

// IncrementalPriorityQueue is a priority queue where the priority of the items
// is defined incrementally by a series of sets of descending priority.
// Given a series of sets, item A has a higher priority than B if A appears in
// a set before B does. If both A and B appear for the first time in the same
// set, then the second set breaks the tie. If the second is also the same,
// then the third is used, and so on. The series of sets can be fed into the
// data structure one set at a time with update().
template <class T>
class IncrementalPriorityQueue {
 public:
  IncrementalPriorityQueue() = default;

  size_t size() const {
    return map.size();
  }

  bool empty() const {
    return size() == 0;
  }

  // Get the item with the highest priority. In case of a tie, get the
  // lexicographically smallest.
  const T& top() const {
    if (map.size() == 0) {
      throw std::runtime_error("empty");
    }
    return map.begin()->first;
  }

  // Whether the top item has a priority strictly higher than any other item in
  // the queue.
  bool is_top_strict() const {
    if (map.size() == 0) {
      throw std::runtime_error("empty");
    }
    if (map.size() < 2) {
      return true;
    }
    auto iterator = map.begin();
    auto& first_sequence = iterator->second;
    ++iterator;
    auto& second_sequence = iterator->second;
    return first_sequence != second_sequence;
  }

  // Removes an item from the queue. The same item cannot be added ever again.
  // If the same item is seen again in an update() call, it will be ignored.
  void remove(const T& item) {
    removed.insert(item);
    map.erase(item);
  }

  // Update the priority of the items passed according to the algorithm
  // described above. Add to the queue any of the items not currently present.
  // Any previously removed items will be ignored and not re-added to the queue.
  void update(const std::vector<T>& items) {
    int index = update_count++;
    for (auto& item : items) {
      if (removed.contains(item)) {
        continue;
      }

      const auto itemPtr = map.getPtrIfExists(item);
      if (itemPtr) {
        Sequence sequence;
        sequence.reserve(itemPtr->size() + 1);
        sequence.insert(sequence.end(), itemPtr->begin(), itemPtr->end());
        sequence.push_back(index);
        map.assign(item, std::move(sequence));
      } else {
        map.assign(item, Sequence{index});
      }
    }
  }

 private:
  using Sequence = std::vector<int>;

  struct Compare {
    bool operator()(
        const std::pair<T, Sequence>& lhs,
        const std::pair<T, Sequence>& rhs) const {
      const auto& [lhsKey, lhsSeq] = lhs;
      const auto& [rhsKey, rhsSeq] = rhs;
      const auto lhsSize = lhsSeq.size();
      const auto rhsSize = rhsSeq.size();
      const auto minSize = std::min(lhsSize, rhsSize);
      for (const auto i : folly::irange(minSize)) {
        const auto lhsSeqI = lhsSeq[i];
        const auto rhsSeqI = rhsSeq[i];
        if (lhsSeqI != rhsSeqI) {
          return lhsSeqI < rhsSeqI;
        }
      }
      // at this point, if the sizes are equal, then the sequences are equal
      if (lhsSize == rhsSize) {
        return lhsKey < rhsKey;
      }
      return lhsSize > rhsSize;
    }
  };

  int update_count = 0;
  SetImpl<T> removed;
  ValueSortedMap<T, Sequence, Compare> map;
};

} // namespace facebook::algopt
