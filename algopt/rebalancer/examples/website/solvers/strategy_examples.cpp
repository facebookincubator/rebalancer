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
 * Solver Strategy Examples
 *
 * This file demonstrates solver strategy patterns shown in the
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
  solver.addObjectDimension("cpu", std::move(cpu));

  std::map<std::string, double> memory;
  memory["task0"] = 4.0;
  memory["task1"] = 3.0;
  memory["task2"] = 5.0;
  memory["task3"] = 2.0;
  solver.addObjectDimension("memory", std::move(memory));

  std::map<std::string, double> data_size;
  data_size["task0"] = 10.0;
  data_size["task1"] = 20.0;
  data_size["task2"] = 15.0;
  data_size["task3"] = 25.0;
  solver.addObjectDimension("data_size", std::move(data_size));
}

void sequential_solvers() {
  /**
   * Use multiple solvers sequentially.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  setupMinimalProblem(solver);

  // sequential_solvers_start
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
  // sequential_solvers_end
}

void time_limited_approach() {
  /**
   * Time-limited solving approach.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  setupMinimalProblem(solver);

  // time_limited_approach_start
  // Give Local Search 30 seconds to find good solution
  LocalSearchSolverSpec lsSpec;
  lsSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  lsSpec.solveTime() = 30;
  solver.addSolver(lsSpec);

  // Then give Optimal up to 5 minutes to refine
  OptimalSolverSpec optSpec;
  optSpec.solverPackage() = OptimalSolverPackage::HIGHS;
  optSpec.solveTime() = 300;
  solver.addSolver(optSpec);
  // time_limited_approach_end
}

void goal_prioritization() {
  /**
   * Prioritize goals with different weights.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  setupMinimalProblem(solver);

  // goal_prioritization_start
  // High priority: balance load
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 10.0);

  // Low priority: minimize movement
  MinimizeMovementSpec moveSpec;
  moveSpec.name() = "minimize-moves";
  moveSpec.scope() = "host";
  moveSpec.dimension() = "data_size";
  solver.addGoal(std::move(moveSpec), 1.0);
  // goal_prioritization_end
}

void constraint_vs_goal() {
  /**
   * Use constraints for hard requirements, goals for preferences.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  setupMinimalProblem(solver);

  // constraint_vs_goal_start
  // Hard constraint: don't exceed capacity
  CapacitySpec capacitySpec;
  capacitySpec.name() = "capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  solver.addConstraint(std::move(capacitySpec));

  // Soft goal: prefer balanced CPU
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 1.0);
  // constraint_vs_goal_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all Solver Strategy examples...\n\n";

  std::cout << "1. Sequential Solvers...\n";
  sequential_solvers();

  std::cout << "\n✓ All Solver Strategy examples completed successfully!\n";
  return 0;
}
// example_end
