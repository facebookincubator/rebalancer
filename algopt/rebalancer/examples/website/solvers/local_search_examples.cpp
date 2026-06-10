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
 * Local Search Solver Examples
 *
 * This file demonstrates local search solver usage patterns shown in the
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
   * Quick example showing basic LocalSearchSolverSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  setupMinimalProblem(solver);

  // quick_example_start
  LocalSearchSolverSpec spec;
  spec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  spec.solveTime() = 30; // Stop after 30 seconds
  spec.stopAfterMoves() = 100000; // Or after 100K iterations

  solver.addSolver(spec);
  auto solution = solver.solve();

  // Check solve time
  std::cout << "Time: " << *solution.problemProfile()->solveSec() << "s\n";
  // quick_example_end
}

void fast_interactive() {
  /**
   * Fast interactive rebalancing for quick responses.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // fast_interactive_start
  LocalSearchSolverSpec spec;
  spec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  spec.solveTime() = 5; // 5 second limit

  solver.addSolver(spec);
  // fast_interactive_end
}

void production_rebalancing() {
  /**
   * Production rebalancing balancing speed and quality.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // production_rebalancing_start
  LocalSearchSolverSpec spec;
  spec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  spec.solveTime() = 60; // 1 minute
  spec.stopAfterMoves() = 500000; // 500K moves

  solver.addSolver(spec);
  // production_rebalancing_end
}

void offline_optimization() {
  /**
   * Offline optimization taking time for best solution.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // offline_optimization_start
  LocalSearchSolverSpec spec;
  spec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  spec.solveTime() = 3600; // 1 hour
  // No move limit - run until no improvements

  solver.addSolver(spec);
  // offline_optimization_end
}

void reproducible_results() {
  /**
   * Get reproducible results with fixed seed.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // reproducible_results_start
  LocalSearchSolverSpec spec;
  spec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  spec.randomSeed() = 42;

  solver.addSolver(spec);
  // reproducible_results_end
}

void increase_limits() {
  /**
   * Increase limits to improve solution quality.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // increase_limits_start
  // Give more time
  LocalSearchSolverSpec spec;
  spec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  spec.solveTime() = 300; // 5 minutes instead of 30 seconds
  spec.stopAfterMoves() = 1000000; // 1M moves instead of 100K

  solver.addSolver(spec);
  // increase_limits_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all Local Search Solver examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "\n✓ All Local Search Solver examples completed successfully!\n";
  return 0;
}
// example_end
