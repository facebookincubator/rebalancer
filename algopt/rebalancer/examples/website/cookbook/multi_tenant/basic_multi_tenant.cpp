// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Multi-Tenant Fair Sharing Example
 *
 * This example demonstrates how to allocate resources fairly across multiple
 * tenants with different priorities and quotas. It shows:
 * - Partitioning workloads by tenant
 * - Premium tenant isolation (max 1 per server)
 * - CPU and memory capacity constraints
 * - Balanced resource allocation within tenant allocations
 * - Minimizing workload movement during rebalancing
 *
 * Problem: 5 tenants, 20 servers, 200 workloads total
 * Goal: Fair allocation while respecting quotas and isolation
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
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

namespace {
struct TenantConfig {
  std::string tier;
  int cpu_quota;
  int mem_quota;
  int priority;
  int workload_count;
};
} // namespace

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "multi-tenant-allocator", "production");

  solver.setObjectName("workload");
  solver.setContainerName("server");

  // Tenant configuration
  std::map<std::string, TenantConfig> tenants = {
      {"premium_tenant_a",
       {.tier = "premium",
        .cpu_quota = 100,
        .mem_quota = 500,
        .priority = 3,
        .workload_count = 40}},
      {"premium_tenant_b",
       {.tier = "premium",
        .cpu_quota = 80,
        .mem_quota = 400,
        .priority = 3,
        .workload_count = 30}},
      {"standard_tenant_c",
       {.tier = "standard",
        .cpu_quota = 60,
        .mem_quota = 300,
        .priority = 2,
        .workload_count = 50}},
      {"standard_tenant_d",
       {.tier = "standard",
        .cpu_quota = 50,
        .mem_quota = 250,
        .priority = 2,
        .workload_count = 40}},
      {"basic_tenant_e",
       {.tier = "basic",
        .cpu_quota = 30,
        .mem_quota = 150,
        .priority = 1,
        .workload_count = 40}},
  };

  // Initial imbalanced assignment
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(20)) {
    assignment[fmt::format("server{}", i)] = {};
  }

  // Create workloads and assign initially (imbalanced)
  int workload_id = 0;
  for (const auto& [tenant, config] : tenants) {
    for (const auto i : folly::irange(config.workload_count)) {
      std::string workload_name = fmt::format("{}_workload_{}", tenant, i);
      int server_idx = workload_id % 5; // Concentrate on first 5 servers
      assignment[fmt::format("server{}", server_idx)].push_back(workload_name);
      workload_id++;
    }
  }
  solver.setAssignment(assignment);

  // Define tenant partition
  std::map<std::string, std::vector<std::string>> tenant_partition;
  for (const auto& [tenant, config] : tenants) {
    for (const auto i : folly::irange(config.workload_count)) {
      tenant_partition[tenant].push_back(
          fmt::format("{}_workload_{}", tenant, i));
    }
  }
  solver.addPartition("tenant", tenant_partition);

  // Workload CPU and memory usage
  std::map<std::string, double> cpu_usage, memory_usage;
  for (const auto& [tenant, config] : tenants) {
    double avg_cpu =
        static_cast<double>(config.cpu_quota) / config.workload_count;
    double avg_mem =
        static_cast<double>(config.mem_quota) / config.workload_count;

    for (const auto i : folly::irange(config.workload_count)) {
      std::string workload_name = fmt::format("{}_workload_{}", tenant, i);
      cpu_usage[workload_name] = avg_cpu * (0.8 + 0.4 * (i % 5) / 5.0);
      memory_usage[workload_name] = avg_mem * (0.8 + 0.4 * (i % 5) / 5.0);
    }
  }
  solver.addObjectDimension("cpu_usage", cpu_usage);
  solver.addObjectDimension("memory_usage", memory_usage);

  // Server capacities
  std::map<std::string, double> server_cpu_capacity, server_mem_capacity;
  for (const auto i : folly::irange(20)) {
    server_cpu_capacity[fmt::format("server{}", i)] = 25.0;
    server_mem_capacity[fmt::format("server{}", i)] = 128.0;
  }
  solver.addContainerDimension("cpu_capacity", server_cpu_capacity);
  solver.addContainerDimension("memory_capacity", server_mem_capacity);

  // CONSTRAINT 1: Server CPU capacity
  CapacitySpec cpuCap;
  cpuCap.name() = "server-cpu-capacity";
  cpuCap.scope() = "server";
  cpuCap.dimension() = "cpu_usage";
  solver.addConstraint(cpuCap);

  // CONSTRAINT 2: Server memory capacity
  CapacitySpec memCap;
  memCap.name() = "server-memory-capacity";
  memCap.scope() = "server";
  memCap.dimension() = "memory_usage";
  solver.addConstraint(memCap);

  // CONSTRAINT 3: Premium tenant isolation
  std::vector<std::string> premium_tenants;
  premium_tenants.reserve(tenants.size());
  for (const auto& [tenant, config] : tenants) {
    if (config.tier == "premium") {
      premium_tenants.push_back(tenant);
    }
  }

  if (!premium_tenants.empty()) {
    GroupCountSpec premiumIsolation;
    premiumIsolation.name() = "premium-tenant-isolation";
    premiumIsolation.scope() = "server";
    premiumIsolation.partitionName() = "tenant";
    premiumIsolation.bound() = GroupCountSpecBound::MAX;
    premiumIsolation.limit() = Limit();
    premiumIsolation.limit()->type() = LimitType::ABSOLUTE;
    premiumIsolation.limit()->globalLimit() = 1;
    solver.addConstraint(premiumIsolation);
  }

  // GOAL 1: Balance CPU
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "server";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;
  solver.addGoal(balanceCpu, 1.0);

  // GOAL 2: Balance memory
  BalanceSpec balanceMem;
  balanceMem.name() = "balance-memory";
  balanceMem.scope() = "server";
  balanceMem.dimension() = "memory_usage";
  balanceMem.formula() = BalanceSpecFormula::LEGACY;
  balanceMem.fixAverageToInitial() = true;
  solver.addGoal(balanceMem, 1.0);

  // GOAL 3: Minimize movement
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-movement";
  minMove.scope() = "server";
  minMove.dimension() = "cpu_usage";
  solver.addGoal(minMove, 0.2);

  // Use Local Search solver
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 60000;
  solver.addSolver(localSearch);

  // Solve
  auto solution = solver.solve();

  // Print results
  std::cout << "AssignmentSolution found in " << "<solve time>" << "ms\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "Workloads moved: " << "<move count>" << "\n";

  // Print per-tenant allocation
  std::cout << "\nPer-tenant resource allocation:\n";
  std::cout << fmt::format(
      "{:<25} {:<10} {:<12} {:<12} {:<12} {:<12} {}\n",
      "Tenant",
      "Tier",
      "Quota CPU",
      "Actual CPU",
      "Quota Mem",
      "Actual Mem",
      "Servers");
  std::cout << std::string(110, '-') << "\n";

  for (const auto& [tenant, config] : tenants) {
    const auto& tenant_workloads = tenant_partition[tenant];
    double tenant_cpu = 0.0, tenant_mem = 0.0;

    for (const auto& w : tenant_workloads) {
      tenant_cpu += cpu_usage[w];
      tenant_mem += memory_usage[w];
    }

    // Count servers with this tenant
    std::set<std::string> servers_with_tenant;
    for (const auto& [workload, server] : *solution.assignment()) {
      if (std::find(
              tenant_workloads.begin(), tenant_workloads.end(), workload) !=
          tenant_workloads.end()) {
        servers_with_tenant.insert(server);
      }
    }

    std::cout << fmt::format(
        "{:<25} {:<10} {:<12.1f} {:<12.1f} {:<12.1f} {:<12.1f} {}\n",
        tenant,
        config.tier,
        static_cast<double>(config.cpu_quota),
        tenant_cpu,
        static_cast<double>(config.mem_quota),
        tenant_mem,
        servers_with_tenant.size());
  }

  return 0;
}
// solution_end
