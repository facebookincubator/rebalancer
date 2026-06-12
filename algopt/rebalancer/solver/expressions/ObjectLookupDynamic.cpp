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

#include "algopt/rebalancer/solver/expressions/ObjectLookupDynamic.h"

#include "algopt/rebalancer/solver/expressions/PropertiesHelper.h"

#include <folly/container/Enumerate.h>

#include <algorithm>
#include <sstream>
#include <string_view>
#include <vector>

namespace facebook::rebalancer {

ObjectLookupDynamic::ObjectLookupDynamic(
    ExprPtr sumOfObjectLookups,
    const entities::ObjectScalarDimension& dimension,
    std::shared_ptr<const entities::Universe> universe)
    : Expression(std::move(universe)) {
  // Verify that the dimension is dynamic
  if (!dimension.isDynamic()) {
    throw std::runtime_error(
        "ObjectLookupDynamic can only be used with dynamic dimensions");
  }
  add_child(sumOfObjectLookups);
  if (std::dynamic_pointer_cast<LinearSum>(sumOfObjectLookups) == nullptr) {
    throw std::runtime_error(
        "ObjectLookupDynamic can only be used with LinearSum");
  }
  const auto& dimScope = universe_->getScope(dimension.getScopeId());
  for (auto& lookup : getSumOfLookups()->children()) {
    if (std::dynamic_pointer_cast<ObjectLookup>(lookup) == nullptr) {
      throw std::runtime_error(
          "ObjectLookupDynamic can only be used with ObjectLookups");
    }
    const auto& childContainers = lookup->getDirectlyAffectedContainers();
    assert(childContainers.exists() && !childContainers.getSetPtr()->empty());

    // Check that all containers of the lookup expression
    // belong to the same scopeItem of dimension's scope
    const auto anyContainer = *childContainers.getSetPtr()->begin();
    const std::optional<entities::ScopeItemId> expectedScopeItem =
        dimScope.getScopeItemId(anyContainer);
    for (const auto containerId : *childContainers.getSetPtr()) {
      auto containerScopeItem = dimScope.getScopeItemId(containerId);
      if (containerScopeItem != expectedScopeItem) {
        throw std::runtime_error(
            fmt::format(
                "container set not consistent with dimension's scope {}",
                universe_->getEntityName(dimension.getScopeId())));
      }
      allContainers_->insert(containerId);
    }
  }
  directlyAffectedContainers.set(allContainers_);

  setInitialValue(sumOfObjectLookups->getInitialValue());
}

const std::string_view& ObjectLookupDynamic::getType() const {
  return type;
}

std::optional<AffectedByChange> ObjectLookupDynamic::isAffectedByChange(
    const AffectedByChangeDecisionData& data) const {
  return getSumOfLookups()->isAffectedByChange(data);
}

double ObjectLookupDynamic::evaluate(
    const BottomToTopEvaluator& evaluator,
    const ChangeSet& changes) const {
  return getSumOfLookups()->evaluate(evaluator, changes);
}

void ObjectLookupDynamic::updateEquivalenceSets(
    EquivalenceSets& equivalenceSets) const {
  return getSumOfLookups()->updateEquivalenceSets(equivalenceSets);
}

algopt::lp::Expression ObjectLookupDynamic::lp(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs) {
  return getSumOfLookups()->lp(evaluator, minimizing, configs);
}

bool ObjectLookupDynamic::hasNoLpIntent() const {
  return getSumOfLookups()->hasNoLpIntent();
}

ExpressionProperties ObjectLookupDynamic::getProperties() const {
  ExpressionProperties properties;
  properties.properties()->emplace(
      "containers",
      PropertiesHelper::makeContainerIdListValue(*allContainers_));
  return properties;
}

double ObjectLookupDynamic::innerFullApply(
    const TopToBottomEvaluator& evaluator,
    const Assignment& assignment) {
  value = getSumOfLookups()->fullApply(evaluator, assignment);
  return value;
}

double ObjectLookupDynamic::innerPartialApply(
    const BottomToTopEvaluator& evaluator,
    const Assignment& assignment,
    const ChangeSet& changes) {
  value = getSumOfLookups()->partialApply(evaluator, assignment, changes);
  return value;
}

std::string ObjectLookupDynamic::innerDigest(size_t /*maxChildren*/) const {
  std::vector<std::string_view> names;
  names.reserve(allContainers_->size());
  for (const auto& containerId : *allContainers_) {
    names.push_back(universe_->getEntityName(containerId));
  }
  std::sort(names.begin(), names.end());
  std::stringstream ss;
  ss << "on Containers: ";
  for (const auto& [idx, name] : folly::enumerate(names)) {
    if (idx > 0) {
      ss << ", ";
    }
    ss << name;
  }
  return ss.str();
}

Bounds ObjectLookupDynamic::innerLowerAndUpperBounds(
    Context& context,
    const BoundConstraints& bc) const {
  return getSumOfLookups()->lowerAndUpperBounds(context, bc);
}

void ObjectLookupDynamic::lpIntent(
    const LpEvaluator& evaluator,
    bool minimizing) {
  return getSumOfLookups()->lpIntent(evaluator, minimizing);
}

LinearSum* ObjectLookupDynamic::getSumOfLookups() const {
  return reinterpret_cast<LinearSum*>(getOnlyChildRawPtr());
}

} // namespace facebook::rebalancer
