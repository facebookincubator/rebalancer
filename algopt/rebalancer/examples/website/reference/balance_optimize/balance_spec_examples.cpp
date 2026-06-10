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
 * BalanceSpec Reference Examples
 *
 * This file demonstrates all the usage patterns for BalanceSpec shown in the
 * reference documentation. Each function is a complete, runnable example.
 */

// example_start
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

void quick_example() {
  /**
   * Quick example showing basic BalanceSpec usage.
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
          {"task0", 2.0},
          {"task1", 3.0},
          {"task2", 1.0},
          {"task3", 4.0},
          {"task4", 2.0},
          {"task5", 3.0},
      });

  // quick_example_start
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu_usage";
  balanceSpec.formula() = BalanceSpecFormula::LEGACY;
  balanceSpec.fixAverageToInitial() = true;
  solver.addGoal(balanceSpec, 1.0);
  // quick_example_end
}

void basic_load_balancing() {
  /**
   * Basic load balancing - balance task count across hosts.
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

  // basic_load_balancing_start
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-tasks";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "task_count"; // Auto-created for object count
  balanceSpec.formula() = BalanceSpecFormula::LEGACY;
  balanceSpec.fixAverageToInitial() = true;
  solver.addGoal(std::move(balanceSpec), 1.0);
  // basic_load_balancing_end
}

void balance_with_fixed_average() {
  /**
   * Balance with fixed average - maintain total load while rebalancing.
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
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 3.0},
          {"task2", 1.0},
          {"task3", 4.0},
          {"task4", 2.0},
          {"task5", 3.0},
      });

  // balance_fixed_average_start
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  balanceSpec.formula() = BalanceSpecFormula::LEGACY;
  balanceSpec.fixAverageToInitial() = true; // Don't change total CPU
  solver.addGoal(std::move(balanceSpec), 1.0);
  // balance_fixed_average_end
}

void multi_level_balancing() {
  /**
   * Multi-level balancing - balance at both host and rack level.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {"task4"}},
          {"host3", {"task5"}},
      });
  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 3.0},
          {"task2", 1.0},
          {"task3", 4.0},
          {"task4", 2.0},
          {"task5", 3.0},
      });

  // Add rack scope
  std::map<std::string, std::string> host_to_rack = {
      {"host0", "rack0"},
      {"host1", "rack0"},
      {"host2", "rack1"},
      {"host3", "rack1"},
  };
  solver.addScope("rack", host_to_rack);

  // multi_level_balancing_start
  // Fine-grained: balance across hosts
  BalanceSpec balanceHosts;
  balanceHosts.name() = "balance-hosts";
  balanceHosts.scope() = "host";
  balanceHosts.dimension() = "cpu";
  solver.addGoal(std::move(balanceHosts), 1.0);

  // Coarse-grained: balance across racks
  BalanceSpec balanceRacks;
  balanceRacks.name() = "balance-racks";
  balanceRacks.scope() = "rack";
  balanceRacks.dimension() = "cpu";
  solver.addGoal(
      std::move(balanceRacks), 0.5); // Less important than host-level
  // multi_level_balancing_end
}

void balance_with_upper_bound() {
  /**
   * Balance with upper bound - allow some imbalance, but not too much.
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

  // balance_upper_bound_start
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "memory";
  balanceSpec.upperBound() = 1.2; // Allow up to 120% of average
  balanceSpec.boundType() = BalanceSpecBoundType::RELATIVE;
  solver.addGoal(std::move(balanceSpec), 1.0);
  // balance_upper_bound_end
}

void balance_with_datacenter_drain() {
  /**
   * Balance across datacenters when draining one DC.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {"task4"}},
          {"host3", {"task5"}},
      });

  // Add datacenter scope
  std::map<std::string, std::string> host_to_dc = {
      {"host0", "dc1"},
      {"host1", "dc1"},
      {"host2", "dc2"},
      {"host3", "dc2"},
  };
  solver.addScope("datacenter", host_to_dc);
  solver.addObjectDimension(
      "objects",
      std::map<std::string, double>{
          {"task0", 1.0},
          {"task1", 1.0},
          {"task2", 1.0},
          {"task3", 1.0},
          {"task4", 1.0},
          {"task5", 1.0},
      });

  // balance_datacenter_drain_start
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-across-dcs";
  balanceSpec.scope() = "datacenter";
  balanceSpec.dimension() = "objects";
  balanceSpec.includeInInitialAverage() =
      std::vector<std::string>{"dc-to-drain"}; // Include even though draining
  balanceSpec.fixAverageToInitial() = true;
  solver.addGoal(std::move(balanceSpec), 1.0);
  // balance_datacenter_drain_end
}

void combining_with_capacity() {
  /**
   * Common pattern: balance while respecting capacity constraints.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0"}},
          {"host1", {"task1"}},
      });

  // Set up dimensions and capacities
  std::map<std::string, double> object_memory = {
      {"task0", 4.0}, {"task1", 8.0}};
  std::map<std::string, double> container_capacity = {
      {"host0", 32.0}, {"host1", 32.0}};
  solver.addObjectDimension("memory", object_memory);
  solver.addContainerDimension("memory_capacity", container_capacity);

  // combining_capacity_start
  // Constraint: don't exceed capacity
  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  solver.addConstraint(capacitySpec);

  // Goal: balance memory usage
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "memory";
  solver.addGoal(balanceSpec, 1.0);
  // combining_capacity_end
}

void complete_example() {
  /**
   * Complete runnable example demonstrating BalanceSpec.
   *
   * This example shows how to use BalanceSpec to balance CPU and memory
   * across hosts while respecting capacity constraints.
   */
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "balance-example", "production");

  solver.setObjectName("task");
  solver.setContainerName("host");

  // Initial imbalanced assignment
  std::map<std::string, std::vector<std::string>> assignment = {
      {"host0", {"task0", "task1", "task2", "task3", "task4"}},
      {"host1", {"task5"}},
      {"host2", {}},
      {"host3", {"task6", "task7", "task8", "task9"}},
  };
  solver.setAssignment(assignment);

  // Task resources
  std::map<std::string, double> cpu_usage;
  std::map<std::string, double> memory_usage;
  for (const auto i : folly::irange(10)) {
    cpu_usage[fmt::format("task{}", i)] = 1.0 + (i % 3);
    memory_usage[fmt::format("task{}", i)] = 2.0 + (i % 4);
  }

  solver.addObjectDimension("cpu", cpu_usage);
  solver.addObjectDimension("memory", memory_usage);

  // Host capacities
  std::map<std::string, double> host_cpu_capacity;
  std::map<std::string, double> host_memory_capacity;
  for (const auto i : folly::irange(4)) {
    host_cpu_capacity[fmt::format("host{}", i)] = 20.0;
    host_memory_capacity[fmt::format("host{}", i)] = 40.0;
  }

  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);
  solver.addContainerDimension("memory_capacity", host_memory_capacity);

  // Add capacity constraints
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu";
  solver.addConstraint(cpuCapacity);

  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory";
  solver.addConstraint(memoryCapacity);

  // Add balance goals
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;
  solver.addGoal(balanceCpu, 1.0);

  BalanceSpec balanceMemory;
  balanceMemory.name() = "balance-memory";
  balanceMemory.scope() = "host";
  balanceMemory.dimension() = "memory";
  balanceMemory.formula() = BalanceSpecFormula::LEGACY;
  balanceMemory.fixAverageToInitial() = true;
  solver.addGoal(balanceMemory, 1.0);

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
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all Balance examples...\n";

  std::cout << "\n1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Basic Load Balancing...\n";
  basic_load_balancing();

  std::cout << "\n3. Balance With Fixed Average...\n";
  balance_with_fixed_average();

  std::cout << "\n4. Multi Level Balancing...\n";
  multi_level_balancing();

  std::cout << "\n5. Balance With Upper Bound...\n";
  balance_with_upper_bound();

  std::cout << "\n6. Balance With Datacenter Drain...\n";
  balance_with_datacenter_drain();

  std::cout << "\n7. Combining With Capacity...\n";
  combining_with_capacity();

  std::cout << "\n8. Complete Example...\n";
  complete_example();

  std::cout << "\n✓ All Balance examples completed successfully!\n";
  return 0;
}
// example_end
