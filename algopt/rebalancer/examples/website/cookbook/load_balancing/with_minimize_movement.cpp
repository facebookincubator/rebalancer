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
 * Load Balancing with Minimize Movement
 *
 * This variation prefers solutions that move fewer tasks, reducing disruption
 * during rebalancing.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "load-balancer", "production");

  solver.setObjectName("task");
  solver.setContainerName("host");

  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(40)) {
    assignment["host0"].push_back(fmt::format("task{}", i));
  }
  for (const auto i : folly::irange(40, 70)) {
    assignment["host1"].push_back(fmt::format("task{}", i));
  }
  for (const auto i : folly::irange(70, 100)) {
    assignment["host2"].push_back(fmt::format("task{}", i));
  }
  for (const auto i : folly::irange(3, 10)) {
    assignment[fmt::format("host{}", i)] = {};
  }
  solver.setAssignment(assignment);

  std::map<std::string, double> cpu_usage;
  for (const auto i : folly::irange(100)) {
    cpu_usage[fmt::format("task{}", i)] = 1.0 + (i % 5) * 0.5;
  }
  solver.addObjectDimension("cpu_usage", cpu_usage);

  std::map<std::string, double> memory_usage;
  for (const auto i : folly::irange(100)) {
    memory_usage[fmt::format("task{}", i)] = 2.0 + (i % 3) * 1.0;
  }
  solver.addObjectDimension("memory_usage", memory_usage);

  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;
  solver.addGoal(balanceCpu, 1.0);

  BalanceSpec balanceMem;
  balanceMem.name() = "balance-memory";
  balanceMem.scope() = "host";
  balanceMem.dimension() = "memory_usage";
  balanceMem.formula() = BalanceSpecFormula::LEGACY;
  balanceMem.fixAverageToInitial() = true;
  solver.addGoal(balanceMem, 1.0);

  // variation_start
  MinimizeMovementSpec minMoves;
  minMoves.name() = "minimize-moves";
  solver.addGoal(minMoves, 0.1); // Lower priority than balancing
  // variation_end

  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 10000;
  solver.addSolver(localSearch);

  auto solution = solver.solve();

  std::cout << "AssignmentSolution found in " << "<solve time>" << "ms\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "Objects moved: " << "<move count>" << "\n";

  return 0;
}
