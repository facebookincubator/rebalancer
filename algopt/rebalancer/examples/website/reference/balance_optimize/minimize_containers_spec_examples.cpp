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
 * MinimizeContainersSpec Reference Examples
 *
 * This file demonstrates all the usage patterns for MinimizeContainersSpec
 * shown in the reference documentation. Each function is a complete, runnable
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

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void quick_example() {
  /**
   * Quick example showing basic MinimizeContainersSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"vm0", "vm1"}},
          {"host1", {"vm2"}},
          {"host2", {"vm3", "vm4"}},
          {"host3", {}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"vm0", 1.0},
          {"vm1", 1.0},
          {"vm2", 1.0},
          {"vm3", 1.0},
          {"vm4", 1.0},
      });

  // quick_example_start
  // Consolidate VMs onto fewest hosts
  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-hosts";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count"; // Count dimension
  solver.addGoal(minimizeSpec, 10.0); // High weight - primary goal
  // quick_example_end
}

void basic_vm_consolidation() {
  /**
   * Basic VM consolidation - pack VMs onto fewest hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  // Create initial sparse assignment across many hosts
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(10)) {
    std::vector<std::string> vms;
    vms.reserve(10);
    for (const auto j : folly::irange(10)) {
      vms.push_back(fmt::format("vm{}", i * 10 + j));
    }
    assignment[fmt::format("host{}", i)] = vms;
  }
  solver.setAssignment(assignment);

  // basic_vm_consolidation_start
  // Add count dimension (each VM = 1.0)
  std::map<std::string, double> vm_count;
  for (const auto i : folly::irange(100)) {
    vm_count[fmt::format("vm{}", i)] = 1.0;
  }
  solver.addObjectDimension("count", std::move(vm_count));

  // Minimize hosts used
  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-hosts";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  solver.addGoal(std::move(minimizeSpec), 10.0);
  // basic_vm_consolidation_end
}

void consolidation_with_capacity() {
  /**
   * Consolidation with capacity constraints - ensure limits respected.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"vm0", "vm1"}},
          {"host1", {"vm2"}},
          {"host2", {"vm3", "vm4"}},
          {"host3", {}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"vm0", 1.0},
          {"vm1", 1.0},
          {"vm2", 1.0},
          {"vm3", 1.0},
          {"vm4", 1.0},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"vm0", 2.0},
          {"vm1", 3.0},
          {"vm2", 1.0},
          {"vm3", 4.0},
          {"vm4", 2.0},
      });
  solver.addObjectDimension(
      "memory_usage",
      std::map<std::string, double>{
          {"vm0", 4.0},
          {"vm1", 8.0},
          {"vm2", 2.0},
          {"vm3", 6.0},
          {"vm4", 4.0},
      });
  solver.addContainerDimension(
      "cpu_capacity",
      std::map<std::string, double>{
          {"host0", 20.0},
          {"host1", 20.0},
          {"host2", 20.0},
          {"host3", 20.0},
      });
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{
          {"host0", 40.0},
          {"host1", 40.0},
          {"host2", 40.0},
          {"host3", 40.0},
      });

  // consolidation_capacity_start
  // Minimize hosts
  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-hosts";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  solver.addGoal(std::move(minimizeSpec), 10.0);

  // Respect CPU capacity
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu_usage";
  solver.addConstraint(std::move(cpuCapacity));

  // Respect memory capacity
  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory_usage";
  solver.addConstraint(std::move(memoryCapacity));
  // consolidation_capacity_end
}

void weighted_container_costs() {
  /**
   * Weighted container costs - prefer freeing expensive containers.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_host_1", {"vm0", "vm1"}},
          {"old_host_2", {"vm2"}},
          {"premium_host_1", {"vm3"}},
          {"standard_host_1", {"vm4", "vm5"}},
          {"standard_host_2", {"vm6"}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"vm0", 1.0},
          {"vm1", 1.0},
          {"vm2", 1.0},
          {"vm3", 1.0},
          {"vm4", 1.0},
          {"vm5", 1.0},
          {"vm6", 1.0},
      });

  // weighted_costs_start
  // Assign costs to containers (higher = prefer to free)
  std::map<std::string, double> container_costs = {
      {"old_host_1", 100.0}, // Old hardware, want to decommission
      {"old_host_2", 100.0},
      {"premium_host_1", 50.0}, // Expensive, free if possible
      {"standard_host_1", 1.0}, // Normal cost
      {"standard_host_2", 1.0},
  };

  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-cost-weighted";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  minimizeSpec.containerCosts() = std::move(container_costs);
  solver.addGoal(std::move(minimizeSpec), 10.0);
  // weighted_costs_end
}

void safety_limit_on_freeing() {
  /**
   * Safety limit - prevent freeing too many containers at once.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"vm0", "vm1"}},
          {"host1", {"vm2"}},
          {"host2", {"vm3", "vm4"}},
          {"host3", {}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"vm0", 1.0},
          {"vm1", 1.0},
          {"vm2", 1.0},
          {"vm3", 1.0},
          {"vm4", 1.0},
      });

  // safety_limit_start
  // Free at most 10 hosts per round (safety)
  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "gradual-consolidation";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  minimizeSpec.maxFreeLimit() = 10; // Max 10 hosts freed
  solver.addGoal(std::move(minimizeSpec), 10.0);
  // safety_limit_end
}

void rack_level_consolidation() {
  /**
   * Rack-level consolidation - consolidate entire racks.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"vm0", "vm1"}},
          {"host1", {"vm2"}},
          {"host2", {"vm3", "vm4"}},
          {"host3", {}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"vm0", 1.0},
          {"vm1", 1.0},
          {"vm2", 1.0},
          {"vm3", 1.0},
          {"vm4", 1.0},
      });

  // Add rack scope
  std::map<std::string, std::string> host_to_rack = {
      {"host0", "rack0"},
      {"host1", "rack0"},
      {"host2", "rack1"},
      {"host3", "rack1"},
  };
  solver.addScope("rack", host_to_rack);

  // rack_consolidation_start
  // Minimize racks used
  MinimizeContainersSpec minimizeRacks;
  minimizeRacks.name() = "minimize-racks";
  minimizeRacks.scope() = "rack";
  minimizeRacks.dimension() = "count";
  solver.addGoal(std::move(minimizeRacks), 8.0);

  // Also minimize hosts within used racks
  MinimizeContainersSpec minimizeHosts;
  minimizeHosts.name() = "minimize-hosts";
  minimizeHosts.scope() = "host";
  minimizeHosts.dimension() = "count";
  solver.addGoal(
      std::move(minimizeHosts), 5.0); // Lower weight than rack consolidation
  // rack_consolidation_end
}

void consolidation_with_balance() {
  /**
   * Consolidation with balance - minimize containers while balancing load.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"vm0", "vm1"}},
          {"host1", {"vm2"}},
          {"host2", {"vm3", "vm4"}},
          {"host3", {}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"vm0", 1.0},
          {"vm1", 1.0},
          {"vm2", 1.0},
          {"vm3", 1.0},
          {"vm4", 1.0},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"vm0", 2.0},
          {"vm1", 3.0},
          {"vm2", 1.0},
          {"vm3", 4.0},
          {"vm4", 2.0},
      });

  // consolidation_balance_start
  // Primary: Minimize hosts
  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-hosts";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  solver.addGoal(std::move(minimizeSpec), 10.0);

  // Secondary: Balance load on used hosts
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-remaining";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(balanceSpec), 2.0); // Lower weight
  // consolidation_balance_end
}

void combining_with_to_free() {
  /**
   * Combining with ToFreeSpec - targeted + general consolidation.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_host_1", {"vm0", "vm1"}},
          {"old_host_2", {"vm2"}},
          {"host0", {"vm3", "vm4"}},
          {"host1", {"vm5"}},
          {"host2", {}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"vm0", 1.0},
          {"vm1", 1.0},
          {"vm2", 1.0},
          {"vm3", 1.0},
          {"vm4", 1.0},
          {"vm5", 1.0},
      });

  // combining_to_free_start
  // Explicitly drain specific hosts (constraint)
  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "drain-targets";
  toFreeSpec.containers() =
      std::vector<std::string>{"old_host_1", "old_host_2"};
  solver.addConstraint(std::move(toFreeSpec));

  // Also minimize total hosts used (goal)
  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-all-hosts";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  solver.addGoal(std::move(minimizeSpec), 5.0);
  // combining_to_free_end
}

void consolidation_with_balance_fix() {
  /**
   * Fixing overloaded containers - consolidate AND balance.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"vm0", "vm1"}},
          {"host1", {"vm2"}},
          {"host2", {"vm3", "vm4"}},
          {"host3", {}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"vm0", 1.0},
          {"vm1", 1.0},
          {"vm2", 1.0},
          {"vm3", 1.0},
          {"vm4", 1.0},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"vm0", 2.0},
          {"vm1", 3.0},
          {"vm2", 1.0},
          {"vm3", 4.0},
          {"vm4", 2.0},
      });

  // consolidation_balance_fix_start
  // GOOD: Consolidate AND balance
  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-hosts";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  solver.addGoal(std::move(minimizeSpec), 10.0);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu_usage";
  solver.addGoal(
      std::move(balanceSpec), 3.0); // Prevents overloading packed containers
  // consolidation_balance_fix_end
}

void count_dimension_setup() {
  /**
   * Proper count dimension setup.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  // Create initial sparse assignment across many hosts
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(10)) {
    std::vector<std::string> vms;
    vms.reserve(10);
    for (const auto j : folly::irange(10)) {
      vms.push_back(fmt::format("vm{}", i * 10 + j));
    }
    assignment[fmt::format("host{}", i)] = vms;
  }
  solver.setAssignment(assignment);

  std::vector<std::string> all_objects;
  all_objects.reserve(100);
  for (const auto i : folly::irange(100)) {
    all_objects.push_back(fmt::format("vm{}", i));
  }

  // count_dimension_start
  // GOOD: Add count dimension first
  std::map<std::string, double> count_dimension;
  for (const auto& obj : all_objects) {
    count_dimension[obj] = 1.0;
  }
  solver.addObjectDimension("count", std::move(count_dimension));

  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-hosts";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  solver.addGoal(std::move(minimizeSpec), 10.0);
  // count_dimension_end
}

void correct_formula_usage() {
  /**
   * Using the correct formula for bin packing.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("vm");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"vm0", "vm1"}},
          {"host1", {"vm2"}},
          {"host2", {"vm3", "vm4"}},
          {"host3", {}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"vm0", 1.0},
          {"vm1", 1.0},
          {"vm2", 1.0},
          {"vm3", 1.0},
          {"vm4", 1.0},
      });

  // correct_formula_start
  // GOOD: Correct formula (or omit for default)
  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-hosts";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  minimizeSpec.formula() = MinimizeContainerSpecFormula::LEGACY;
  solver.addGoal(std::move(minimizeSpec), 10.0);
  // correct_formula_end
}

void infeasibility_check() {
  /**
   * Check if consolidation is feasible before attempting.
   */
  // infeasibility_check_start
  // Example VM resource requirements
  std::map<std::string, double> vm_cpu;
  std::map<std::string, double> vm_memory;
  for (const auto i : folly::irange(100)) {
    vm_cpu[fmt::format("vm{}", i)] = 2.0 + (i % 3);
    vm_memory[fmt::format("vm{}", i)] = 4.0 + (i % 4);
  }

  // Calculate total resources needed
  double total_cpu_needed = 0.0;
  double total_memory_needed = 0.0;
  for (const auto& [vm, cpu] : vm_cpu) {
    total_cpu_needed += cpu;
  }
  for (const auto& [vm, memory] : vm_memory) {
    total_memory_needed += memory;
  }

  // Target consolidation
  int target_hosts = 10;
  double host_cpu_capacity = 20.0;
  double host_memory_capacity = 40.0;
  double target_cpu_capacity = target_hosts * host_cpu_capacity;
  double target_memory_capacity = target_hosts * host_memory_capacity;

  // Check feasibility
  if (total_cpu_needed > target_cpu_capacity ||
      total_memory_needed > target_memory_capacity) {
    std::cout << fmt::format(
        "Warning: Cannot consolidate to {} hosts!\n", target_hosts);
    std::cout << fmt::format(
        "  Need CPU: {}, Available: {}\n",
        total_cpu_needed,
        target_cpu_capacity);
    std::cout << fmt::format(
        "  Need Memory: {}, Available: {}\n",
        total_memory_needed,
        target_memory_capacity);
  }
  // infeasibility_check_end
}

