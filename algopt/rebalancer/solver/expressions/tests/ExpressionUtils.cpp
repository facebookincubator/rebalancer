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

#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"

#include "algopt/lp/detail/highs/highs.h"
#include "algopt/lp/environment/Environment.h"
#include "algopt/lp/generic/Operators.h"
#include "algopt/rebalancer/solver/expressions/LinearSum.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/tests/ExprProblemCreation.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

namespace {
entities::Map<
    entities::ContainerId,
    entities::Map<entities::EquivalenceSetId, int>>
getContainerIdToEquivSetIdToObjectCount(
    const entities::Map<entities::ContainerId, std::vector<entities::ObjectId>>&
        containerToObjects,
    const Problem& problem) {
  entities::
      Map<entities::ContainerId, entities::Map<entities::EquivalenceSetId, int>>
          containerIdToEquivSetIdToObjectCount;
  for (auto& [containerId, objects] : containerToObjects) {
    auto& equivSetIdToNObjects =
        containerIdToEquivSetIdToObjectCount[containerId];
    for (auto& objectId : objects) {
      auto equivSetId = problem.getEquivalenceSets().at(objectId);
      equivSetIdToNObjects[equivSetId]++;
    }
  }

  return containerIdToEquivSetIdToObjectCount;
}
} // namespace

Orchestrator getOrchestrator(Expression& expression) {
  // Take an estimate of number of objects and containers for all tests instead
  // of individually counting them for building AffectedByChangeDecisionData.
  // This is ok because AffectedByChangeDecisionData only affects an internal
  // optimization that is not needed for unit tests.
  constexpr int maxNumObjects = 100;
  constexpr int maxNumContainers = 100;
  const AffectedByChangeDecisionData data(maxNumObjects, maxNumContainers);
  Orchestrator orchestrator;
  orchestrator.init({&expression}, data);
  return orchestrator;
}

double evaluate(Expression& expression, const ChangeSet& changes) {
  Context context;
  context.changes() = changes;
  return getOrchestrator(expression).evaluate(&expression, context);
}

double is_positive(Expression& expression, const ChangeSet& changes) {
  return expression.getPrecision().isStrictlyGtZero(
      evaluate(expression, changes));
}

double _apply(Expression& expression, const Assignment& assignment) {
  Context context;
  return expression.fullApply(TopToBottomEvaluator(context), assignment);
}

double _applyChanges(
    Expression& expr,
    Context& context,
    const Assignment& assignment) {
  Orchestrator orchestrator;
  orchestrator.init(
      std::vector<Expression*>{&expr},
      AffectedByChangeDecisionData(
          assignment.getObjects().size(), assignment.getContainers().size()));
  orchestrator.apply(context, assignment);
  return expr.value;
}

double lower_bound(Expression& expression, const BoundConstraints& bc) {
  Context context;
  expression.init_unconstrained_bounds(context);
  context.clear();
  return expression.lowerAndUpperBounds(context, bc).lower_bound;
}

double upper_bound(Expression& expression, const BoundConstraints& bc) {
  Context context;
  expression.init_unconstrained_bounds(context);
  context.clear();
  return expression.lowerAndUpperBounds(context, bc).upper_bound;
}

ChangeSet swapChanges(
    const PackerMap<entities::ObjectId, entities::ContainerId>&
        initialAssignment,
    entities::ObjectId object1,
    entities::ObjectId object2) {
  ChangeSet ret;
  ret.insert(Change(object1, initialAssignment.at(object1), -1));
  ret.insert(Change(object1, initialAssignment.at(object2), 1));
  ret.insert(Change(object2, initialAssignment.at(object2), -1));
  ret.insert(Change(object2, initialAssignment.at(object1), 1));
  return ret;
}

PackerMap<entities::ContainerId, std::vector<entities::ObjectId>>
getContainerToObjects(
    const PackerMap<entities::ObjectId, entities::ContainerId>&
        objectToContainer) {
  PackerMap<entities::ContainerId, std::vector<entities::ObjectId>>
      containerToObjects;
  for (auto& [objectId, containerId] : objectToContainer) {
    containerToObjects[containerId].push_back(objectId);
  }

  return containerToObjects;
}

Assignment makeAssignment(
    PackerMap<entities::ObjectId, entities::ContainerId> objectToContainer) {
  return Assignment(getContainerToObjects(objectToContainer));
}

