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

#include "algopt/lp/detail/decomposable/DecomposableProblem.h"
#include "algopt/lp/detail/generic/impl/GenericExpressionImpl.h"
#include "algopt/lp/detail/generic/impl/GenericVariableImpl.h"
#include "algopt/lp/factory/ProblemFactory.h"
#include "algopt/lp/generic/Operators.h"
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <fmt/core.h>
#include <folly/container/F14Map.h>
#include <folly/container/F14Set.h>
#include <folly/container/irange.h>
#include <gtest/gtest.h>

#include <functional>
#include <memory>

using namespace facebook::algopt::lp;
using namespace facebook::algopt::lp::detail;

Problem makeDecomposableProblem() {
  return Problem(
      std::make_unique<DecomposableProblem>(ProblemFactory::makeGurobiProblem));
}

/** creates an example instance and stores vars in @param vars */
Problem makeDecomposableProblemInstance(
    const std::function<Problem()>& factory,
    std::vector<Variable>& vars) {
  Problem instance = factory();
  // instance can be decomposed into two subproblems
  // subproblem_1 = {vars.at(0), vars.at(1)}
  // subproblem_2 = {vars.at(2), vars.at(3)}
  // this decomposition is implicitly stored into partitionIds
  Expression obj;
  for (const auto i : folly::irange(4)) {
    vars.push_back(instance.makeBoolVar(fmt::format("x_{}", i)));
    obj += vars.at(i);
    vars.at(i).setPartition((i / 2) + 1);
  }
  // optimal solution is 2
  instance.setObjective(obj);
  instance.newConstraint(vars.at(0) + vars.at(1) >= 0.5);
  instance.newConstraint(vars.at(0) - vars.at(1) <= 0);
  instance.newConstraint(vars.at(2) + vars.at(3) >= 0.5);
  instance.newConstraint(vars.at(2) - vars.at(3) <= 0);
  return instance;
}

void solveUsingDecomposition(Problem& decomposable) {
  auto& decompProblem =
      static_cast<const DecomposableProblem&>(decomposable.get());
  std::map<PartitionId, DecomposableProblem> subproblems;
  // create one problem per partition id
  for (auto& partitionId : {PartitionId(1), PartitionId(2)}) {
    subproblems.emplace(
        partitionId, DecomposableProblem(decompProblem, {partitionId}));
    auto& subproblem = subproblems.at(partitionId);
    auto imageExpr = subproblem.image(decompProblem.getOnlyObjective());
    subproblem.setObjective(imageExpr.get());
  }

  int numComplicating = 0;
  for (const auto& ctr : decompProblem.getConstraints()) {
    if (auto partitionId = ctr->getPartitionId()) {
      auto& subproblem = subproblems.at(*partitionId);
      subproblem.newConstraint(subproblem.image(ctr->getRelation()).get(), "");
    } else {
      numComplicating++;
    }
  }
  EXPECT_EQ(0, numComplicating);
  for (auto& [_, subproblem] : subproblems) {
    subproblem.solve();
  }
}

TEST(DecompositionTest, endToEndNoComplicating) {
  REBALANCER_SKIP_IF_NO_GUROBI();
  // Create a problem instance that is easily decomposable
  std::vector<Variable> fullProblemVars;
  Problem fullProblem = makeDecomposableProblemInstance(
      ProblemFactory::makeGurobiProblem, fullProblemVars);
  // solve using gurobi
  fullProblem.solve();

  // create a copy of the same problem but solve it using decomposition
  std::vector<Variable> decompProblemVars;
  Problem decomposable = makeDecomposableProblemInstance(
      makeDecomposableProblem, decompProblemVars);
  solveUsingDecomposition(decomposable);

  /// the results must be the same as no complicating constraints
  EXPECT_EQ(fullProblemVars.size(), decompProblemVars.size());
  for (const auto i : folly::irange(fullProblemVars.size())) {
    EXPECT_EQ(
        fullProblemVars.at(i).getValue(), decompProblemVars.at(i).getValue());
  }
}

TEST(DecompositionTest, decomposableImageAPI) {
  REBALANCER_SKIP_IF_NO_GUROBI();
  Problem decomposable = makeDecomposableProblem();

  Variable x = decomposable.makeVar("x");
  x.setPartition(0);
  auto xp = std::static_pointer_cast<const GenericVariableImpl>(x.get());
  xp->setInitialValue(2.5);
  xp->setValue(2.5);
  xp->setLB(0);
  xp->setUB(5);
  EXPECT_EQ(
      "x [2.5 → 2.5] (CONTINUOUS <lb: 0 ub: 5> partition: 0)", xp->digest());

  Variable y = decomposable.makeVar("y");
  y.setPartition(1);
  auto yp = std::static_pointer_cast<const GenericVariableImpl>(y.get());
  yp->setValue(1.2);
  yp->setLB(0);
  yp->setUB(5);
  EXPECT_EQ("y [1.2] (CONTINUOUS <lb: 0 ub: 5> partition: 1)", yp->digest());

  Expression e1 = 2 * x + 5 * y + 6;
  EXPECT_NEAR(e1.getValue(), 17, 1e-8);
  const PartitionId p0(0);
  const PartitionId p1(1);
  auto ep = std::static_pointer_cast<const GenericExpressionImpl>(e1.get());
  const RealizationRule dropConstRule(RealizationRule::ConstantTerm::DROP);
  EXPECT_NEAR(ep->image({p0}, dropConstRule).getValue(), 5, 1e-8);
  EXPECT_NEAR(ep->image({p1}, dropConstRule).getValue(), 6, 1e-8);
  EXPECT_NEAR(ep->image({p0, p1}, dropConstRule).getValue(), 11, 1e-8);
  // e2 = 5y + 2 * 2.5
  Expression e2 = ep->image(
      {p1},
      RealizationRule(
          RealizationRule::RemainingTerms::SUBSTITUTE_CURR_VALUE,
          RealizationRule::ConstantTerm::DROP));
  auto ep2 = std::static_pointer_cast<const GenericExpressionImpl>(e2.get());
  EXPECT_EQ(ep2->digest(), "5 + (5 · y)");
  EXPECT_NEAR(e2.getValue(), 11, 1e-8);

  // original expression is complicating
  EXPECT_EQ(ep->getPartitionId(), std::nullopt);
  EXPECT_EQ(ep2->getPartitionId(), p1);

  // e3 = 2x - 5 + 5y - 6
  const Expression e3 = ep->image(
      {p0, p1},
      RealizationRule(
          RealizationRule::AllowedTerms::FULLY_CONSTRAINED,
          RealizationRule::ConstantTerm::DROP));
  EXPECT_NEAR(e3.getValue(), 0, 1e-8);
  yp->setValue(0.2);
  EXPECT_NEAR(e3.getValue(), -5, 1e-8);

  yp->setValue(0.5 * EPSILON);
  // e4 = 2x + 5 * (0.5 * EPSILON) + 6 = 2x + 6
  Expression e4 = ep->image(
      {p0},
      RealizationRule(RealizationRule::RemainingTerms::SUBSTITUTE_CURR_VALUE));
  auto ep4 = std::static_pointer_cast<const GenericExpressionImpl>(e4.get());
  EXPECT_EQ(ep4->digest(), "6 + (2 · x)");
  EXPECT_NEAR(e4.getValue(), 11, 1e-8);
}