void verify_consolidation() {
  /**
   * Verify consolidation achieved target.
   */
  // verify_consolidation_start
  // Lambda to verify consolidation target
  [[maybe_unused]] auto verify_consolidation_target =
      [](const AssignmentSolution& solution, int target_max_containers) {
        // Count occupied containers
        // assignment() is object -> container (F14FastMap<string, string>)
        std::set<std::string> occupied_containers;
        for (const auto& [object, container] : *solution.assignment()) {
          occupied_containers.insert(container);
        }
        int occupied_count = occupied_containers.size();

        if (occupied_count <= target_max_containers) {
          std::cout << fmt::format(
              "Consolidation success: {} containers used "
              "(target: <={})\n",
              occupied_count,
              target_max_containers);
          return true;
        } else {
          std::cout << fmt::format(
              "Consolidation missed target: {} containers used "
              "(target: <={})\n",
              occupied_count,
              target_max_containers);
          return false;
        }
      };

  // Example usage (with a mock solution - in practice use actual solver result)
  // verify_consolidation_target(solution, 10);
  // verify_consolidation_end
}

void calculate_savings() {
  /**
   * Calculate cost savings from consolidation.
   */
  // calculate_savings_start
  // Calculate consolidation savings
  int initial_hosts = 50;
  int final_hosts = 30;
  const double cost_per_host_per_month = 1000.0;
  int hosts_freed = initial_hosts - final_hosts;
  double monthly_savings = hosts_freed * cost_per_host_per_month;
  double annual_savings = monthly_savings * 12;

  std::cout << "Cost savings analysis:\n";
  std::cout << fmt::format("  Initial hosts: {}\n", initial_hosts);
  std::cout << fmt::format("  Final hosts: {}\n", final_hosts);
  std::cout << fmt::format("  Hosts freed: {}\n", hosts_freed);
  std::cout << fmt::format("  Monthly savings: ${:.2f}\n", monthly_savings);
  std::cout << fmt::format("  Annual savings: ${:.2f}\n", annual_savings);
  // calculate_savings_end
}

