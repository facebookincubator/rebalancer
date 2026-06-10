// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/*
 * Gradual Datacenter Migration Example
 *
 * Demonstrates how to migrate workloads gradually from an old datacenter to a
 * new datacenter over multiple rounds to minimize risk and disruption. This
 * example shows:
 * - Multi-round migration strategy
 * - Pinning already-migrated workloads to prevent ping-pong
 * - Protecting critical workloads in early rounds
 * - Gradually freezing the old datacenter
 * - Balancing load during migration
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

std::map<std::string, std::vector<std::string>> initializeOldDatacenter(
    int workload_count,
    int hosts_per_dc) {
  std::map<std::string, std::vector<std::string>> assignment;

  // DC-OLD hosts
  for (const auto i : folly::irange(hosts_per_dc)) {
    assignment[fmt::format("old_dc_host_{}", i)] = {};
  }

  // DC-NEW hosts (empty initially)
  for (const auto i : folly::irange(hosts_per_dc)) {
    assignment[fmt::format("new_dc_host_{}", i)] = {};
  }

  // Distribute workloads across old DC
  for (const auto i : folly::irange(workload_count)) {
    int host_idx = i % hosts_per_dc;
    assignment[fmt::format("old_dc_host_{}", host_idx)].push_back(
        fmt::format("workload_{}", i));
  }

  return assignment;
}

AssignmentSolution solveMigrationRound(
    const std::map<std::string, std::vector<std::string>>& current_assignment,
    int round_num,
    int total_rounds,
    int hosts_per_dc) {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(
      executor, fmt::format("dc-migration-round-{}", round_num), "production");

  solver.setObjectName("workload");
  solver.setContainerName("host");
  solver.setAssignment(current_assignment);

  // Define datacenter scope
  std::map<std::string, std::string> host_to_dc;
  for (const auto i : folly::irange(hosts_per_dc)) {
    host_to_dc[fmt::format("old_dc_host_{}", i)] = "dc_old";
    host_to_dc[fmt::format("new_dc_host_{}", i)] = "dc_new";
  }
  solver.addScope("datacenter", host_to_dc);

  // Workload resource usage
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> cpu_dist(2.0, 6.0);
  std::uniform_real_distribution<> mem_dist(4.0, 12.0);

  std::map<std::string, double> workload_cpu, workload_mem;
  for (const auto i : folly::irange(200)) {
    std::string workload = fmt::format("workload_{}", i);
    workload_cpu[workload] = cpu_dist(gen);
    workload_mem[workload] = mem_dist(gen);
  }

  solver.addObjectDimension("cpu_usage", workload_cpu);
  solver.addObjectDimension("memory_usage", workload_mem);

  // Host capacities
  std::map<std::string, double> host_cpu_capacity, host_mem_capacity;
  for (const auto i : folly::irange(hosts_per_dc)) {
    host_cpu_capacity[fmt::format("old_dc_host_{}", i)] = 20.0;
    host_mem_capacity[fmt::format("old_dc_host_{}", i)] = 32.0;
    host_cpu_capacity[fmt::format("new_dc_host_{}", i)] = 32.0;
    host_mem_capacity[fmt::format("new_dc_host_{}", i)] = 64.0;
  }

  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);
  solver.addContainerDimension("memory_capacity", host_mem_capacity);

  // CONSTRAINT: Capacity limits
  CapacitySpec cpuCap, memCap;
  cpuCap.name() = "cpu-capacity";
  cpuCap.scope() = "host";
  cpuCap.dimension() = "cpu_usage";
  solver.addConstraint(cpuCap);

  memCap.name() = "memory-capacity";
  memCap.scope() = "host";
  memCap.dimension() = "memory_usage";
  solver.addConstraint(memCap);

  // Pin workloads already migrated to new DC
  std::vector<std::string> already_migrated;
  for (const auto& [host, workloads] : current_assignment) {
    if (host.find("new_dc_") == 0) {
      already_migrated.insert(
          already_migrated.end(), workloads.begin(), workloads.end());
    }
  }

  if (!already_migrated.empty()) {
    AvoidMovingSpec pinMigrated;
    pinMigrated.name() = "pin-already-migrated";
    pinMigrated.objects() = already_migrated;
    solver.addConstraint(pinMigrated);
    std::cout << "  Pinned " << already_migrated.size()
              << " already-migrated workloads\n";
  }

  // In early rounds, keep critical workloads stable
  if (round_num < total_rounds - 2) {
    std::vector<std::string> critical_workloads;
    critical_workloads.reserve(20);
    for (const auto i : folly::irange(20)) {
      critical_workloads.push_back(fmt::format("workload_{}", i));
    }

    AvoidMovingSpec pinCritical;
    pinCritical.name() = "pin-critical-early";
    pinCritical.objects() = critical_workloads;
    solver.addConstraint(pinCritical);
    std::cout << "  Pinned " << critical_workloads.size()
              << " critical workloads (early round)\n";
  }

  // In later rounds, freeze old DC
  if (round_num >= total_rounds / 2) {
    std::vector<std::string> old_dc_hosts;
    old_dc_hosts.reserve(hosts_per_dc);
    for (const auto i : folly::irange(hosts_per_dc)) {
      old_dc_hosts.push_back(fmt::format("old_dc_host_{}", i));
    }

    NonAcceptingSpec freezeOld;
    freezeOld.name() = "freeze-old-dc";
    freezeOld.scope() = "host";
    freezeOld.items() = old_dc_hosts;
    solver.addConstraint(freezeOld);
    std::cout << "  Old DC frozen (no new placements)\n";
  }

  // GOAL: Balance within each datacenter
  BalanceSpec balanceCpu, balanceMem;
  balanceCpu.name() = "balance-cpu-per-dc";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;
  solver.addGoal(balanceCpu, 2.0);

  balanceMem.name() = "balance-memory-per-dc";
  balanceMem.scope() = "host";
  balanceMem.dimension() = "memory_usage";
  balanceMem.formula() = BalanceSpecFormula::LEGACY;
  balanceMem.fixAverageToInitial() = true;
  solver.addGoal(balanceMem, 2.0);

  // GOAL: Minimize movement (higher weight in later rounds)
  double movement_weight =
      0.5 + (static_cast<double>(round_num) / total_rounds) * 2.0;
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-movement";
  minMove.scope() = "host";
  minMove.dimension() = "cpu_usage";
  solver.addGoal(minMove, movement_weight);

  // Solve
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 30000;
  solver.addSolver(localSearch);

  auto solution = solver.solve();

  std::cout << "  AssignmentSolution found in " << "<solve time>"
            << "ms\n";
  std::cout << "  Objective value: " << *solution.finalObjective()->value()
            << "\n";

  return solution;
}

