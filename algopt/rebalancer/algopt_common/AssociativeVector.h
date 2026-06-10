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
#include <limits>
#include <vector>

namespace facebook::algopt {

template <class T>
class AssociativeVector {
 public:
  AssociativeVector(std::function<T(T, T)> operation, T neutral)
      : operation(operation), neutral(neutral), size(0) {}

  AssociativeVector(
      std::function<T(T, T)> operation,
      T neutral,
      const std::vector<T>& values)
      : AssociativeVector(operation, neutral) {
    init(values);
  }

  void init(const std::vector<T>& values) {
    size = values.size();
    tree.resize(tree_size(size));
    if (size != 0) {
      init(1, 0, size, values);
    }
  }

  void update(int index, T value) {
    update(1, 0, size, index, value);
  }

  T query() const {
    return query(0, size);
  }

  T query(int lb, int ub) const {
    return query(1, 0, size, lb, ub);
  }

 private:
  std::function<T(T, T)> operation;
  T neutral;
  int size;
  std::vector<T> tree;

  int tree_size(int n) const {
    if (n == 0) {
      return 0;
    }
    int p = 2;
    while (p < 2 * n) {
      p *= 2;
    }
    return p;
  }

  void init(int node, int node_lb, int node_ub, const std::vector<T>& values) {
    if (node_ub - node_lb == 1) {
      tree[node] = values[node_lb];
      return;
    }
    int middle = (node_lb + node_ub) / 2;
    init(2 * node, node_lb, middle, values);
    init(2 * node + 1, middle, node_ub, values);
    tree[node] = operation(tree[2 * node], tree[2 * node + 1]);
  }

  void update(int node, int node_lb, int node_ub, int index, T value) {
    if (node_ub - node_lb == 1) {
      tree[node] = value;
      return;
    }
    int middle = (node_lb + node_ub) / 2;
    if (index < middle) {
      update(2 * node, node_lb, middle, index, value);
    } else {
      update(2 * node + 1, middle, node_ub, index, value);
    }
    tree[node] = operation(tree[2 * node], tree[2 * node + 1]);
  }

  T query(int node, int node_lb, int node_ub, int query_lb, int query_ub)
      const {
    if (node_ub <= query_lb || query_ub <= node_lb) {
      return neutral;
    }
    if (query_lb <= node_lb && node_ub <= query_ub) {
      return tree[node];
    }
    int middle = (node_lb + node_ub) / 2;
    return operation(
        query(2 * node, node_lb, middle, query_lb, query_ub),
        query(2 * node + 1, middle, node_ub, query_lb, query_ub));
  }
};

template <class T>
class SumVector : public AssociativeVector<T> {
 public:
  SumVector() : AssociativeVector<T>(std::plus<T>(), T(0)) {}

  explicit SumVector(const std::vector<T>& values) : SumVector() {
    this->init(values);
  }
};

template <class T>
class MaxVector : public AssociativeVector<T> {
 public:
  MaxVector()
      : AssociativeVector<T>(
            [](T x, T y) { return std::max(x, y); },
            std::numeric_limits<T>::lowest()) {}

  explicit MaxVector(const std::vector<T>& values) : MaxVector() {
    this->init(values);
  }
};

} // namespace facebook::algopt
