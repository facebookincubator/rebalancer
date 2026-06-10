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

#include "algopt/lp/detail/generic/impl/GenericProblemImpl.h"
#include "algopt/lp/factory/ProblemFactory.h"
#include "algopt/lp/generic/Expression.h"
#include "algopt/lp/generic/Operators.h"
#include "algopt/lp/generic/Problem.h"
#include "algopt/rebalancer/benchmarks/utils/LpInstanceGenerator.h"
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <folly/container/irange.h>
#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>

#include <memory>

using namespace facebook::algopt::lp;
using namespace facebook::algopt::lp::detail;
using namespace facebook;

Problem makeGenericGurobiProblem() {
  return Problem(
      std::make_unique<GenericProblemImpl>(ProblemFactory::makeGurobiProblem));
}

void checkIfSameProblems(
    const GenericProblemImpl& origProb,
    const GenericProblemImpl& newProb) {
  // if the problem has same variables
  auto origVars = origProb.getVars();
  auto newVars = newProb.getVars();
  EXPECT_EQ(origVars.size(), newVars.size());
  EXPECT_EQ(origProb.getConstraints().size(), newProb.getConstraints().size());
  EXPECT_EQ(origProb.getObjectiveSize(), newProb.getObjectiveSize());
  const int numObjectives = origProb.getObjectiveSize();

  // index original variables and constraints by there names
  folly::F14FastMap<std::string, ConstGenericVarPtr> indexedVarsOrig;
  folly::F14FastMap<std::string, std::shared_ptr<GenericConstraintImpl>>
      indexedConstraintsOrig;
  for (auto var : origVars) {
    indexedVarsOrig.emplace(var->getName(), var);
  }
  for (auto ctr : origProb.getConstraints()) {
    indexedConstraintsOrig.emplace(ctr->getName(), ctr);
  }
  // variables of both models should be identical
  for (const auto& var : newVars) {
    const auto& origVar = indexedVarsOrig.at(var->getName());
    EXPECT_EQ(origVar->digest(), var->digest());
  }
  // constraints of both models should be identical
  for (const auto& ctr : newProb.getConstraints()) {
    auto origCtr = indexedConstraintsOrig.at(ctr->getName());
    EXPECT_EQ(
        origCtr->getRelation().digest(true), ctr->getRelation().digest(true));
  }
  // objectives of both models should be identical
  for (const auto i : folly::irange(numObjectives)) {
    auto origObj = std::static_pointer_cast<const GenericExpressionImpl>(
        origProb.getObjectiveAt(i));
    auto newObj = std::static_pointer_cast<const GenericExpressionImpl>(
        newProb.getObjectiveAt(i));
    EXPECT_EQ(origObj->digest(true), newObj->digest(true));
  }
}

TEST(ReplayerTest, GenericSimple) {
  REBALANCER_SKIP_IF_NO_GUROBI();
  // Example from: https://en.wikipedia.org/wiki/Integer_programming#Example
  // With a small change to the objective which makes the solution unique.
  auto problem = makeGenericGurobiProblem();
  auto x = problem.makeIntVar("x");
  x.setLB(0);
  auto y = problem.makeIntVar("y");
  y.setLB(0);
  problem.newConstraint(-x + y <= 1, "ctr1");
  problem.newConstraint(3 * x + 2 * y <= 12, "ctr2");
  problem.newConstraint(2 * x + 3 * y <= 12, "ctr3");
  auto maximize = -0.5 * x + y;

  problem.setObjective(-maximize);
  const folly::test::TemporaryFile jsonModel("algopt_replayer_test");

  problem.solve();
  auto& originalProblem =
      dynamic_cast<const GenericProblemImpl&>(problem.get());
  problem.saveToFile(jsonModel.path().string());

  auto newProblem =
      std::make_unique<GenericProblemImpl>(ProblemFactory::makeGurobiProblem);
  newProblem->loadFromFile(jsonModel.path().string());
  newProblem->solve(true /*useSolverSpecificMultiObjectiveSolveIfAvailable*/);
  checkIfSameProblems(originalProblem, *newProblem);
}

TEST(ReplayerTest, GenericBig) {
  REBALANCER_SKIP_IF_NO_GUROBI();
  // Example from: https://en.wikipedia.org/wiki/Integer_programming#Example
  // With a small change to the objective which makes the solution unique.
  auto problem = makeGenericGurobiProblem();
  int randomSeed = 1234;
  LpInstanceGenerator generator(problem, LpInstanceType::ODD_EVEN, randomSeed);
  constexpr int MAX_SUBPROBLEMS = 10;
  for (const auto subproblem : folly::irange(1, MAX_SUBPROBLEMS + 1)) {
    generator.addHittingSetInstance(100, 200, 10, PartitionId(subproblem));
  }
  generator.addComplicatingConstraints(100, MAX_SUBPROBLEMS / 2);

  const folly::test::TemporaryFile jsonModel("algopt_replayer_test");
  auto& originalProblem =
      dynamic_cast<const GenericProblemImpl&>(problem.get());
  problem.saveToFile(jsonModel.path().string());

  auto newProblem =
      std::make_unique<GenericProblemImpl>(ProblemFactory::makeGurobiProblem);
  newProblem->loadFromFile(jsonModel.path().string());

  // the above problems are not solved, but if the models are identical we can
  // be sure that they solve to the same thing
  checkIfSameProblems(originalProblem, *newProblem);
}
