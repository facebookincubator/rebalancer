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

#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <iterator>
#include <stack>

namespace facebook::rebalancer {

// Traverses an expression DAG efficiently in pre-order, without repeating
// nodes, traversing children in the order defined by
// Expression::get_sorted_children. A function can be passed to decide whether
// the children of an expression should be traversed, otherwise all expressions
// will be traversed.
class PreOrderExpressionIterator
    : public std::iterator<std::input_iterator_tag, Expression*> {
 public:
  using ExpressionPotentialPair = std::pair<Expression*, double>;

 public:
  PreOrderExpressionIterator();
  PreOrderExpressionIterator(
      Expression*,
      bool descending,
      std::function<bool(Expression*)> shouldExpand,
      interface::HottestTraversalConfig traversalConfig);
  PreOrderExpressionIterator& operator++();
  bool operator!=(const PreOrderExpressionIterator& other) const;
  bool operator==(const PreOrderExpressionIterator& other) const;
  const ExpressionPotentialPair& operator*() const;

 private:
  void unique_push_to_stack(const ExpressionPotentialPair& exprPotentialPair);

  std::function<bool(Expression*)> shouldExpand;
  std::stack<ExpressionPotentialPair> stack;
  folly::F14FastSet<Expression*> seen;
  bool descending_ = false;
  const interface::HottestTraversalConfig traversalConfig_;
};

class PreOrderExpressionTraversal {
 public:
  PreOrderExpressionTraversal(Expression* expression, bool descending);
  PreOrderExpressionTraversal(
      Expression* expression,
      bool descending,
      std::function<bool(Expression*)> shouldExpand,
      interface::HottestTraversalConfig traversalConfig);
  PreOrderExpressionIterator begin() const;
  static PreOrderExpressionIterator end();

 private:
  Expression* expression;
  bool descending;
  std::function<bool(Expression*)> shouldExpand;
  interface::HottestTraversalConfig traversalConfig_;
};

class PreOrderAscendingExpressionTraversal
    : public PreOrderExpressionTraversal {
 public:
  explicit PreOrderAscendingExpressionTraversal(Expression* expression);
};

class PreOrderDescendingExpressionTraversal
    : public PreOrderExpressionTraversal {
 public:
  explicit PreOrderDescendingExpressionTraversal(Expression* expression);
};

} // namespace facebook::rebalancer
