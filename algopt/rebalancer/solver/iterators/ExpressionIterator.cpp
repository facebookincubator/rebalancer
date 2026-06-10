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

#include "algopt/rebalancer/solver/iterators/ExpressionIterator.h"

#include <cassert>

namespace facebook::rebalancer {

namespace {
static const interface::HottestTraversalConfig kDefaultHottestTraversalConfig{};
}

PreOrderExpressionIterator::PreOrderExpressionIterator() = default;

PreOrderExpressionIterator::PreOrderExpressionIterator(
    Expression* expression,
    bool descending,
    std::function<bool(Expression*)> shouldExpand,
    interface::HottestTraversalConfig traversalConfig)
    : shouldExpand(std::move(shouldExpand)),
      descending_(descending),
      traversalConfig_(std::move(traversalConfig)) {
  if (*traversalConfig_.pruneOptimalSubgraphs()) {
    // compute the potential of root assuming it is minimized.
    const auto& [lowerBound, upperBound] = expression->getUnconstrainedBounds();
    const auto potential = (expression->value - lowerBound);
    unique_push_to_stack({expression, potential});
  } else {
    // the root node is always added to the stack
    unique_push_to_stack({expression, 1.0});
  }
}

PreOrderExpressionIterator& PreOrderExpressionIterator::operator++() {
  assert(!stack.empty());
  auto expr = stack.top().first;
  stack.pop();

  if (shouldExpand(expr)) {
    auto pushInOrderToStack = [&](const auto& beginIter, const auto& endIter) {
      for (auto it = beginIter; it != endIter; ++it) {
        unique_push_to_stack(*it);
      }
    };

    // We need to push children on the stack in the opposite of the desired
    // traversal order; getDescendingChildPotentials() returns the child
    // potentials sorted in descending order
    auto& descendingChildPotentials = expr->getDescendingChildPotentials();
    descending_ ? pushInOrderToStack(
                      descendingChildPotentials.rbegin(),
                      descendingChildPotentials.rend())
                : pushInOrderToStack(
                      descendingChildPotentials.begin(),
                      descendingChildPotentials.end());
  }
  return *this;
}

bool PreOrderExpressionIterator::operator!=(
    const PreOrderExpressionIterator& other) const {
  // Domain of equality: any pair of valid iterators from the same traversal
  // where at least one points to the end.
  return stack.empty() != other.stack.empty();
}

bool PreOrderExpressionIterator::operator==(
    const PreOrderExpressionIterator& other) const {
  // Domain of equality: any pair of valid iterators from the same traversal
  // where at least one points to the end.
  return stack.empty() == other.stack.empty();
}

const PreOrderExpressionIterator::ExpressionPotentialPair&
PreOrderExpressionIterator::operator*() const {
  assert(!stack.empty());
  return stack.top();
}

void PreOrderExpressionIterator::unique_push_to_stack(
    const ExpressionPotentialPair& exprPotentialPair) {
  const auto& [expr, potential] = exprPotentialPair;
  const auto exprAlreadyOptimal =
      algopt::Precision::isZero(potential) || expr->isFixed();
  // add to stack if pruneOptimal is false or the expression has non-zero
  // potential
  const bool shouldAddToStack =
      (!*traversalConfig_.pruneOptimalSubgraphs()) || !exprAlreadyOptimal;
  if (shouldAddToStack && seen.emplace(expr).second) {
    stack.push(exprPotentialPair);
  }
}

PreOrderExpressionTraversal::PreOrderExpressionTraversal(
    Expression* expression,
    bool descending)
    : PreOrderExpressionTraversal(
          expression,
          descending,
          [](Expression*) { return true; },
          kDefaultHottestTraversalConfig) {}

PreOrderExpressionTraversal::PreOrderExpressionTraversal(
    Expression* expression,
    bool descending,
    std::function<bool(Expression*)> shouldExpand,
    interface::HottestTraversalConfig traversalConfig)
    : expression(expression),
      descending(descending),
      shouldExpand(std::move(shouldExpand)),
      traversalConfig_(std::move(traversalConfig)) {}

PreOrderExpressionIterator PreOrderExpressionTraversal::begin() const {
  return PreOrderExpressionIterator(
      expression, descending, shouldExpand, traversalConfig_);
}

PreOrderExpressionIterator PreOrderExpressionTraversal::end() {
  return PreOrderExpressionIterator();
}

PreOrderAscendingExpressionTraversal::PreOrderAscendingExpressionTraversal(
    Expression* expression)
    : PreOrderExpressionTraversal(expression, false) {}

PreOrderDescendingExpressionTraversal::PreOrderDescendingExpressionTraversal(
    Expression* expression)
    : PreOrderExpressionTraversal(expression, true) {}

} // namespace facebook::rebalancer
