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
 * MinimizeSquaresSpec Reference Examples
 *
 * This file demonstrates all the usage patterns for MinimizeSquaresSpec shown
 * in the reference documentation. Each function is a complete, runnable
 * example.
 */

// example_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void quick_example() {
  /**
   * Quick example showing basic MinimizeSquaresSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 0.1},
          {"task1", 0.15},
          {"task2", 0.1},
          {"task3", 0.2},
          {"task4", 0.1},
          {"task5", 0.15},
      });

  // quick_example_start
  // Strong balance enforcement via sum-of-squares
  MinimizeSquaresSpec minimizeSpec;
  minimizeSpec.name() = "strong-balance";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "cpu_usage";
  minimizeSpec.lowerBound() = 0.0; // Minimum utilization
  minimizeSpec.upperBound() = 1.0; // Maximum utilization (optional)
  minimizeSpec.pieceCount() = 100; // Piecewise linear approximation pieces
  solver.addGoal(minimizeSpec, 1.0);
  // quick_example_end
}

void strong_load_balancing() {
  /**
   * Enforce very even load distribution.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 0.1},
          {"task1", 0.15},
          {"task2", 0.1},
          {"task3", 0.2},
          {"task4", 0.1},
          {"task5", 0.15},
      });

  // strong_load_balancing_start
  // Aggressively balance CPU
  MinimizeSquaresSpec minimizeSpec;
  minimizeSpec.name() = "even-cpu-distribution";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "cpu_usage";
  minimizeSpec.lowerBound() = 0.0;
  minimizeSpec.upperBound() = 1.0; // Utilization normalized to [0, 1]
  solver.addGoal(
      std::move(minimizeSpec), 2.0); // High weight for strong enforcement
  // strong_load_balancing_end
}

void multi_resource_strong_balance() {
  /**
   * Balance multiple resources with squared penalty.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 0.1},
          {"task1", 0.15},
          {"task2", 0.1},
          {"task3", 0.2},
          {"task4", 0.1},
          {"task5", 0.15},
      });
  solver.addObjectDimension(
      "memory_usage",
      std::map<std::string, double>{
          {"task0", 0.15},
          {"task1", 0.2},
          {"task2", 0.1},
          {"task3", 0.25},
          {"task4", 0.15},
          {"task5", 0.2},
      });

  // multi_resource_strong_balance_start
  // Strong CPU balance
  MinimizeSquaresSpec cpuSpec;
  cpuSpec.name() = "cpu-balance";
  cpuSpec.scope() = "host";
  cpuSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(cpuSpec), 2.0);

  // Strong memory balance
  MinimizeSquaresSpec memorySpec;
  memorySpec.name() = "memory-balance";
  memorySpec.scope() = "host";
  memorySpec.dimension() = "memory_usage";
  solver.addGoal(std::move(memorySpec), 2.0);
  // multi_resource_strong_balance_end
}

void filtered_strong_balance() {
  /**
   * Apply strong balancing only to specific hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"prod_host_1", {"task0", "task1"}},
          {"prod_host_2", {"task2"}},
          {"prod_host_3", {"task3", "task4"}},
          {"dev_host_1", {"task5"}},
      });
  solver.addObjectDimension(
      "query_rate",
      std::map<std::string, double>{
          {"task0", 100.0},
          {"task1", 200.0},
          {"task2", 150.0},
          {"task3", 300.0},
          {"task4", 100.0},
          {"task5", 50.0},
      });

  // filtered_strong_balance_start
  // Production hosts need very even distribution
  std::vector<std::string> prod_hosts = {
      "prod_host_1", "prod_host_2", "prod_host_3"};

  MinimizeSquaresSpec minimizeSpec;
  minimizeSpec.name() = "prod-strong-balance";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "query_rate";
  Filter filter;
  filter.itemsWhitelist() = std::move(prod_hosts);
  minimizeSpec.filter() = std::move(filter);
  solver.addGoal(std::move(minimizeSpec), 3.0);
  // filtered_strong_balance_end
}

void rack_level_balance() {
  /**
   * Strong balance at rack level.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2"}},
          {"host2", {"task3"}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "network_traffic",
      std::map<std::string, double>{
          {"task0", 5.0},
          {"task1", 10.0},
          {"task2", 8.0},
          {"task3", 3.0},
          {"task4", 7.0},
          {"task5", 6.0},
      });

  // Add rack scope
  std::map<std::string, std::string> host_to_rack = {
      {"host0", "rack0"},
      {"host1", "rack0"},
      {"host2", "rack1"},
      {"host3", "rack1"},
  };
  solver.addScope("rack", host_to_rack);

  // rack_level_balance_start
  // Ensure racks are evenly loaded
  MinimizeSquaresSpec minimizeSpec;
  minimizeSpec.name() = "rack-balance";
  minimizeSpec.scope() = "rack";
  minimizeSpec.dimension() = "network_traffic";
  solver.addGoal(std::move(minimizeSpec), 1.5);
  // rack_level_balance_end
}

void upper_bound_for_headroom() {
  /**
   * Use upperBound to create headroom target.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 0.1},
          {"task1", 0.15},
          {"task2", 0.1},
          {"task3", 0.2},
          {"task4", 0.1},
          {"task5", 0.15},
      });

  // upper_bound_headroom_start
  // Balance but keep utilization under 80%
  MinimizeSquaresSpec minimizeSpec;
  minimizeSpec.name() = "balance-with-headroom";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "cpu_usage";
  minimizeSpec.lowerBound() = 0.0;
  minimizeSpec.upperBound() = 0.8; // Treat >80% same as 80% (softcap)
  solver.addGoal(std::move(minimizeSpec), 2.0);
  // upper_bound_headroom_end
}

void piecewise_approximation_tuning() {
  /**
   * Adjust pieceCount for accuracy vs performance.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 0.1},
          {"task1", 0.15},
          {"task2", 0.1},
          {"task3", 0.2},
          {"task4", 0.1},
          {"task5", 0.15},
      });

  // piecewise_tuning_high_accuracy_start
  // High accuracy (slower solve)
  MinimizeSquaresSpec minimizeSpec;
  minimizeSpec.name() = "high-accuracy-balance";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "cpu_usage";
  minimizeSpec.pieceCount() = 200; // More pieces = more accurate approximation
  solver.addGoal(std::move(minimizeSpec), 1.0);
  // piecewise_tuning_high_accuracy_end
}

void piecewise_fast_solve() {
  /**
   * Fast solve with fewer pieces.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 0.1},
          {"task1", 0.15},
          {"task2", 0.1},
          {"task3", 0.2},
          {"task4", 0.1},
          {"task5", 0.15},
      });

  // piecewise_tuning_fast_start
  // Fast solve (less accurate)
  MinimizeSquaresSpec minimizeSpec;
  minimizeSpec.name() = "fast-balance";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "cpu_usage";
  minimizeSpec.pieceCount() = 20; // Fewer pieces = faster but rougher
  solver.addGoal(std::move(minimizeSpec), 1.0);
  // piecewise_tuning_fast_end
}

void combining_with_capacity() {
  /**
   * Balance within capacity limits.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 4.0},
          {"task1", 8.0},
          {"task2", 2.0},
          {"task3", 6.0},
          {"task4", 4.0},
          {"task5", 8.0},
      });
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{
          {"host0", 32.0},
          {"host1", 32.0},
          {"host2", 32.0},
          {"host3", 32.0},
      });

  // combining_capacity_start
  // Hard: Capacity
  CapacitySpec capacitySpec;
  capacitySpec.name() = "capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  solver.addConstraint(capacitySpec);

  // Soft: Strong balance (within capacity)
  MinimizeSquaresSpec minimizeSpec;
  minimizeSpec.name() = "balance";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "memory";
  solver.addGoal(minimizeSpec, 2.0);
  // combining_capacity_end
}

void combining_with_minimize_movement() {
  /**
   * Balance but limit churn.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 0.1},
          {"task1", 0.15},
          {"task2", 0.1},
          {"task3", 0.2},
          {"task4", 0.1},
          {"task5", 0.15},
      });

  // combining_minimize_movement_start
  // Primary: Strong balance
  MinimizeSquaresSpec minimizeSpec;
  minimizeSpec.name() = "balance";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(minimizeSpec), 3.0);

  // Secondary: Limit movement
  MinimizeMovementSpec movementSpec;
  movementSpec.name() = "minimize-moves";
  movementSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(movementSpec), 1.0);
  // combining_minimize_movement_end
}

void complete_example() {
  /**
   * Complete runnable example demonstrating MinimizeSquaresSpec.
   *
   * This example shows how to use MinimizeSquaresSpec to achieve very even
   * distribution of CPU and memory across hosts using quadratic penalty.
   */
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "minimize-squares-example", "production");

  solver.setObjectName("task");
  solver.setContainerName("host");

  // Initial imbalanced assignment
  std::map<std::string, std::vector<std::string>> assignment = {
      {"host0", {"task0", "task1", "task2", "task3", "task4", "task5"}},
      {"host1", {"task6"}},
      {"host2", {}},
      {"host3", {"task7", "task8", "task9"}},
  };
  solver.setAssignment(assignment);

  // Task resources (normalized to [0, 1] range for utilization)
  std::map<std::string, double> cpu_usage;
  std::map<std::string, double> memory_usage;
  for (const auto i : folly::irange(10)) {
    cpu_usage[fmt::format("task{}", i)] = 0.1 + (i % 3) * 0.05;
    memory_usage[fmt::format("task{}", i)] = 0.15 + (i % 4) * 0.05;
  }

  solver.addObjectDimension("cpu_usage", cpu_usage);
  solver.addObjectDimension("memory_usage", memory_usage);

  // Host capacities (normalized to 1.0 for utilization)
  std::map<std::string, double> host_cpu_capacity;
  std::map<std::string, double> host_memory_capacity;
  for (const auto i : folly::irange(4)) {
    host_cpu_capacity[fmt::format("host{}", i)] = 1.0;
    host_memory_capacity[fmt::format("host{}", i)] = 1.0;
  }

  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);
  solver.addContainerDimension("memory_capacity", host_memory_capacity);

  // Add capacity constraints
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu_usage";
  solver.addConstraint(cpuCapacity);

  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory_usage";
  solver.addConstraint(memoryCapacity);

  // Add strong balance goals using MinimizeSquaresSpec
  MinimizeSquaresSpec cpuBalance;
  cpuBalance.name() = "strong-cpu-balance";
  cpuBalance.scope() = "host";
  cpuBalance.dimension() = "cpu_usage";
  cpuBalance.lowerBound() = 0.0;
  cpuBalance.upperBound() = 1.0;
  cpuBalance.pieceCount() = 100;
  solver.addGoal(cpuBalance, 2.0);

  MinimizeSquaresSpec memoryBalance;
  memoryBalance.name() = "strong-memory-balance";
  memoryBalance.scope() = "host";
  memoryBalance.dimension() = "memory_usage";
  memoryBalance.lowerBound() = 0.0;
  memoryBalance.upperBound() = 1.0;
  memoryBalance.pieceCount() = 100;
  solver.addGoal(memoryBalance, 2.0);

  // Add minimize movement as secondary goal
  MinimizeMovementSpec movementSpec;
  movementSpec.name() = "minimize-moves";
  movementSpec.dimension() = "cpu_usage";
  solver.addGoal(movementSpec, 0.5);

  // Solve
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 10;
  solver.addSolver(localSearch);

  auto solution = solver.solve();

  // Print results
  std::cout << fmt::format(
      "AssignmentSolution found in {}ms\n",
      *solution.problemProfile()->solveSec());
  std::cout << fmt::format(
      "Objective value: {}\n", *solution.finalObjective()->value());

  // Calculate balance metrics
  // assignment() is object -> container, build reverse map
  std::map<std::string, std::vector<std::string>> host_to_tasks;
  for (const auto& [task, host] : *solution.assignment()) {
    host_to_tasks[host].push_back(task);
  }

  std::vector<double> cpu_utilizations;
  std::vector<double> memory_utilizations;

  std::vector<std::string> hosts = {"host0", "host1", "host2", "host3"};
  for (const auto& host : hosts) {
    auto it = host_to_tasks.find(host);
    const std::vector<std::string> empty;
    const auto& tasks = (it != host_to_tasks.end()) ? it->second : empty;

    double cpu = 0.0;
    double memory = 0.0;
    for (const auto& task : tasks) {
      cpu += cpu_usage[task];
      memory += memory_usage[task];
    }

    cpu_utilizations.push_back(cpu);
    memory_utilizations.push_back(memory);

    std::cout << fmt::format("\n{}:\n", host);
    std::cout << fmt::format("  Tasks: {}\n", tasks.size());
    std::cout << fmt::format("  CPU: {:.2f}%\n", cpu * 100);
    std::cout << fmt::format("  Memory: {:.2f}%\n", memory * 100);
  }

  // Calculate sum of squares
  double cpu_sum_squares = 0.0;
  double memory_sum_squares = 0.0;
  for (double u : cpu_utilizations) {
    cpu_sum_squares += u * u;
  }
  for (double u : memory_utilizations) {
    memory_sum_squares += u * u;
  }

  std::cout << "\nBalance quality:\n";
  std::cout << fmt::format("  CPU sum of squares: {:.4f}\n", cpu_sum_squares);
  std::cout << fmt::format(
      "  Memory sum of squares: {:.4f}\n", memory_sum_squares);

  double cpu_avg = 0.0;
  for (double u : cpu_utilizations) {
    cpu_avg += u;
  }
  cpu_avg /= cpu_utilizations.size();

  double cpu_variance = 0.0;
  for (double u : cpu_utilizations) {
    cpu_variance += (u - cpu_avg) * (u - cpu_avg);
  }
  cpu_variance /= cpu_utilizations.size();

  std::cout << fmt::format(
      "  CPU std dev: {:.2f}%\n", std::sqrt(cpu_variance) * 100);
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all MinimizeSquares examples...\n";

  std::cout << "\n1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Strong Load Balancing...\n";
  strong_load_balancing();

  std::cout << "\n3. Multi Resource Strong Balance...\n";
  multi_resource_strong_balance();

  std::cout << "\n4. Filtered Strong Balance...\n";
  filtered_strong_balance();

  std::cout << "\n5. Rack Level Balance...\n";
  rack_level_balance();

  std::cout << "\n6. Upper Bound For Headroom...\n";
  upper_bound_for_headroom();

  std::cout << "\n7. Piecewise Approximation Tuning...\n";
  piecewise_approximation_tuning();

  std::cout << "\n8. Piecewise Fast Solve...\n";
  piecewise_fast_solve();

  std::cout << "\n9. Combining With Capacity...\n";
  combining_with_capacity();

  std::cout << "\n10. Combining With Minimize Movement...\n";
  combining_with_minimize_movement();

  std::cout << "\n11. Complete Example...\n";
  complete_example();

  std::cout << "\n✓ All MinimizeSquares examples completed successfully!\n";
  return 0;
}
// example_end
