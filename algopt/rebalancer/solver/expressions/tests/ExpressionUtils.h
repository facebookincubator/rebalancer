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
#include "algopt/lp/generic/Problem.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/Orchestrator.h"
#include "algopt/rebalancer/solver/expressions/StableStayed.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"

#include <fmt/format.h>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

namespace facebook::rebalancer {

inline std::shared_ptr<ObjectVector> makeObjectVector(
    const PackerMap<entities::ObjectId, double>& objectValues,
    double defaultValue,
    size_t totalObjects,
    std::shared_ptr<const entities::Universe> universe) {
  if (totalObjects < objectValues.size()) {
    throw std::runtime_error(
        fmt::format(
            "ObjectVector: size of objectValues {} must not exceed totalObjects {}",
            objectValues.size(),
            totalObjects));
  }
  auto nonDefaultValues = std::make_shared<entities::ObjectIdToDoubleMap>(
      totalObjects, defaultValue, objectValues.size());
  for (const auto& [objectId, value] : objectValues) {
    nonDefaultValues->emplace(objectId, value);
  }
  return object_vector(std::move(nonDefaultValues), std::move(universe));
}

inline std::shared_ptr<ObjectVector> makeObjectVector(
    const PackerMap<entities::ObjectId, double>& objectValues,
    std::shared_ptr<const entities::Universe> universe) {
  const auto totalObjects = universe->getNumObjects();
  return makeObjectVector(
      objectValues,
      /*defaultValue=*/0,
      totalObjects,
      std::move(universe));
}

inline std::shared_ptr<ObjectVector> makeObjectVector(
    entities::ObjectIdToDoubleMap objectValues,
    std::shared_ptr<const entities::Universe> universe) {
  return object_vector(
      std::make_shared<const entities::ObjectIdToDoubleMap>(
          std::move(objectValues)),
      std::move(universe));
}

} // namespace facebook::rebalancer

namespace facebook::rebalancer::packer::tests {

struct LpAssertOptions {
  // use to specify when there is always an exception when building lp() for
  // expr
  std::optional<std::string> exceptionForLpExpr = std::nullopt;
  // use to specify an exception only when maximing the expr
  std::optional<std::string> exceptionOnlyForLpExprMax = std::nullopt;
  // tolerances to use when building lp() expressions; if nothing is specified,
  // then the default tolerances are used
  std::optional<algopt::lp::Tolerances> lpTolerances = std::nullopt;

  std::string getExpectedExceptionMsgWhenMinimizing() const {
    return exceptionForLpExpr ? *exceptionForLpExpr : "";
  }

  std::string getExpectedExceptionMsgWhenMaximizing() const {
    if (exceptionForLpExpr) {
      return *exceptionForLpExpr;
    } else if (exceptionOnlyForLpExprMax) {
      return *exceptionOnlyForLpExprMax;
    }
    return "";
  }

  std::string getExpectedExceptionMsg(bool shouldMinimize) const {
    return shouldMinimize ? getExpectedExceptionMsgWhenMinimizing()
                          : getExpectedExceptionMsgWhenMaximizing();
  }
};

Orchestrator getOrchestrator(Expression& expression);

double evaluate(Expression& expression, const ChangeSet& changes);

double is_positive(Expression& expression, const ChangeSet& changes);

double _apply(Expression& expression, const Assignment& assignment);

double
_applyChanges(Expression& expr, Context& context, const Assignment& assignment);

double lower_bound(
    Expression& expression,
    const BoundConstraints& bc = BoundConstraints());

double upper_bound(
    Expression& expression,
    const BoundConstraints& bc = BoundConstraints());

ChangeSet swapChanges(
    const PackerMap<entities::ObjectId, entities::ContainerId>&
        initialAssignment,
    entities::ObjectId object1,
    entities::ObjectId object2);

PackerMap<entities::ContainerId, std::vector<entities::ObjectId>>
getContainerToObjects(
    const PackerMap<entities::ObjectId, entities::ContainerId>&
        objectToContainer);

Assignment makeAssignment(
    PackerMap<entities::ObjectId, entities::ContainerId> objectToContainer);

Assignment getModifiedAssignment(
    const Assignment& initialAssignment,
    const ChangeSet& changeSet);

bool descendingChildPotentialsAsExpected(
    Expression& expr,
    std::vector<double> expectedDescendingChildPotentialValues,
    std::optional<std::vector<ExprPtr>> expectedDescendingChildExprs =
        std::nullopt);
// updates the equivalence sets using the expression expr
void updateEquivalenceSets(EquivalenceSets& equivalenceSets, Expression& expr);

// updates the equivalence sets using all the expressions in the subtree rooted
// at expr.
void updateEquivalenceSetsRecursive(
    EquivalenceSets& equivalenceSets,
    Expression& expr,
    entities::EntityIdType numObjects);

void verifyLpExpression(
    const ExprPtr& expr,
    const entities::Map<entities::ContainerId, std::vector<entities::ObjectId>>&
        containerToObjects,
    const std::shared_ptr<const entities::Universe>& universe,
    double expectedValue,
    const std::optional<LpAssertOptions>& lpAssertOptions);

} // namespace facebook::rebalancer::packer::tests