Assignment getModifiedAssignment(
    const Assignment& initialAssignment,
    const ChangeSet& changeSet) {
  Assignment newAssignment = initialAssignment;
  auto moveSet = MoveSet::fromChangeSet(changeSet);
  for (auto& move : moveSet) {
    newAssignment.moveTo(move.getObject(), move.getDestinationContainer());
  }

  return newAssignment;
}

bool descendingChildPotentialsAsExpected(
    Expression& expr,
    std::vector<double> expectedDescendingChildPotentialValues,
    std::optional<std::vector<ExprPtr>> expectedDescendingChildExprs) {
  auto& descendingChildPotentials = expr.getDescendingChildPotentials();
  if (expectedDescendingChildPotentialValues.size() !=
      descendingChildPotentials.size()) {
    XLOG(ERR) << fmt::format(
        "descendingChildPotentials size is not as expected; expected {} but got {}",
        expectedDescendingChildPotentialValues.size(),
        descendingChildPotentials.size());
    return false;
  }

  int index = 0;
  for (const auto& [childExpr, childPotentialValue, _] :
       descendingChildPotentials) {
    if (expr.getPrecision().compare(
            expectedDescendingChildPotentialValues.at(index),
            childPotentialValue) != 0) {
      XLOG(ERR) << fmt::format(
          "descendingChildPotentials values are not as expected at index {}; expected {} but got {}",
          index,
          expectedDescendingChildPotentialValues.at(index),
          childPotentialValue);
      return false;
    }

    if (expectedDescendingChildExprs.has_value() &&
        expectedDescendingChildExprs->at(index).get() != childExpr) {
      XLOG(ERR) << fmt::format(
          "descendingChildPotentials expressions are not as expected at index {}",
          index);
      return false;
    }

    ++index;
  }

  return true;
}

// updates the equivalence sets using the expression expr
void updateEquivalenceSets(EquivalenceSets& equivalenceSets, Expression& expr) {
  expr.updateEquivalenceSets(equivalenceSets);
}
// updates the equivalence sets using all the expressions in the subtree rooted
// at expr.
void updateEquivalenceSetsRecursive(
    EquivalenceSets& equivalenceSets,
    Expression& expr,
    entities::EntityIdType numObjects) {
  Orchestrator orchestrator;
  // The unused parameters are for some optimizations that speed up
  // equivalence test computation. They are not relevant for the unit tests.
  orchestrator.init(
      {&expr}, AffectedByChangeDecisionData(0 /*unused*/, 0 /*unused*/));
  orchestrator.updateEquivalenceSets(equivalenceSets, numObjects);
}