void complete_example() {
  /**
   * Complete runnable example demonstrating MinimizeContainersSpec.
   *
   * This example shows how to use MinimizeContainersSpec to consolidate VMs
   * onto fewer hosts while respecting capacity constraints and balancing load.
   */
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "consolidation-example", "production");

  solver.setObjectName("vm");
  solver.setContainerName("host");

  // Initial sparse assignment (VMs spread across many hosts)
  std::map<std::string, std::vector<std::string>> assignment = {
      {"host0", {"vm0", "vm1"}},
      {"host1", {"vm2"}},
      {"host2", {"vm3", "vm4"}},
      {"host3", {"vm5"}},
      {"host4", {}},
      {"host5", {"vm6", "vm7"}},
      {"host6", {"vm8"}},
      {"host7", {}},
      {"host8", {"vm9"}},
      {"host9", {}},
  };
  solver.setAssignment(assignment);

  // VM resources
  std::map<std::string, double> vm_cpu;
  std::map<std::string, double> vm_memory;
  std::map<std::string, double> vm_count;
  for (const auto i : folly::irange(10)) {
    vm_cpu[fmt::format("vm{}", i)] = 2.0 + (i % 3);
    vm_memory[fmt::format("vm{}", i)] = 4.0 + (i % 4);
    vm_count[fmt::format("vm{}", i)] = 1.0;
  }

  solver.addObjectDimension("cpu", vm_cpu);
  solver.addObjectDimension("memory", vm_memory);
  solver.addObjectDimension("count", vm_count);

  // Host capacities
  std::map<std::string, double> host_cpu_capacity;
  std::map<std::string, double> host_memory_capacity;
  for (const auto i : folly::irange(10)) {
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

  // Primary goal: Minimize hosts used
  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-hosts";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  solver.addGoal(minimizeSpec, 10.0);

  // Secondary goal: Balance CPU on remaining hosts
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(balanceSpec, 2.0);

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

  // Count occupied hosts
  // assignment() is object -> container (F14FastMap<string, string>)
  std::set<std::string> occupied_hosts;
  for (const auto& [vm, host] : *solution.assignment()) {
    occupied_hosts.insert(host);
  }
  int occupied = occupied_hosts.size();
  std::cout << fmt::format("Hosts used: {} (reduced from 6)\n", occupied);
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all MinimizeContainers examples...\n";

  std::cout << "\n1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Basic Vm Consolidation...\n";
  basic_vm_consolidation();

  std::cout << "\n3. Consolidation With Capacity...\n";
  consolidation_with_capacity();

  std::cout << "\n4. Weighted Container Costs...\n";
  weighted_container_costs();

  std::cout << "\n5. Safety Limit On Freeing...\n";
  safety_limit_on_freeing();

  std::cout << "\n6. Rack Level Consolidation...\n";
  rack_level_consolidation();

  std::cout << "\n7. Consolidation With Balance...\n";
  consolidation_with_balance();

  std::cout << "\n8. Combining With To Free...\n";
  combining_with_to_free();

  std::cout << "\n9. Consolidation With Balance Fix...\n";
  consolidation_with_balance_fix();

  std::cout << "\n10. Count Dimension Setup...\n";
  count_dimension_setup();

  std::cout << "\n11. Correct Formula Usage...\n";
  correct_formula_usage();

  std::cout << "\n12. Infeasibility Check...\n";
  infeasibility_check();

  std::cout << "\n13. Verify Consolidation...\n";
  verify_consolidation();

  std::cout << "\n14. Calculate Savings...\n";
  calculate_savings();

  std::cout << "\n15. Complete Example...\n";
  complete_example();

  std::cout << "\n✓ All MinimizeContainers examples completed successfully!\n";
  return 0;
}
// example_end
