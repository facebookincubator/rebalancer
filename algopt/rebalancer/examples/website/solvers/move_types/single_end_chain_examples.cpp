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
 * SingleEndChain Move Type Reference Examples
 *
 * This file demonstrates all the usage patterns for SingleEndChain move type
 * shown in the reference documentation. Each function is a complete, runnable
 * example.
 * */

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

// quick_example_start
void quickExample() {
  // Use SingleEndChain for high-quality capacity-constrained solutions
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleEndChainMoveTypeSpec() =
      SingleEndChainMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// quick_example_end

void quickExampleComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleEndChainMoveTypeSpec() =
      SingleEndChainMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup capacity-constrained problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3"}},
          {"host1", {"task4", "task5"}},
          {"host2", {}},
      });

  std::map<std::string, double> memory = {
      {"task0", 2.0},
      {"task1", 2.0},
      {"task2", 2.0},
      {"task3", 2.0},
      {"task4", 4.0},
      {"task5", 4.0},
  };
  solver.addObjectDimension("memory", memory);

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 8.0;
  capacitySpec.limit() = limit;
  solver.addConstraint(capacitySpec);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "memory";
  solver.addGoal(balanceSpec, 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial objective: " << *solution.initialObjective()->value()
            << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << *solution.initialObjective()->value() -
          *solution.finalObjective()->value()
            << "\n";
}

// basic_configuration_start
void basicConfiguration() {
  // Use after Single and Swap for better quality
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleEndChainMoveTypeSpec() =
      SingleEndChainMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// basic_configuration_end

// high_quality_start
void highQuality() {
  // Best quality for capacity-constrained problems
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;
  solverSpec.solveTime() = 600; // 10 minutes - allow time for expensive moves

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleEndChainMoveTypeSpec() =
      SingleEndChainMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// high_quality_end

// production_rebalancing_start
void productionRebalancing() {
  // Balance quality and time for production
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 10.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "memory";
  solver.addGoal(std::move(balanceSpec), 10.0);

  MinimizeMovementSpec moveSpec;
  moveSpec.name() = "minimize-moves";
  solver.addGoal(std::move(moveSpec), 1.0);

  LocalSearchSolverSpec solverSpec;
  solverSpec.solveTime() = 300; // 5 minutes limit

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleEndChainMoveTypeSpec() =
      SingleEndChainMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// production_rebalancing_end

void productionRebalancingComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 10.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "memory";
  solver.addGoal(std::move(balanceSpec), 10.0);

  MinimizeMovementSpec moveSpec;
  moveSpec.name() = "minimize-moves";
  solver.addGoal(std::move(moveSpec), 1.0);

  LocalSearchSolverSpec solverSpec;
  solverSpec.solveTime() = 300;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleEndChainMoveTypeSpec() =
      SingleEndChainMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup production-like problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0", "shard1", "shard2"}},
          {"host1", {"shard3", "shard4", "shard5"}},
          {"host2", {"shard6"}},
          {"host3", {}},
      });

  std::map<std::string, double> memory = {
      {"shard0", 3.0},
      {"shard1", 2.0},
      {"shard2", 3.0},
      {"shard3", 2.5},
      {"shard4", 2.5},
      {"shard5", 2.0},
      {"shard6", 7.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial objective: " << *solution.initialObjective()->value()
            << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << *solution.initialObjective()->value() -
          *solution.finalObjective()->value()
            << "\n";
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "=== Quick example showing basic SingleEndChain usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
