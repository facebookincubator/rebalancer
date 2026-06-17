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
 * Optimal Solver Examples
 *
 * This file demonstrates optimal solver usage patterns shown in the
 * solver documentation.
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

void quick_example() {
  /**
   * Quick example showing basic OptimalSolverSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  setupMinimalProblem(solver);

  // quick_example_start
  OptimalSolverSpec spec;
  spec.solverPackage() = OptimalSolverPackage::HIGHS;
  spec.solveTime() = 300; // 5 minute time limit

  solver.addSolver(spec);

  auto solution = solver.solve();

  // Check if optimal solver profile is available
  if (solution.problemProfile()->optimalSolverProfile().has_value()) {
    std::cout << "Optimal solver profile available!\n";
  } else {
    std::cout << "No optimal solver profile\n";
  }
  // quick_example_end
}

void small_problem_optimal() {
  /**
   * Small problem that needs optimal solution.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // small_problem_optimal_start
  // Small problem, can wait for optimal
  OptimalSolverSpec spec;
  spec.solverPackage() = OptimalSolverPackage::HIGHS;
  spec.solveTime() = 600; // 10 minutes

  solver.addSolver(spec);
  // small_problem_optimal_end
}

void medium_problem_time_limited() {
  /**
   * Medium problem with time limit.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // medium_problem_time_limited_start
  // Medium problem, accept "good enough"
  OptimalSolverSpec spec;
  spec.solverPackage() = OptimalSolverPackage::HIGHS;
  spec.solveTime() = 300; // 5 minutes

  solver.addSolver(spec);
  // medium_problem_time_limited_end
}

void quick_optimality_check() {
  /**
   * Quick check if problem can be solved.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // quick_optimality_check_start
  // See if we can solve quickly
  OptimalSolverSpec spec;
  spec.solverPackage() = OptimalSolverPackage::HIGHS;
  spec.solveTime() = 60; // 1 minute

  solver.addSolver(spec);
  // quick_optimality_check_end
}

void validate_local_search() {
  /**
   * Compare Local Search to Optimal.
   */
  // validate_local_search_start
  // Run Local Search first
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver1(executor, "example", "test");
  setupMinimalProblem(solver1);
  LocalSearchSolverSpec lsSpec;
  lsSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  solver1.addSolver(lsSpec);
  auto ls_solution = solver1.solve();

  // Then run Optimal on same problem
  ProblemSolver solver2(executor, "example", "test");
  setupMinimalProblem(solver2);
  OptimalSolverSpec optSpec;
  optSpec.solverPackage() = OptimalSolverPackage::HIGHS;
  optSpec.solveTime() = 300;
  solver2.addSolver(optSpec);
  auto opt_solution = solver2.solve();

  // Compare
  const double ls_val = *ls_solution.finalObjective()->value();
  const double opt_val = *opt_solution.finalObjective()->value();
  const double gap = (ls_val - opt_val) / opt_val;
  std::cout << "Local Search was " << (gap * 100) << "% from optimal\n";
  // validate_local_search_end
}

void warm_starting() {
  /**
   * Warm start optimal solver with initial solution.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // warm_starting_start
  // Run Local Search first, then Optimal
  LocalSearchSolverSpec lsSpec;
  lsSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  lsSpec.solveTime() = 10;
  solver.addSolver(lsSpec);

  OptimalSolverSpec optSpec;
  optSpec.solverPackage() = OptimalSolverPackage::HIGHS;
  optSpec.solveTime() = 60;
  solver.addSolver(optSpec);
  // Optimal will use Local Search result as warmstart
  // warm_starting_end
}

void thread_control() {
  /**
   * Control thread usage for optimal solver.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // thread_control_start
  // Use specific number of threads
  OptimalSolverSpec spec;
  spec.solverPackage() = OptimalSolverPackage::HIGHS;

  solver.addSolver(spec);
  // thread_control_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all Optimal Solver examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "\n✓ All Optimal Solver examples completed successfully!\n";
  return 0;
}
// example_end
