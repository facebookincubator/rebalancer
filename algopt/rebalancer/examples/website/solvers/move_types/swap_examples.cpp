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
 * Swap Move Type Reference Examples
 *
 * This file demonstrates all the usage patterns for Swap move type shown in the
 * reference documentation. Each function is a complete, runnable example.
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
  // Use Swap for capacity-constrained problems
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  // Add Swap
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

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
  solver.addSolver(solverSpec);

  // Setup capacity-constrained problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}}, // Full (4GB)
          {"host1", {"task2"}}, // Full (4GB)
          {"host2", {}}, // Empty
      });

  std::map<std::string, double> memory = {
      {"task0", 2.0},
      {"task1", 2.0},
      {"task2", 4.0},
  };
  solver.addObjectDimension("memory", memory);

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 4.0;
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

// basic_capacity_constrained_start
void basicCapacityConstrained() {
  // Capacity is tight - need swaps to make progress
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 64.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// basic_capacity_constrained_end

void basicCapacityConstrainedComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 64.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem where hosts are at capacity
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}}, // 64GB - full
          {"host1", {"task3", "task4"}}, // 64GB - full
          {"host2", {"task5"}}, // 32GB - partial
      });

  std::map<std::string, double> memory = {
      {"task0", 32.0},
      {"task1", 16.0},
      {"task2", 16.0},
      {"task3", 48.0},
      {"task4", 16.0},
      {"task5", 32.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

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

// greedy_swap_start
void greedySwap() {
  // Exit early when improvement found (faster)
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  SwapMoveTypeSpec swapSpec;
  swapSpec.greedyOnSrc() = true;
  swapSpec.greedyOnDst() = true;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// greedy_swap_end

void greedySwapComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  SwapMoveTypeSpec swapSpec;
  swapSpec.greedyOnSrc() = true;
  swapSpec.greedyOnDst() = true;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;
  solver.addSolver(solverSpec);

  // Setup capacity-constrained problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}}, // 48GB
          {"host1", {"task3", "task4"}}, // 32GB
          {"host2", {"task5"}}, // 16GB
      });

  std::map<std::string, double> memory = {
      {"task0", 16.0},
      {"task1", 16.0},
      {"task2", 16.0},
      {"task3", 16.0},
      {"task4", 16.0},
      {"task5", 16.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 64.0;
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

// sampled_swap_start
void sampledSwap() {
  // For large problems, sample subset of objects
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 100;
  swapSpec.sampleSize() = std::move(sampleSize);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// sampled_swap_end

void sampledSwapComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 100;
  swapSpec.sampleSize() = std::move(sampleSize);
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;
  solver.addSolver(solverSpec);

  // Setup capacity-constrained problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}}, // 8GB - full
          {"host1", {"task2", "task3"}}, // 6GB
          {"host2", {"task4"}}, // 3GB
      });

  std::map<std::string, double> memory = {
      {"task0", 5.0},
      {"task1", 3.0},
      {"task2", 3.0},
      {"task3", 3.0},
      {"task4", 3.0},
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

// swap_within_groups_start
void swapWithinGroups() {
  // Only swap objects from the same group (e.g., same database shard)
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  SwapMoveTypeSpec swapSpec;
  swapSpec.partitionNameToExploreSwapsWithinObjectGroup() = "shard";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// swap_within_groups_end

void swapWithinGroupsComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  SwapMoveTypeSpec swapSpec;
  swapSpec.partitionNameToExploreSwapsWithinObjectGroup() = "shard";
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}}, // 8GB - full
          {"host1", {"task2"}}, // 4GB
          {"host2", {"task3"}}, // 4GB
      });

  std::map<std::string, double> memory = {
      {"task0", 4.0},
      {"task1", 4.0},
      {"task2", 4.0},
      {"task3", 4.0},
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

// restricted_destinations_start
void restrictedDestinations() {
  // Only swap within the same rack (limit destinations)
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  SwapMoveTypeSpec swapSpec;
  MoveToCurrentScopeItemSpec destSpec;
  destSpec.scopeNameForExploringMovesToCurrentScopeItem() = "rack";
  swapSpec.destinationsToExplore().ensure().moveToCurrentScopeItem() = destSpec;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// restricted_destinations_end

void restrictedDestinationsComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  SwapMoveTypeSpec swapSpec;
  MoveToCurrentScopeItemSpec destSpec;
  destSpec.scopeNameForExploringMovesToCurrentScopeItem() = "rack";
  swapSpec.destinationsToExplore().ensure().moveToCurrentScopeItem() = destSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}}, // 6GB
          {"host1", {"task2"}}, // 3GB
          {"host2", {"task3"}}, // 3GB
      });

  std::map<std::string, double> memory = {
      {"task0", 3.0},
      {"task1", 3.0},
      {"task2", 3.0},
      {"task3", 3.0},
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
  std::cout << "=== Quick example showing basic Swap move type usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