void gradualDatacenterMigration() {
  const int TOTAL_ROUNDS = 5;
  const int WORKLOAD_COUNT = 200;
  const int HOSTS_PER_DC = 30;

  // Initial state: All workloads in DC-OLD
  auto current_assignment =
      initializeOldDatacenter(WORKLOAD_COUNT, HOSTS_PER_DC);

  std::cout << "Starting gradual datacenter migration (" << TOTAL_ROUNDS
            << " rounds)\n";
  std::cout << std::string(80, '=') << "\n";

  for (const auto round_num : folly::irange(TOTAL_ROUNDS)) {
    std::cout << "\nROUND " << (round_num + 1) << "/" << TOTAL_ROUNDS << "\n";
    std::cout << std::string(80, '=') << "\n";

    int target_migrated_pct = ((round_num + 1) * 100) / TOTAL_ROUNDS;
    int target_migrated_count = (target_migrated_pct * WORKLOAD_COUNT) / 100;

    std::cout << "Target: " << target_migrated_count << " workloads in DC-NEW ("
              << target_migrated_pct << "%)\n";

    auto solution = solveMigrationRound(
        current_assignment, round_num, TOTAL_ROUNDS, HOSTS_PER_DC);

    // Convert object->container to container->objects
    current_assignment.clear();
    // Ensure all hosts are in the assignment (even if empty)
    for (const auto i : folly::irange(HOSTS_PER_DC)) {
      current_assignment[fmt::format("old_dc_host_{}", i)] = {};
      current_assignment[fmt::format("new_dc_host_{}", i)] = {};
    }
    for (const auto& [obj, container] : *solution.assignment()) {
      current_assignment[container].push_back(obj);
    }

    std::cout << "Round " << (round_num + 1) << " complete\n";
  }

  std::cout << "\nGradual migration complete!\n";
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  gradualDatacenterMigration();
  return 0;
}
// solution_end
