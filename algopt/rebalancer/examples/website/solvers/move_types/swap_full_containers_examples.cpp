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
 * SwapFullContainers Move Type Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
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
  // Exchange entire container contents
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullContainersMoveTypeSpec() =
      SwapFullContainersMoveTypeSpec{};

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
  solverSpec.moveTypeList()->back().swapFullContainersMoveTypeSpec() =
      SwapFullContainersMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem with imbalanced containers
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}}, // 12GB - overloaded
          {"host1", {"task3"}}, // 3GB - underutilized
          {"host2", {}}, // Empty
      });

  std::map<std::string, double> memory = {
      {"task0", 4.0},
      {"task1", 4.0},
      {"task2", 4.0},
      {"task3", 3.0},
  };
  solver.addObjectDimension("memory", memory);

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 16.0;
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

// server_migration_start
void serverMigration() {
  // Migrate all shards from overloaded server to another
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullContainersMoveTypeSpec() =
      SwapFullContainersMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// server_migration_end

void serverMigrationComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullContainersMoveTypeSpec() =
      SwapFullContainersMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"shard0", "shard1", "shard2"}}, // 15GB - overloaded
          {"server1", {"shard3", "shard4"}}, // 8GB
          {"server2", {"shard5"}}, // 4GB
      });

  std::map<std::string, double> memory = {
      {"shard0", 5.0},
      {"shard1", 5.0},
      {"shard2", 5.0},
      {"shard3", 4.0},
      {"shard4", 4.0},
      {"shard5", 4.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "server";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 16.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "memory";
  solver.addGoal(std::move(balanceSpec), 1.0);

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

// consolidation_start
void consolidation() {
  // Container-level rebalancing for similar-sized containers
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullContainersMoveTypeSpec() =
      SwapFullContainersMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// consolidation_end

void consolidationComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullContainersMoveTypeSpec() =
      SwapFullContainersMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}}, // 8GB
          {"host1", {"task2", "task3"}}, // 8GB
          {"host2", {"task4"}}, // 4GB
      });

  std::map<std::string, double> memory = {
      {"task0", 4.0},
      {"task1", 4.0},
      {"task2", 4.0},
      {"task3", 4.0},
      {"task4", 4.0},
  };
  solver.addObjectDimension("memory", memory);

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 12.0;
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

// combined_start
void combined() {
  // Multi-stage: coarse container rebalancing, then fine object placement
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  // Add SwapFullContainers
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullContainersMoveTypeSpec() =
      SwapFullContainersMoveTypeSpec{};

  // Add Swap
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// combined_end

void combinedComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullContainersMoveTypeSpec() =
      SwapFullContainersMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}}, // 9GB
          {"host1", {"task3"}}, // 3GB
          {"host2", {"task4", "task5"}}, // 6GB
      });

  std::map<std::string, double> memory = {
      {"task0", 3.0},
      {"task1", 3.0},
      {"task2", 3.0},
      {"task3", 3.0},
      {"task4", 3.0},
      {"task5", 3.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 12.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "memory";
  solver.addGoal(std::move(balanceSpec), 1.0);

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

// capacity_constrained_start
void capacityConstrained() {
  // Full swaps can work when individual moves are capacity-blocked
  // Note: Full swaps may violate capacity if container sizes differ
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  // Add SwapFullContainers
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullContainersMoveTypeSpec() =
      SwapFullContainersMoveTypeSpec{};

  // Add Swap as fallback
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// capacity_constrained_end

void capacityConstrainedComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullContainersMoveTypeSpec() =
      SwapFullContainersMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem where hosts are at capacity
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}}, // 8GB - full
          {"host1", {"task2", "task3"}}, // 8GB - full
          {"host2", {"task4"}}, // 4GB
      });

  std::map<std::string, double> memory = {
      {"task0", 4.0},
      {"task1", 4.0},
      {"task2", 4.0},
      {"task3", 4.0},
      {"task4", 4.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 8.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "memory";
  solver.addGoal(std::move(balanceSpec), 1.0);

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
  std::cout
      << "=== Quick example showing basic SwapFullContainers usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
