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
#include <optional>

namespace facebook::algopt {

// AssociativeMap is a key-value map with efficient operations to insert, remove
// and update key-value pairs, and to query the aggregation of all values under
// a given associative operation such as sum. It has one key property: the
// values are aggregated in a stable order determined by the set of keys alone,
// independent of the order the keys were inserted or deleted. This is key when
// stability against floating point precision errors is required.
// Implements an AVL tree (https://en.wikipedia.org/wiki/AVL_tree)
template <class K, class V>
class AssociativeMap {
 public:
  AssociativeMap(std::function<V(V, V)> operation, V neutral)
      : operation(operation), neutral(std::move(neutral)), root(nullptr) {}

  void update(const K& key, const V& value) { // O(log(n))
    insert_or_update(root, key, value);
  }

  std::optional<V> getValueIfExists(const K& key) const {
    return getValueIfExists(root, key);
  }

  void remove(const K& key) { // O(log(n))
    remove(root, key);
  }

  V query() const { // O(1)
    if (!root) {
      return neutral;
    }
    return root->result;
  }

  // Get value of aggregating all keys less than k
  V query_lt(const K& key) const { // O(log(n))
    if (!root) {
      return neutral;
    }
    return less_than(*root, key, true);
  }

  // Get value of aggregating all keys less than or equal to k
  V query_le(const K& key) const { // O(log(n))
    if (!root) {
      return neutral;
    }
    return less_than(*root, key, false);
  }

  void clear() { // O(n)
    root.reset();
  }

 private:
  struct Node {
    Node(K k, V v)
        : key(std::move(k)),
          value(std::move(v)),
          result(value),
          height(1),
          left(nullptr),
          right(nullptr) {}

    K key;
    V value;
    V result;
    size_t height;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
  };

  using NodePtr = std::unique_ptr<Node>;

  enum InsertOrUpdate {
    INSERT,
    UPDATE,
  };

  std::function<V(V, V)> operation;
  V neutral;
  NodePtr root;

  // O(1)
  size_t height(const NodePtr& node) const {
    return node ? node->height : 0;
  }

  std::optional<V> getValueIfExists(const NodePtr& node, const K& key) const {
    if (!node) {
      return std::nullopt;
    }

    if (key == node->key) {
      return node->value;
    }

    if (key < node->key) {
      return getValueIfExists(node->left, key);
    }

    return getValueIfExists(node->right, key);
  }

  // Update height of a node assuming heights of children are correct. O(1)
  void update_height(Node& node) const {
    node.height = 1 + std::max(height(node.left), height(node.right));
  }

  // Update result of a node assuming results of children are correct. O(1)
  void update_result(Node& node) const {
    node.result = node.value;
    if (node.left) {
      node.result = operation(node.result, node.left->result);
    }
    if (node.right) {
      node.result = operation(node.result, node.right->result);
    }
  }

  // O(1)
  void update_height_and_result(Node& node) const {
    update_height(node);
    update_result(node);
  }

  // Apply a left rotation. O(1)
  // rotate_left(A):
  //     A                    C
  //   /  \                 /  \
  //  B    C      =>       A    E
  //     /  \            /  \
  //    D    E          B    D
  void rotate_left(NodePtr& node) const {
    auto tmp = std::move(node);
    node = std::move(tmp->right);
    tmp->right = std::move(node->left);
    node->left = std::move(tmp);
    update_height_and_result(*node->left);
    update_height_and_result(*node);
  }

  // Apply a right rotation. O(1)
  // rotate_right(A):
  //        A               B
  //      /  \            /  \
  //     B    C    =>    D    A
  //   /  \                 /  \
  //  D    E               E    C
  void rotate_right(NodePtr& node) const {
    auto tmp = std::move(node);
    node = std::move(tmp->left);
    tmp->left = std::move(node->right);
    node->right = std::move(tmp);
    update_height_and_result(*node->right);
    update_height_and_result(*node);
  }

  // Apply fixes to the node after changing it or its children. O(1)
  // - Rebalance.
  // - Update height.
  // - Update result.
  void rebalance(NodePtr& node) const {
    if (height(node->left) > height(node->right) + 1) {
      if (height(node->left->right) > height(node->left->left)) {
        rotate_left(node->left);
      }
      rotate_right(node);
      return;
    }
    if (height(node->right) > height(node->left) + 1) {
      if (height(node->right->left) > height(node->right->right)) {
        rotate_right(node->right);
      }
      rotate_left(node);
      return;
    }
    update_height_and_result(*node);
  }

  // Insert/update the value for a given key and recompute results. O(log(n))
  InsertOrUpdate insert_or_update(NodePtr& node, const K& key, const V& value)
      const {
    if (!node) {
      node = std::make_unique<Node>(key, value);
      return INSERT;
    }
    if (key == node->key) {
      node->value = value;
      update_result(*node);
      return UPDATE;
    }
    InsertOrUpdate result;
    if (key < node->key) {
      result = insert_or_update(node->left, key, value);
    } else {
      result = insert_or_update(node->right, key, value);
    }
    if (result == UPDATE) {
      update_result(*node);
    } else {
      rebalance(node);
    }
    return result;
  }

  // Extract the lowest key from the given subtree, and return it. O(log(n))
  NodePtr extract_min(NodePtr& node) const {
    if (!node->left) {
      auto min_node = std::move(node);
      node = std::move(min_node->right);
      return min_node;
    }
    auto min_node = extract_min(node->left);
    rebalance(node);
    return min_node;
  }

  // Merge two trees into one and return the new root. O(log(n))
  NodePtr merge(NodePtr left, NodePtr right) const {
    if (!right) {
      return left;
    }
    auto new_root = extract_min(right);
    new_root->left = std::move(left);
    new_root->right = std::move(right);
    rebalance(new_root);
    return new_root;
  }

  // O(log(n))
  void remove(NodePtr& node, const K& key) const {
    if (!node) {
      return;
    }
    if (key == node->key) {
      node = merge(std::move(node->left), std::move(node->right));
      return;
    }
    if (key < node->key) {
      remove(node->left, key);
    } else {
      remove(node->right, key);
    }
    rebalance(node);
  }

  // O(log(n))
  V less_than(const Node& node, const K& key, bool strict = true) const {
    if ((strict && node.key < key) || (!strict && node.key <= key)) {
      V result = node.value;
      if (node.left) {
        result = operation(result, node.left->result);
      }
      if (node.right) {
        result = operation(result, less_than(*node.right, key, strict));
      }
      return result;
    }
    if (node.left) {
      return less_than(*node.left, key, strict);
    }
    return neutral;
  }
};

template <class K, class V>
class SumMap : public AssociativeMap<K, V> {
 public:
  SumMap() : AssociativeMap<K, V>(std::plus<V>(), V(0)) {}
};

} // namespace facebook::algopt
