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

#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"

#include "algopt/rebalancer/solver/expressions/Orchestrator.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"

#include <folly/container/F14Map.h>

namespace facebook::rebalancer::packer::tests {
namespace {
entities::Map<entities::ContainerId, std::vector<entities::ObjectId>>
getContainerToObjects(const Assignment& assignment) {
  entities::Map<entities::ContainerId, std::vector<entities::ObjectId>>
      containerToObjects;
  for (const auto& [containerId, objects] :
       assignment.getContainerToObjectsMap()) {
    auto& objectsInContainer = containerToObjects[containerId];
    for (const auto objectId : objects) {
      objectsInContainer.push_back(objectId);
    }
  }
  return containerToObjects;
}

} // namespace

double ExpressionTestsBase::evaluate(
    const ExprPtr& expr,
    const ObjectToNewContainer& changes,
    const Assignment& currAssignment,
    const std::optional<LpAssertOptions>& lpAssertOptions) const {
  auto changeSet = getChangeSet(changes, currAssignment);
  Context context;
  context.changes() = changeSet;

  const auto evalValue = getOrchestrator(*expr).evaluate(&*expr, context);

  const auto newAssignment = getModifiedAssignment(currAssignment, changes);

  verifyLpExpression(
      expr,
      getContainerToObjects(newAssignment),
      getUniversePtr(),
      evalValue,
      lpAssertOptions);

  return evalValue;
}

double ExpressionTestsBase::evaluate(
    Expression& expression,
    const ChangeSet& changes) {
  Context context;
  context.changes() = changes;
  return getOrchestrator(expression).evaluate(&expression, context);
}

double ExpressionTestsBase::apply(
    const ExprPtr& expr,
    const Assignment& assignment,
    const std::optional<LpAssertOptions>& lpAssertOptions) const {
  Context context;
  expr->fullApply(TopToBottomEvaluator(context), assignment);

  verifyLpExpression(
      expr,
      getContainerToObjects(assignment),
      getUniversePtr(),
      expr->value,
      lpAssertOptions);

  return expr->value;
}

double ExpressionTestsBase::applyChanges(
    const ExprPtr& expr,
    const ObjectToNewContainer& moves,
    const Assignment& currAssignment,
    const std::optional<LpAssertOptions>& lpAssertOptions) const {
  auto changeSet = getChangeSet(moves, currAssignment);
  Context context;
  context.changes() = changeSet;

  const auto newAssignment = getModifiedAssignment(currAssignment, moves);

  Orchestrator orchestrator;
  orchestrator.init(
      std::vector<Expression*>{&*expr},
      AffectedByChangeDecisionData(
          newAssignment.getObjects().size(),
          newAssignment.getContainers().size()));
  orchestrator.apply(context, newAssignment);

  verifyLpExpression(
      expr,
      getContainerToObjects(newAssignment),
      getUniversePtr(),
      expr->value,
      lpAssertOptions);

  return expr->value;
}

ChangeSet ExpressionTestsBase::getChangeSet(
    const ObjectToNewContainer& moves,
    const Assignment& currAssignment) {
  ChangeSet changeSet;
  for (const auto& [object, container] : moves) {
    changeSet.insert(Change(object, currAssignment.getContainer(object), -1));
    changeSet.insert(Change(object, container, 1));
  }

  return changeSet;
}

Assignment ExpressionTestsBase::getModifiedAssignment(
    const Assignment& assignment,
    const ObjectToNewContainer& changes) {
  Assignment newAssignment = assignment;
  for (const auto [object, container] : changes) {
    newAssignment.moveTo(object, container);
  }
  return newAssignment;
}

} // namespace facebook::rebalancer::packer::tests
