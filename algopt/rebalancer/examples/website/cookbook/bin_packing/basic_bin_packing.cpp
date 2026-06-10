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
 * Basic Bin Packing and Consolidation
 *
 * This example demonstrates how to pack VMs onto the minimum number of hosts
 * to save costs while respecting capacity constraints and maintaining balance.
 *
 * Problem: 100 virtual machines running across 30 hosts. Many hosts are lightly
 * loaded.
 *
 * Goal: Consolidate VMs onto fewest hosts possible to free up hosts for
 * decommissioning or maintenance.
 */

// solution_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "vm-consolidator", "production");

  // Define objects and containers
  solver.setObjectName("vm");
  solver.setContainerName("host");

  // Initial spread assignment (inefficient - VMs spread across all hosts)
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(30)) {
    assignment[fmt::format("host{}", i)] = {};
  }

  // Distribute VMs round-robin (spread thinly)
  for (const auto i : folly::irange(100)) {
    int host_idx = i % 30;
    assignment[fmt::format("host{}", host_idx)].push_back(
        fmt::format("vm{}", i));
  }
  solver.setAssignment(assignment);

  // VM CPU usage (varies 0.5 to 8.0 cores)
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0.0, 1.0);

  std::map<std::string, double> vm_cpu;
  for (const auto i : folly::irange(100)) {
    vm_cpu[fmt::format("vm{}", i)] = 0.5 + dis(gen) * 7.5;
  }
  solver.addObjectDimension("cpu_usage", vm_cpu);

  // VM memory usage (varies 1 to 32 GB)
  std::map<std::string, double> vm_memory;
  for (const auto i : folly::irange(100)) {
    vm_memory[fmt::format("vm{}", i)] = 1.0 + dis(gen) * 31.0;
  }
  solver.addObjectDimension("memory_usage", vm_memory);

  // Add count dimension for MinimizeContainers
  std::map<std::string, double> vm_count;
  for (const auto i : folly::irange(100)) {
    vm_count[fmt::format("vm{}", i)] = 1.0;
  }
  solver.addObjectDimension("count", vm_count);

  // Host capacities (uniform for simplicity)
  std::map<std::string, double> host_cpu_capacity;
  std::map<std::string, double> host_memory_capacity;
  for (const auto i : folly::irange(30)) {
    host_cpu_capacity[fmt::format("host{}", i)] = 64.0; // 64 cores
    host_memory_capacity[fmt::format("host{}", i)] = 256.0; // 256 GB
  }
  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);
  solver.addContainerDimension("memory_capacity", host_memory_capacity);

  // CONSTRAINT 1: CPU capacity
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu_usage";
  solver.addConstraint(cpuCapacity);

  // CONSTRAINT 2: Memory capacity
  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory_usage";
  solver.addConstraint(memoryCapacity);

  // GOAL 1: Minimize number of hosts used (PRIMARY)
  MinimizeContainersSpec minimizeHosts;
  minimizeHosts.name() = "minimize-hosts";
  minimizeHosts.scope() = "host";
  minimizeHosts.dimension() = "count";
  solver.addGoal(minimizeHosts, 10.0); // High weight - main goal

  // GOAL 2: Balance CPU across used hosts (SECONDARY)
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;
  solver.addGoal(balanceCpu, 1.0);

  // GOAL 3: Balance memory across used hosts (SECONDARY)
  BalanceSpec balanceMemory;
  balanceMemory.name() = "balance-memory";
  balanceMemory.scope() = "host";
  balanceMemory.dimension() = "memory_usage";
  balanceMemory.formula() = BalanceSpecFormula::LEGACY;
  balanceMemory.fixAverageToInitial() = true;
  solver.addGoal(balanceMemory, 1.0);

  // Use Local Search solver
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 60000;
  solver.addSolver(localSearch);

  // Solve
  auto solution = solver.solve();

  // Analyze solution
  std::cout << "AssignmentSolution found in " << "<solve time>" << "ms\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "VMs moved: " << "<move count>" << "\n";

  // Count hosts used
  int initial_hosts_used = 0;
  int final_hosts_used = 0;

  // Build host -> VMs map from solution (solution.assignment() is object ->
  // container)
  std::set<std::string> hosts_with_vms;
  for (const auto& [vm, host] : *solution.assignment()) {
    hosts_with_vms.insert(host);
  }
  final_hosts_used = hosts_with_vms.size();

  for (const auto& [host, vms] : assignment) {
    if (!vms.empty()) {
      initial_hosts_used++;
    }
  }

  std::cout << "\nConsolidation results:\n";
  std::cout << "  Initial hosts used: " << initial_hosts_used << "\n";
  std::cout << "  Final hosts used: " << final_hosts_used << "\n";
  std::cout << "  Hosts freed: " << (initial_hosts_used - final_hosts_used)
            << "\n";
  std::cout << fmt::format(
      "  Reduction: {:.1f}%\n",
      ((initial_hosts_used - final_hosts_used) * 100.0 / initial_hosts_used));

  return 0;
}
// solution_end