static void createProblemBuildLpExprAndAssert(
    const ExprPtr& exprToEvaluate,
    bool shouldMinimize,
    const entities::Map<entities::ContainerId, std::vector<entities::ObjectId>>&
        containerToObjects,
    const std::shared_ptr<const entities::Universe>& universe,
    double expectedValue,
    const std::optional<LpAssertOptions>& lpAssertOptions) {
  auto coeff = shouldMinimize ? 1.0 : -1.0;
  const ExprPtr objective = std::make_shared<LinearSum>(
      universe,
      /*initial=*/0,
      PackerMap<ExprPtr, double>{{exprToEvaluate, coeff}});
  objective->value = coeff * exprToEvaluate->value;

  // for each available solver package, create a new lp problem and evaluate
  // the expr. Skip solvers that are not compiled in.
  std::vector<interface::OptimalSolverPackage> solverPackages;
  if (algopt::isGurobiAvailable()) {
    solverPackages.push_back(interface::OptimalSolverPackage::GUROBI);
  }
  if (algopt::isXpressAvailable()) {
    solverPackages.push_back(interface::OptimalSolverPackage::XPRESS);
  }
  if (algopt::isHiGHSAvailable()) {
    solverPackages.push_back(interface::OptimalSolverPackage::HIGHS);
  }
  if (solverPackages.empty()) {
    // No MIP solver available.
    return;
  }
  for (auto solverPackage : solverPackages) {
    // first create a Problem. we need Problem because lp() needs LpEvaluator,
    // which in turn needs Problem
    auto problemPtr = createTestProblem(
        universe,
        {objective},
        /*constraint=*/const_expr(0, universe),
        /*nonAcceptingContainers=*/{},
        rebalancer::ProblemConfigs{},
        /*performInitialFullApply=*/false);
    auto& problem = *problemPtr;

    auto& dynamicContainers = problem.containers;
    auto dynamicEqSets = problem.getDynamicEquivalentSets(dynamicContainers);
    auto& orchestrator = problem.getOrchestrator();
    auto dynamicChildren = orchestrator.getDynamicChildren(dynamicContainers);
    auto containerIdToEquivSetIdToObjectCount =
        getContainerIdToEquivSetIdToObjectCount(containerToObjects, problem);

    try {
      // setup a mip problem, where objective is the objective expression
      // defined above and constraints are to enforce the assignment specified
      // in containerToObjects
      LpContext lpContext(dynamicContainers, dynamicEqSets, dynamicChildren);
      auto& lpStore = problem.lp_store;
      lpStore.reset(solverPackage, /*simplify=*/false, lpContext);
      if (lpAssertOptions.has_value() &&
          lpAssertOptions->lpTolerances.has_value()) {
        lpStore.setTolerances(lpAssertOptions->lpTolerances.value());
      }

      orchestrator.buildLp(
          {problem.objective.getOnlyObjective()}, {}, problem, lpContext, {});
      auto& lpProblem = problem.lp_store.getLpProblem();

      auto getLpExpr = [&lpContext, &lpProblem](const ExprPtr& exprForLp) {
        auto id = exprForLp->getId();
        // if expr is neither in lpMin nor in lpNotMin, then it is a constant
        if (!lpContext.lpMin().contains(id) &&
            !lpContext.lpNotMin().contains(id)) {
          return lpProblem.makeExpression(exprForLp->value);
        } else {
          return lpContext.lpMin().contains(id) ? lpContext.lpMin().at(id)
                                                : lpContext.lpNotMin().at(id);
        }
      };

      auto objLpExpr = getLpExpr(objective);
      lpProblem.setObjective(objLpExpr);

      // add constraints to enforce the assignment specified in
      // containerToObjects
      for (auto dynamicEqSetId : dynamicEqSets) {
        for (auto dynamicContainerId : dynamicContainers) {
          auto objectCount =
              containerIdToEquivSetIdToObjectCount[dynamicContainerId]
                                                  [dynamicEqSetId];
          auto var = problem.lp_store.getAssignmentVar(
              dynamicEqSetId, dynamicContainerId);
          lpProblem.newConstraint(
              var == objectCount,
              fmt::format(
                  "do_not_move_{}_from_{}",
                  dynamicEqSetId,
                  dynamicContainerId));
        }
      }

      lpProblem.solve();

      if (lpProblem.getStatus() ==
          facebook::algopt::lp::thrift::ProblemStatus::NO_SOLUTION_EXISTS) {
        throw std::runtime_error(
            "LP problem has no solution (infeasible or unbounded)");
      }

      auto lpEvalValue = getLpExpr(exprToEvaluate).getValue();

      EXPECT_NEAR(expectedValue, lpEvalValue, 1e-3) << fmt::format(
          "expected {} lp expression to evaluate to {}, but got {}",
          apache::thrift::util::enumNameSafe(solverPackage),
          expectedValue,
          lpEvalValue);
    } catch (const facebook::algopt::lp::detail::HiGHSError& e) {
      if (lpAssertOptions) {
        EXPECT_EQ(
            lpAssertOptions->getExpectedExceptionMsg(shouldMinimize), e.what());
      } else {
        XLOG(WARN) << "HiGHS solver failed (likely MIQP not supported), "
                   << "skipping HiGHS verification: " << e.what();
      }
    } catch (std::exception& e) {
      if (!lpAssertOptions) {
        XLOG(ERR) << "Exception when trying to verify lp expression";
        throw std::runtime_error(e.what());
      }
      EXPECT_EQ(
          lpAssertOptions->getExpectedExceptionMsg(shouldMinimize), e.what());
    }
  }

  return;
}

void verifyLpExpression(
    const ExprPtr& expr,
    const entities::Map<entities::ContainerId, std::vector<entities::ObjectId>>&
        containerToObjects,
    const std::shared_ptr<const entities::Universe>& universe,
    double expectedValue,
    const std::optional<LpAssertOptions>& lpAssertOptions) {
  // create problems where we minimize and maximize expr and assert that the lp
  // expressions built have the expected value
  for (auto shouldMinimize : {true, false}) {
    createProblemBuildLpExprAndAssert(
        expr,
        shouldMinimize,
        containerToObjects,
        universe,
        expectedValue,
        lpAssertOptions);
  }
}

} // namespace facebook::rebalancer::packer::tests
