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

/**
 * Solver Overview Examples
 *
 * This file demonstrates solver selection and usage patterns shown in the
 * solver overview documentation.
 */

// example_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

// Helper to set up a minimal problem for solver demonstrations
void setupMinimalProblem(ProblemSolver& solver) {
  solver.setObjectName("task");
  solver.setContainerName("host");

  std::map<std::string, std::vector<std::string>> assignment;
  assignment["host0"] = {"task0", "task1", "task2", "task3"};
  assignment["host1"] = {};
  assignment["host2"] = {};
  solver.setAssignment(assignment);

  std::map<std::string, double> cpu;
  cpu["task0"] = 1.0;
  cpu["task1"] = 2.0;
  cpu["task2"] = 1.5;
  cpu["task3"] = 2.5;
  solver.addObjectDimension("cpu", cpu);

  BalanceSpec balance;
  balance.name() = "balance-cpu";
  balance.scope() = "host";
  balance.dimension() = "cpu";
  balance.formula() = BalanceSpecFormula::LEGACY;
  balance.fixAverageToInitial() = true;
  solver.addGoal(balance, 1.0);
}

void solver_selection() {
  /**
   * Basic solver selection example.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  setupMinimalProblem(solver);

  // solver_selection_start
  // Option 1: Local Search (fast, scales)
  LocalSearchSolverSpec spec;
  spec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  spec.solveTime() = 30; // 30 second limit
  spec.stopAfterMoves() = 100000; // Max iterations

  solver.addSolver(spec);

  // Solve
  auto solution = solver.solve();
  // solver_selection_end
}

void combining_solvers() {
  /**
   * Combine multiple solvers sequentially.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // combining_solvers_start
  // Strategy: Local Search warmup, then Optimal refinement
  LocalSearchSolverSpec lsSpec;
  lsSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  lsSpec.solveTime() = 10;
  solver.addSolver(lsSpec);

  OptimalSolverSpec optSpec;
  optSpec.solverPackage() = OptimalSolverPackage::HIGHS;
  optSpec.solveTime() = 60;
  solver.addSolver(optSpec);

  // Local Search runs first, finds good solution
  // Optimal uses it as warmstart and refines
  // combining_solvers_end
}

void compare_solvers() {
  /**
   * Compare Local Search to Optimal solver.
   */
  // compare_solvers_start
  // Try Local Search
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver1(executor, "example", "test");
  setupMinimalProblem(solver1);
  LocalSearchSolverSpec lsSpec;
  lsSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  solver1.addSolver(lsSpec);
  auto solution1 = solver1.solve();
  std::cout << "Local Search: " << *solution1.finalObjective()->value()
            << " in " << *solution1.problemProfile()->solveSec() << "s\n";

  // Try Optimal
  ProblemSolver solver2(executor, "example", "test");
  setupMinimalProblem(solver2);
  OptimalSolverSpec optSpec;
  optSpec.solverPackage() = OptimalSolverPackage::HIGHS;
  optSpec.solveTime() = 60;
  solver2.addSolver(optSpec);
  auto solution2 = solver2.solve();
  std::cout << "Optimal: " << *solution2.finalObjective()->value() << " in "
            << *solution2.problemProfile()->solveSec() << "s\n";

  // Compare quality vs speed
  const double val1 = *solution1.finalObjective()->value();
  const double val2 = *solution2.finalObjective()->value();
  const double gap = (val1 - val2) / val2;
  std::cout << "Local Search is " << (gap * 100) << "% from optimal\n";
  const double time1 = *solution1.problemProfile()->solveSec();
  const double time2 = *solution2.problemProfile()->solveSec();
  std::cout << "But " << (time2 / time1) << "x faster\n";
  // compare_solvers_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all Solver Overview examples...\n\n";

  std::cout << "1. Solver Selection...\n";
  solver_selection();

  std::cout << "\n✓ All Solver Overview examples completed successfully!\n";
  return 0;
}
// example_end
