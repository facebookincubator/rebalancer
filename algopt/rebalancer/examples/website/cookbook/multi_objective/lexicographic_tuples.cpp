// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Multi-Objective Optimization: Lexicographic Tuples Approach
 *
 * This example demonstrates how to optimize objectives in strict priority order
 * using tuples. The solver optimizes each priority level completely before
 * considering the next level.
 *
 * Use this approach when:
 * - You need guarantees on minimum quality for high-priority goals
 * - You're willing to completely ignore low-priority goals if needed
 * - You have true priority levels (e.g., correctness > performance > cost)
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

// solution_start

AssignmentSolution multi_objective_tuples() {
  // Use tuple optimization for strict priorities
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "storage-rebalancer", "production");

  solver.setObjectName("storage_node");
  solver.setContainerName("host");

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
  solver.addObjectDimension("cpu_usage", cpu_usage);
  solver.addObjectDimension("memory_usage", memory_usage);
  solver.addObjectDimension("data_size", data_size);

  // PRIORITY 0 (highest): Balance CPU
  // Optimize this FIRST, then move to next priority
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;

  solver.addGoal(balanceCpu, 1.0, 0); // Highest priority level (tuple 0)

  // PRIORITY 1: Balance memory
  // Only optimize AFTER CPU is optimized
  BalanceSpec balanceMem;
  balanceMem.name() = "balance-memory";
  balanceMem.scope() = "host";
  balanceMem.dimension() = "memory_usage";
  balanceMem.formula() = BalanceSpecFormula::LEGACY;
  balanceMem.fixAverageToInitial() = true;

  solver.addGoal(balanceMem, 1.0, 1); // Second priority level (tuple 1)

  // PRIORITY 2 (lowest): Minimize movement
  // Only matters after CPU and memory are optimized
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-movement";
  minMove.scope() = "host";
  minMove.dimension() = "data_size";

  solver.addGoal(minMove, 1.0, 2); // Lowest priority level (tuple 2)

  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 30000;
  solver.addSolver(localSearch);

  auto solution = solver.solve();
  return solution;
}

// solution_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  auto solution = multi_objective_tuples();
  std::cout << "AssignmentSolution found with final objective: "
            << *solution.finalObjective()->value() << std::endl;
  return 0;
}
