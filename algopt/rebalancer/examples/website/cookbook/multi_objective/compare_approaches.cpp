// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Multi-Objective Optimization: Comparing Different Approaches
 *
 * This example demonstrates how to compare different multi-objective
 * optimization approaches (weighted sum, lexicographic tuples, and hybrid) to
 * understand their trade-offs and select the best approach for your use case.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

// variation_start

void setupProblem(ProblemSolver& solver) {
  // Set up assignment: 20 storage nodes across 5 hosts (imbalanced)
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(12)) {
    assignment["host0"].push_back("node" + std::to_string(i));
  }
  for (const auto i : folly::irange(12, 20)) {
    assignment["host1"].push_back("node" + std::to_string(i));
  }
  assignment["host2"] = {};
  assignment["host3"] = {};
  assignment["host4"] = {};
  solver.setAssignment(assignment);

  // Define dimensions
  std::map<std::string, double> cpu_usage, memory_usage, data_size;
  for (const auto i : folly::irange(20)) {
    const std::string name = "node" + std::to_string(i);
    cpu_usage[name] = 1.0 + (i % 4) * 0.5;
    memory_usage[name] = 2.0 + (i % 3) * 1.0;
    data_size[name] = 10.0 + (i % 5) * 5.0;
  }
  solver.addObjectDimension("cpu_usage", std::move(cpu_usage));
  solver.addObjectDimension("memory_usage", std::move(memory_usage));
  solver.addObjectDimension("data_size", std::move(data_size));
}

AssignmentSolution multi_objective_weighted() {
  // Use weighted sum of objectives
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "storage-rebalancer", "production");

  solver.setObjectName("storage_node");
  solver.setContainerName("host");

  setupProblem(solver);

  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;
  solver.addGoal(balanceCpu, 10.0);

  BalanceSpec balanceMem;
  balanceMem.name() = "balance-memory";
  balanceMem.scope() = "host";
  balanceMem.dimension() = "memory_usage";
  balanceMem.formula() = BalanceSpecFormula::LEGACY;
  balanceMem.fixAverageToInitial() = true;
  solver.addGoal(balanceMem, 5.0);

  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-movement";
  minMove.scope() = "host";
  minMove.dimension() = "data_size";
  solver.addGoal(minMove, 1.0);

  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 30000;
  solver.addSolver(localSearch);

  return solver.solve();
}

AssignmentSolution multi_objective_tuples() {
  // Use tuple optimization for strict priorities
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "storage-rebalancer", "production");

  solver.setObjectName("storage_node");
  solver.setContainerName("host");

  setupProblem(solver);

  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;

  solver.addGoal(balanceCpu, 1.0, 0);

  BalanceSpec balanceMem;
  balanceMem.name() = "balance-memory";
  balanceMem.scope() = "host";
  balanceMem.dimension() = "memory_usage";
  balanceMem.formula() = BalanceSpecFormula::LEGACY;
  balanceMem.fixAverageToInitial() = true;

  solver.addGoal(balanceMem, 1.0, 1);

  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-movement";
  minMove.scope() = "host";
  minMove.dimension() = "data_size";

  solver.addGoal(minMove, 1.0, 2);

  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 30000;
  solver.addSolver(localSearch);

  return solver.solve();
}

AssignmentSolution multi_objective_hybrid() {
  // Use tuples for major priorities, weights within each tuple
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "storage-rebalancer", "production");

  solver.setObjectName("storage_node");
  solver.setContainerName("host");

  setupProblem(solver);

  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;

  solver.addGoal(balanceCpu, 2.0, 0);

  BalanceSpec balanceMem;
  balanceMem.name() = "balance-memory";
  balanceMem.scope() = "host";
  balanceMem.dimension() = "memory_usage";
  balanceMem.formula() = BalanceSpecFormula::LEGACY;
  balanceMem.fixAverageToInitial() = true;

  solver.addGoal(balanceMem, 1.0, 0);

  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-movement";
  minMove.scope() = "host";
  minMove.dimension() = "data_size";

  solver.addGoal(minMove, 1.0, 1);

  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 30000;
  solver.addSolver(localSearch);

  return solver.solve();
}

void compare_approaches() {
  // Weighted approach
  auto solution1 = multi_objective_weighted();

  // Tuple approach
  auto solution2 = multi_objective_tuples();

  // Hybrid approach
  auto solution3 = multi_objective_hybrid();

  // Compare objectives
  std::cout << "Approach | CPU Balance | Memory Balance | Data Moved\n";
  std::cout << std::string(60, '-') << "\n";
  // TODO: Extract and compare metrics
}

// variation_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  compare_approaches();
  return 0;
}
