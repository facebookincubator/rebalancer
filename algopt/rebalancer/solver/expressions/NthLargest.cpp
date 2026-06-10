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

#include "algopt/rebalancer/solver/expressions/NthLargest.h"

#include "algopt/rebalancer/solver/expressions/BottomToTopEvaluator.h"
#include "algopt/rebalancer/solver/expressions/PropertiesHelper.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/iterators/MapUnion.h"

namespace {
constexpr std::string_view type = "NthLargest";
}

namespace facebook::rebalancer {

NthLargest::NthLargest(
    const std::vector<std::shared_ptr<Expression>>& values,
    int n,
    bool unique,
    std::shared_ptr<const entities::Universe> universe)
    : Expression(std::move(universe)), n_(n), unique_(unique) {
  for (const auto& value : values) {
    add_child(value);
  }
  if (children().empty()) {
    throw std::runtime_error("NthLargest requires at least one value");
  }
}

const std::string_view& NthLargest::getType() const {
  return type;
}

void NthLargest::updateEquivalenceSets(EquivalenceSets& equivalenceSets) const {
  PackerMap<entities::ObjectId, std::vector<entities::ContainerId>>
      objectToContainers;
  for (const auto& child : children()) {
    if (auto var = child->getVar()) {
      auto [obj, cont] = *var;
      objectToContainers[obj].push_back(cont);
    }
  }
  for (auto& [_, containers] : objectToContainers) {
    sort(containers.begin(), containers.end());
  }
  equivalenceSets.mappingMerge(objectToContainers);
}

double NthLargest::_applyUsingChildValues(
    const Evaluator& evaluator,
    const Assignment& assignment) {
  valueFrequency_.clear();
  for (const auto& child : children()) {
    const double value = evaluator.apply(child.get(), assignment);
    ++valueFrequency_[value];
  }
  value = computeNthLargest();
  return value;
}

double NthLargest::innerFullApply(
    const TopToBottomEvaluator& evaluator,
    const Assignment& assignment) {
  return _applyUsingChildValues(evaluator, assignment);
}

double NthLargest::innerPartialApply(
    const BottomToTopEvaluator& evaluator,
    const Assignment& assignment,
    const ChangeSet& /* unused */) {
  return _applyUsingChildValues(evaluator, assignment);
}

double NthLargest::evaluate(
    const BottomToTopEvaluator& evaluator,
    const ChangeSet& changes) const {
  std::map<double, int> valueFrequencyDelta;
  auto& changedChildren = evaluator.getChangedChildren((Expression*)this);
  for (auto child : changedChildren) {
    const double oldValue = child->value;
    const double newValue = evaluator.evaluate(child, changes);
    if (oldValue != newValue) {
      --valueFrequencyDelta[oldValue];
      ++valueFrequencyDelta[newValue];
    }
  }

  if (valueFrequencyDelta.empty()) {
    // No values changed
    return value;
  }

  // Recompute n-th largest with the changes
  return computeNthLargest(valueFrequencyDelta);
}

Bounds NthLargest::innerLowerAndUpperBounds(
    Context& context,
    const BoundConstraints& bc) const {
  double lowerBound = std::numeric_limits<double>::infinity();
  double upperBound = -std::numeric_limits<double>::infinity();
  for (const auto& child : children()) {
    auto [childLb, childUb] = child->lowerAndUpperBounds(context, bc);
    lowerBound = std::min(lowerBound, childLb);
    upperBound = std::max(upperBound, childUb);
  }
  return {.lower_bound = lowerBound, .upper_bound = upperBound};
}

double NthLargest::computeNthLargest(
    const std::map<double, int>& valueFrequencyDelta) const {
  // Iterate values from largest to smallest.
  const MapUnion<std::map<double, int>::const_reverse_iterator> mapUnion(
      valueFrequency_.rbegin(),
      valueFrequency_.rend(),
      valueFrequencyDelta.rbegin(),
      valueFrequencyDelta.rend(),
      0,
      [](double x, double y) { return x > y; });

  int index = 0;
  double smallestValue = std::numeric_limits<double>::infinity();
  for (const auto& [value, baseFrequency, deltaFrequency] : mapUnion) {
    const int frequency = baseFrequency + deltaFrequency;
    if (frequency == 0) {
      continue;
    }

    const int indexIncrement = unique_ ? 1 : frequency;
    index += indexIncrement;
    if (index > n_) {
      // We found the n-th largest value
      return value;
    }

    smallestValue = value;
  }

  // If there are fewer than n+1 (unique) values, then return the smallest value
  return smallestValue;
}

ExpressionProperties NthLargest::getProperties() const {
  ExpressionProperties properties;
  properties.properties()->emplace("n", PropertiesHelper::makeIntValue(n_));
  properties.properties()->emplace(
      "unique", PropertiesHelper::makeBoolValue(unique_));
  return properties;
}

} // namespace facebook::rebalancer
