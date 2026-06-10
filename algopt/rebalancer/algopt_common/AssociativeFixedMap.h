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

#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

namespace facebook::algopt {

// AssociativeFixedMap is similar to AssociativeMap, with a few differences that
// make it more performant:
// - All keys and their initial values must be known during initialization.
// - Values can be updated after initialization.
// - New keys cannot be inserted after initialization.
// - Keys cannot be removed.
// Initialization of AssociativeFixedMap is ~30x faster than inserting the same
// key-value pairs in AssociativeMap, and updates are ~2x faster.
template <class K, class V>
class AssociativeFixedMap {
 public:
  AssociativeFixedMap(std::function<V(V, V)> operation, V neutral)
      : operation(operation), neutral(std::move(neutral)) { // O(1)
  }

  void init(std::vector<std::pair<K, V>> items) { // O(n * log(n))
    std::sort(items.begin(), items.end());
    if (items.empty()) {
      nodes.clear();
    } else {
      nodes.resize(items.size() + 1);
      init_tree(1, items.begin(), items.end());
    }
  }

  void update(const K& key, V value) { // O(log(n))
    update_tree(1, key, std::move(value));
  }

  int count(const K& key) const { // O(log(n))
    return count_tree(1, key);
  }

  const V& query() const { // O(1)
    if (nodes.empty()) {
      return neutral;
    }
    return nodes[1]->result;
  }

  // Get value of aggregating all keys less than k
  V query_lt(const K& key) const { // O(log(n))
    if (nodes.empty()) {
      return neutral;
    }
    return less_than(1, key, true);
  }

  // Get value of aggregating all keys less than or equal to k
  V query_le(const K& key) const { // O(log(n))
    if (nodes.empty()) {
      return neutral;
    }
    return less_than(1, key, false);
  }

 private:
  struct Node {
    K key;
    V value;
    V result;
  };

  int left_child_size(int n) const {
    // Layout of the binary tree:
    //     root
    //   B(h)  B(h)
    //   p     q
    // Legend:
    // - n is the total number of nodes in the tree
    // - root is a single node
    // - B(h) is a complete binary tree of height h
    // - p is a row of 0 to 2^h nodes
    // - q is a row of 0 nodes if r < 2^h, otherwise 0 to 2^h - 1 nodes
    // Result: this function computes the size of B(h) and r combined
    int h = 0;
    while (1 + 2 * ((1 << (h + 1)) - 1) <= n) {
      ++h;
    }
    const auto p = std::min(1 << h, n - 1 - 2 * ((1 << h) - 1));
    return (1 << h) - 1 + p;
  }

  void update_result(int node_index) {
    auto& node = *nodes[node_index];
    node.result = node.value;
    if (2 * node_index < static_cast<int>(nodes.size())) {
      auto& left_child = *nodes[2 * node_index];
      node.result = operation(left_child.result, node.result);
    }
    if (2 * node_index + 1 < static_cast<int>(nodes.size())) {
      auto& right_child = *nodes[2 * node_index + 1];
      node.result = operation(node.result, right_child.result);
    }
  }

  template <class RandomAccessIterator>
  void init_tree(
      int node_index,
      RandomAccessIterator begin,
      RandomAccessIterator end) {
    int left_size = left_child_size(end - begin);
    RandomAccessIterator current = begin + left_size;
    if (begin != current) {
      init_tree(2 * node_index, begin, current);
    }
    if (current + 1 != end) {
      init_tree(2 * node_index + 1, current + 1, end);
    }

    nodes[node_index] = std::make_unique<Node>(Node{
        .key = std::move(current->first), .value = std::move(current->second)});

    update_result(node_index);
  }

  void update_tree(int node_index, const K& key, V value) {
    if (node_index >= static_cast<int>(nodes.size())) {
      throw std::runtime_error("key not found");
    }
    auto& node = *nodes[node_index];
    if (key == node.key) {
      node.value = std::move(value);
    } else {
      int next_index = key < node.key ? 2 * node_index : 2 * node_index + 1;
      update_tree(next_index, key, std::move(value));
    }
    update_result(node_index);
  }

  int count_tree(int node_index, const K& key) const {
    if (node_index >= static_cast<int>(nodes.size())) {
      return 0;
    }
    auto& node = *nodes[node_index];
    if (key == node.key) {
      return 1;
    }
    int next_index = key < node.key ? 2 * node_index : 2 * node_index + 1;
    return count_tree(next_index, key);
  }

  V less_than(int node_index, const K& key, bool strict = true) const {
    auto& node = *nodes[node_index];
    if ((strict && node.key < key) || (!strict && node.key <= key)) {
      V result = node.value;
      if (2 * node_index < nodes.size()) {
        auto& left_child = *nodes[2 * node_index];
        result = operation(left_child.result, result);
      }
      if (2 * node_index + 1 < nodes.size()) {
        result = operation(result, less_than(2 * node_index + 1, key, strict));
      }
      return result;
    }
    if (2 * node_index < nodes.size()) {
      return less_than(2 * node_index, key, strict);
    }
    return neutral;
  }

  std::function<V(V, V)> operation;
  V neutral;
  std::vector<std::unique_ptr<Node>> nodes;
};

template <class K, class V>
class SumFixedMap : public AssociativeFixedMap<K, V> {
 public:
  SumFixedMap() : AssociativeFixedMap<K, V>(std::plus<V>(), V(0)) {}
};

} // namespace facebook::algopt
