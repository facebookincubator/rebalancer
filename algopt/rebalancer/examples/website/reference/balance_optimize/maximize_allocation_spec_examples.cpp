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
 * MaximizeAllocationSpec Reference Examples
 *
 * This file demonstrates all the usage patterns for MaximizeAllocationSpec
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
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void quick_example() {
  /**
   * Quick example showing basic MaximizeAllocationSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"budget_host_1", {"task0", "task1"}},
          {"budget_host_2", {"task2"}},
          {"budget_host_3", {}},
          {"other_host_1", {"task3", "task4"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 3.0},
          {"task2", 1.0},
          {"task3", 4.0},
          {"task4", 2.0},
      });

  // quick_example_start
  // Prefer using budget hosts (fill them first)
  std::vector<std::string> budget_hosts = {
      "budget_host_1", "budget_host_2", "budget_host_3"};

  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "prefer-budget-hosts";
  maximizeSpec.scope() = "host";
  maximizeSpec.dimension() = "cpu_usage";
  Filter filter;
  filter.itemsWhitelist() = budget_hosts;
  maximizeSpec.filter() = filter;
  solver.addGoal(maximizeSpec, 2.0);
  // quick_example_end
}

void prefer_cheap_hosts() {
  /**
   * Fill budget-tier hosts before using premium hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"budget_host_1", {"task0", "task1"}},
          {"budget_host_2", {"task2"}},
          {"budget_host_3", {}},
          {"premium_host_1", {"task3"}},
          {"premium_host_2", {"task4"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 3.0},
          {"task2", 1.0},
          {"task3", 4.0},
          {"task4", 2.0},
      });

  // prefer_cheap_hosts_start
  // Define cost tiers
  std::vector<std::string> budget_hosts = {
      "budget_host_1", "budget_host_2", "budget_host_3"};
  std::vector<std::string> premium_hosts = {"premium_host_1", "premium_host_2"};

  // Maximize allocation on budget hosts (cost savings)
  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "prefer-cheap-hosts";
  maximizeSpec.scope() = "host";
  maximizeSpec.dimension() = "cpu_usage"; // Or "count" for object count
  Filter filter;
  filter.itemsWhitelist() = std::move(budget_hosts);
  maximizeSpec.filter() = std::move(filter);
  solver.addGoal(
      std::move(maximizeSpec), 3.0); // Higher weight = stronger preference

  // Still balance load overall (secondary)
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-all-hosts";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(balanceSpec), 1.0); // Lower weight
  // prefer_cheap_hosts_end
}

void data_locality() {
  /**
   * Maximize data on hosts with local storage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"ssd_host_1", {"task0", "task1"}},
          {"ssd_host_2", {"task2"}},
          {"ssd_host_3", {}},
          {"hdd_host_1", {"task3", "task4"}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"task0", 10.0},
          {"task1", 20.0},
          {"task2", 15.0},
          {"task3", 25.0},
          {"task4", 10.0},
      });

  // data_locality_start
  // Hosts with local SSDs
  std::vector<std::string> local_storage_hosts = {
      "ssd_host_1", "ssd_host_2", "ssd_host_3"};

  // Prefer placing data on local storage hosts
  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "prefer-local-storage";
  maximizeSpec.scope() = "host";
  maximizeSpec.dimension() = "data_size";
  Filter filter;
  filter.itemsWhitelist() = std::move(local_storage_hosts);
  maximizeSpec.filter() = std::move(filter);
  solver.addGoal(std::move(maximizeSpec), 2.0);
  // data_locality_end
}

void rack_consolidation() {
  /**
   * Consolidate workload onto specific racks.
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
          {"host3", {"task4"}},
          {"host4", {"task5"}},
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

  // Add rack scope
  std::map<std::string, std::string> host_to_rack = {
      {"host0", "rack1"},
      {"host1", "rack1"},
      {"host2", "rack2"},
      {"host3", "rack2"},
      {"host4", "rack3"},
  };
  solver.addScope("rack", host_to_rack);

  // rack_consolidation_start
  // Target racks for consolidation (others will be freed)
  std::vector<std::string> active_racks = {"rack1", "rack2", "rack3"};

  // Maximize usage on active racks
  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "consolidate-racks";
  maximizeSpec.scope() = "rack";
  maximizeSpec.dimension() = "cpu_usage";
  Filter filter;
  filter.itemsWhitelist() = std::move(active_racks);
  maximizeSpec.filter() = std::move(filter);
  solver.addGoal(std::move(maximizeSpec), 5.0);

  // This implicitly drains other racks
  // rack_consolidation_end
}

void new_infrastructure_preference() {
  /**
   * Prefer new hosts over old hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"new_dc_host_1", {"task0"}},
          {"new_dc_host_2", {"task1"}},
          {"new_dc_host_3", {}},
          {"old_dc_host_1", {"task2", "task3", "task4"}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"task0", 1.0},
          {"task1", 1.0},
          {"task2", 1.0},
          {"task3", 1.0},
          {"task4", 1.0},
      });

  // new_infrastructure_preference_start
  // New datacenter hosts
  std::vector<std::string> new_dc_hosts = {
      "new_dc_host_1", "new_dc_host_2", "new_dc_host_3"};

  // Maximize allocation on new hosts (migration incentive)
  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "prefer-new-infrastructure";
  maximizeSpec.scope() = "host";
  maximizeSpec.dimension() = "count"; // Object count
  Filter filter;
  filter.itemsWhitelist() = std::move(new_dc_hosts);
  maximizeSpec.filter() = std::move(filter);
  solver.addGoal(std::move(maximizeSpec), 2.5);
  // new_infrastructure_preference_end
}

void zone_preference() {
  /**
   * Prefer specific availability zones.
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
          {"host3", {"task4"}},
          {"host4", {"task5"}},
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

  // Add zone scope
  std::map<std::string, std::string> host_to_zone = {
      {"host0", "zone_a"},
      {"host1", "zone_a"},
      {"host2", "zone_b"},
      {"host3", "zone_b"},
      {"host4", "zone_c"},
  };
  solver.addScope("zone", host_to_zone);

  // zone_preference_start
  // Preferred zones (lower latency, cheaper network)
  std::vector<std::string> preferred_zones = {"zone_a", "zone_b"};

  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "prefer-zones";
  maximizeSpec.scope() = "zone";
  maximizeSpec.dimension() = "network_traffic"; // Network-intensive workloads
  Filter filter;
  filter.itemsWhitelist() = std::move(preferred_zones);
  maximizeSpec.filter() = std::move(filter);
  solver.addGoal(std::move(maximizeSpec), 1.5);
  // zone_preference_end
}

void energy_efficiency() {
  /**
   * Pack onto energy-efficient hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"efficient_host_1", {"task0"}},
          {"efficient_host_2", {"task1"}},
          {"standard_host_1", {"task2", "task3", "task4"}},
      });
  solver.addObjectDimension(
      "power_usage",
      std::map<std::string, double>{
          {"task0", 50.0},
          {"task1", 60.0},
          {"task2", 40.0},
          {"task3", 55.0},
          {"task4", 45.0},
      });

  // energy_efficiency_start
  // Modern, energy-efficient hosts
  std::vector<std::string> efficient_hosts = {
      "efficient_host_1", "efficient_host_2"};

  // Maximize usage on efficient hosts
  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "energy-efficiency";
  maximizeSpec.scope() = "host";
  maximizeSpec.dimension() = "power_usage"; // Or cpu_usage as proxy
  Filter filter;
  filter.itemsWhitelist() = std::move(efficient_hosts);
  maximizeSpec.filter() = std::move(filter);
  solver.addGoal(std::move(maximizeSpec), 2.0);
  // energy_efficiency_end
}

void tiered_storage_strategy() {
  /**
   * Fill fast storage before slow storage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"nvme_host_1", {"task0"}},
          {"nvme_host_2", {"task1"}},
          {"hdd_host_1", {"task2", "task3", "task4"}},
      });
  solver.addObjectDimension(
      "hot_data_size",
      std::map<std::string, double>{
          {"task0", 100.0},
          {"task1", 200.0},
          {"task2", 50.0},
          {"task3", 150.0},
          {"task4", 75.0},
      });

  // tiered_storage_strategy_start
  // Hosts with NVMe storage
  std::vector<std::string> nvme_hosts = {"nvme_host_1", "nvme_host_2"};

  // Maximize allocation on NVMe hosts for hot data
  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "prefer-nvme";
  maximizeSpec.scope() = "host";
  maximizeSpec.dimension() = "hot_data_size"; // Hot data dimension
  Filter filter;
  filter.itemsWhitelist() = std::move(nvme_hosts);
  maximizeSpec.filter() = std::move(filter);
  solver.addGoal(std::move(maximizeSpec), 4.0);
  // tiered_storage_strategy_end
}

void combining_with_minimize_containers() {
  /**
   * Consolidate onto preferred hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2"}},
          {"host2", {"task3", "task4"}},
      });
  solver.addObjectDimension(
      "count",
      std::map<std::string, double>{
          {"task0", 1.0},
          {"task1", 1.0},
          {"task2", 1.0},
          {"task3", 1.0},
          {"task4", 1.0},
      });

  std::vector<std::string> preferred_hosts = {"host0", "host1"};

  // combining_minimize_containers_start
  // Maximize allocation on preferred hosts
  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "prefer-hosts";
  maximizeSpec.scope() = "host";
  maximizeSpec.dimension() = "count";
  Filter filter;
  filter.itemsWhitelist() = std::move(preferred_hosts);
  maximizeSpec.filter() = std::move(filter);
  solver.addGoal(std::move(maximizeSpec), 3.0);

  // Also minimize total hosts used
  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-hosts";
  minimizeSpec.scope() = "host";
  minimizeSpec.dimension() = "count";
  solver.addGoal(std::move(minimizeSpec), 2.0);
  // combining_minimize_containers_end
}

void combining_with_balance() {
  /**
   * Fill preferred hosts, balance the rest.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2"}},
          {"host2", {"task3", "task4"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 3.0},
          {"task2", 1.0},
          {"task3", 4.0},
          {"task4", 2.0},
      });

  std::vector<std::string> budget_hosts = {"host0", "host1"};

  // combining_balance_start
  // Primary: Fill budget hosts
  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "fill-budget";
  maximizeSpec.scope() = "host";
  maximizeSpec.dimension() = "cpu_usage";
  Filter filter;
  filter.itemsWhitelist() = budget_hosts;
  maximizeSpec.filter() = filter;
  solver.addGoal(maximizeSpec, 4.0);

  // Secondary: Balance across all hosts
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-all";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu_usage";
  solver.addGoal(balanceSpec, 1.0);
  // combining_balance_end
}

void combining_with_capacity() {
  /**
   * Maximize within capacity limits.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2"}},
          {"host2", {"task3", "task4"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 3.0},
          {"task2", 1.0},
          {"task3", 4.0},
          {"task4", 2.0},
      });
  solver.addContainerDimension(
      "cpu_capacity",
      std::map<std::string, double>{
          {"host0", 15.0},
          {"host1", 15.0},
          {"host2", 15.0},
      });

  std::vector<std::string> preferred_hosts = {"host0", "host1"};

  // combining_capacity_start
  // Hard limit: Capacity
  CapacitySpec capacitySpec;
  capacitySpec.name() = "capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "cpu_usage";
  solver.addConstraint(std::move(capacitySpec));

  // Soft goal: Maximize on preferred hosts (within capacity)
  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "maximize-preferred";
  maximizeSpec.scope() = "host";
  maximizeSpec.dimension() = "cpu_usage";
  Filter filter;
  filter.itemsWhitelist() = std::move(preferred_hosts);
  maximizeSpec.filter() = std::move(filter);
  solver.addGoal(std::move(maximizeSpec), 3.0);
  // combining_capacity_end
}

void complete_example() {
  /**
   * Complete runnable example demonstrating MaximizeAllocationSpec.
   *
   * This example shows how to use MaximizeAllocationSpec to preferentially
   * fill budget hosts before using premium hosts, resulting in cost savings.
   */
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "maximize-allocation-example", "production");

  solver.setObjectName("task");
  solver.setContainerName("host");

  // Initial assignment: mixed across budget and premium hosts
  std::map<std::string, std::vector<std::string>> assignment = {
      {"budget_host_0", {"task0", "task1"}},
      {"budget_host_1", {"task2"}},
      {"budget_host_2", {}},
      {"premium_host_0", {"task3", "task4", "task5"}},
      {"premium_host_1", {"task6", "task7", "task8", "task9"}},
  };
  solver.setAssignment(assignment);

  // Task resources
  std::map<std::string, double> cpu_usage;
  for (const auto i : folly::irange(10)) {
    cpu_usage[fmt::format("task{}", i)] = 2.0 + (i % 3);
  }
  solver.addObjectDimension("cpu_usage", cpu_usage);

  // Host capacities
  std::map<std::string, double> budget_capacity = {
      {"budget_host_0", 15.0},
      {"budget_host_1", 15.0},
      {"budget_host_2", 15.0}};
  std::map<std::string, double> premium_capacity = {
      {"premium_host_0", 15.0}, {"premium_host_1", 15.0}};
  std::map<std::string, double> all_capacity = budget_capacity;
  all_capacity.insert(premium_capacity.begin(), premium_capacity.end());
  solver.addContainerDimension("cpu_capacity", all_capacity);

  // Add capacity constraint
  CapacitySpec capacitySpec;
  capacitySpec.name() = "cpu-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "cpu_usage";
  solver.addConstraint(capacitySpec);

  // Primary goal: Maximize allocation on budget hosts
  std::vector<std::string> budget_hosts = {
      "budget_host_0", "budget_host_1", "budget_host_2"};
  MaximizeAllocationSpec maximizeSpec;
  maximizeSpec.name() = "prefer-budget-hosts";
  maximizeSpec.scope() = "host";
  maximizeSpec.dimension() = "cpu_usage";
  Filter filter;
  filter.itemsWhitelist() = budget_hosts;
  maximizeSpec.filter() = filter;
  solver.addGoal(maximizeSpec, 3.0);

  // Secondary goal: Balance overall (low weight)
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-all";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu_usage";
  solver.addGoal(balanceSpec, 1.0);

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

  // Calculate allocation on budget vs premium
  // assignment() is object -> container (F14FastMap<string, string>)
  double budget_cpu = 0.0;
  double premium_cpu = 0.0;
  for (const auto& [task, host] : *solution.assignment()) {
    const double cpu = cpu_usage[task];
    if (std::find(budget_hosts.begin(), budget_hosts.end(), host) !=
        budget_hosts.end()) {
      budget_cpu += cpu;
    } else {
      premium_cpu += cpu;
    }
  }

  double total_cpu = budget_cpu + premium_cpu;
  std::cout << "\nAllocation breakdown:\n";
  std::cout << fmt::format(
      "  Budget hosts: {:.1f} CPU ({:.1f}%)\n",
      budget_cpu,
      budget_cpu / total_cpu * 100);
  std::cout << fmt::format(
      "  Premium hosts: {:.1f} CPU ({:.1f}%)\n",
      premium_cpu,
      premium_cpu / total_cpu * 100);
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all MaximizeAllocation examples...\n";

  std::cout << "\n1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Prefer Cheap Hosts...\n";
  prefer_cheap_hosts();

  std::cout << "\n3. Data Locality...\n";
  data_locality();

  std::cout << "\n4. Rack Consolidation...\n";
  rack_consolidation();

  std::cout << "\n5. New Infrastructure Preference...\n";
  new_infrastructure_preference();

  std::cout << "\n6. Zone Preference...\n";
  zone_preference();

  std::cout << "\n7. Energy Efficiency...\n";
  energy_efficiency();

  std::cout << "\n8. Tiered Storage Strategy...\n";
  tiered_storage_strategy();

  std::cout << "\n9. Combining With Minimize Containers...\n";
  combining_with_minimize_containers();

  std::cout << "\n10. Combining With Balance...\n";
  combining_with_balance();

  std::cout << "\n11. Combining With Capacity...\n";
  combining_with_capacity();

  std::cout << "\n12. Complete Example...\n";
  complete_example();

  std::cout << "\n✓ All MaximizeAllocation examples completed successfully!\n";
  return 0;
}
// example_end
