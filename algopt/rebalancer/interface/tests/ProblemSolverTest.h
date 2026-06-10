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

// ProblemSolverTest.h

#pragma once

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/StrategyBuilder.h"
#include "algopt/rebalancer/interface/tests/utils.h"

#include <gtest/gtest.h>

inline void runBasicTest(
    facebook::rebalancer::interface::ProblemSolver& solver,
    bool optimal = false,
    bool runParallelMaterializer = false,
    facebook::rebalancer::interface::OptimalSolverPackage package =
        facebook::rebalancer::interface::OptimalSolverPackage::XPRESS) {
  solver.setObjectName("shard");
  solver.setContainerName("host");
  solver.setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"h1", {"s1", "s2"}}, {"h2", {}}});
  solver.addObjectDimension(
      "cpu", folly::F14FastMap<std::string, double>{{"s1", 20}, {"s2", 10}});
  solver.addContainerDimension(
      "cpu", std::map<std::string, double>{{"h1", 100}, {"h2", 50}});

  facebook::algopt::common::thrift::PrecisionTolerances tolerances;
  tolerances.absolute() = 1e-12;
  tolerances.relative() = 1e-13;
  solver.setPrecision(tolerances);

  facebook::rebalancer::interface::BalanceSpec balanceSpec;
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  balanceSpec.formula() =
      facebook::rebalancer::interface::BalanceSpecFormula::LEGACY;
  balanceSpec.fixAverageToInitial() = true;
  solver.addGoal(balanceSpec);

  if (optimal) {
    facebook::rebalancer::interface::OptimalSolverSpec optimalSpec;
    optimalSpec.solverPackage() = package;
    solver.addSolver(optimalSpec);
  } else {
    facebook::rebalancer::interface::LocalSearchSolverSpec localSearchSpec =
        facebook::rebalancer::interface::makeDefaultLocalSearchSolver();
    localSearchSpec.solveTime() = 10;
    solver.addSolver(localSearchSpec);
  }

  if (runParallelMaterializer) {
    solver.useParallelizedNewMaterializer();
  }
  auto solution = solver.solve();
#ifndef REBALANCER_OSS_BUILD
  auto& profilerRoot = *solution.problemProfile()->hierarchicalProfileRoot();
  EXPECT_EQ("ProblemSolver::Solve", *profilerRoot.eventName());
#endif

  const std::map<std::string, std::string> expected_assignment = {
      {"s1", "h1"}, {"s2", "h2"}};
  EXPECT_EQ(
      expected_assignment,
      facebook::rebalancer::interface::toOrderedMap(*solution.assignment()));
  EXPECT_EQ(
      optimal ? 0.50025 : 0.500125, *solution.initialObjective()->value());
  EXPECT_EQ(0.0, *solution.finalObjective()->value());

  // Expect solvers summary
  ASSERT_EQ(1, solution.solverSummaries()->size());
  EXPECT_EQ(
      facebook::rebalancer::interface::EndReason::OPTIMAL,
      solution.solverSummaries()->at(0).endReason());
}
