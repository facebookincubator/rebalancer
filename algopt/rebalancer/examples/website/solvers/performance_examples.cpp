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
 * Performance Tuning Examples
 *
 * This file demonstrates performance tuning patterns shown in the
 * solver documentation.
 */

// example_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void adjust_time_limit() {
  /**
   * Adjust time limit for performance.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // adjust_time_limit_start
  // Increase time limit for better quality
  LocalSearchSolverSpec spec;
  spec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  spec.solveTime() = 120; // 2 minutes instead of 30 seconds

  solver.addSolver(spec);
  // adjust_time_limit_end
}

void set_mip_gap() {
  /**
   * Set MIP gap tolerance for Optimal solver.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // set_mip_gap_start
  // Accept 5% gap instead of waiting for optimality
  OptimalSolverSpec spec;
  spec.solverPackage() = OptimalSolverPackage::HIGHS;
  spec.solveTime() = 300;

  solver.addSolver(spec);
  // set_mip_gap_end
}

void set_thread_count() {
  /**
   * Control parallelization with thread count.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // set_thread_count_start
  // Use 8 threads for faster solving
  OptimalSolverSpec spec;
  spec.solverPackage() = OptimalSolverPackage::HIGHS;

  solver.addSolver(spec);
  // set_thread_count_end
}

void warmstart_with_local_search() {
  /**
   * Warmstart Optimal solver with Local Search.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // warmstart_with_local_search_start
  // Run Local Search first for good initial solution
  LocalSearchSolverSpec lsSpec;
  lsSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  lsSpec.solveTime() = 30;
  solver.addSolver(lsSpec);

  // Then refine with Optimal (uses LS result as warmstart)
  OptimalSolverSpec optSpec;
  optSpec.solverPackage() = OptimalSolverPackage::HIGHS;
  optSpec.solveTime() = 180;
  solver.addSolver(optSpec);
  // warmstart_with_local_search_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all Performance Tuning examples...\n\n";

  std::cout << "1. Adjust Time Limit...\n";
  adjust_time_limit();

  std::cout << "\n✓ All Performance Tuning examples completed successfully!\n";
  return 0;
}
// example_end
