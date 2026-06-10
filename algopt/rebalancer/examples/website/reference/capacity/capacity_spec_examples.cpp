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
 * CapacitySpec Reference Examples
 *
 * This file demonstrates all the usage patterns for CapacitySpec shown in the
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
   * Quick example showing basic CapacitySpec usage as constraint and goal.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 4.0}, {"task1", 4.0}, {"task2", 4.0}});
  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 2.0}, {"task2", 2.0}});

  // quick_example_start
  // As a constraint (hard): memory must not exceed capacity
  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory";
  Limit memLimit;
  memLimit.globalLimit() = 64.0; // Max 64GB per host
  memoryCapacity.limit() = memLimit;
  solver.addConstraint(memoryCapacity);

  // As a goal (soft): prefer to stay under CPU capacity
  CapacitySpec cpuSoftLimit;
  cpuSoftLimit.name() = "cpu-soft-limit";
  cpuSoftLimit.scope() = "host";
  cpuSoftLimit.dimension() = "cpu";
  Limit cpuLimit;
  cpuLimit.globalLimit() = 16.0; // Prefer max 16 cores
  cpuSoftLimit.limit() = cpuLimit;
  solver.addGoal(cpuSoftLimit, 10.0);
  // quick_example_end
}

void global_limit_example() {
  /**
   * Global limit - same limit for all scope items.
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
  solver.addObjectDimension(
      "memory", std::map<std::string, double>{{"task0", 4.0}, {"task1", 4.0}});

  // global_limit_start
  // All hosts have 64GB capacity
  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 64.0;
  memoryCapacity.limit() = std::move(limit);
  solver.addConstraint(std::move(memoryCapacity));
  // global_limit_end
}

void per_scope_item_limits_example() {
  /**
   * Per-scope-item limits - different limits for different scope items.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0"}},
          {"host1", {"task1"}},
          {"host2", {"task2"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 4.0}, {"task1", 4.0}, {"task2", 4.0}});

  // per_scope_item_limits_start
  // Different hosts have different capacities
  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory";
  Limit limit;
  limit.scopeItemLimits() = {
      {"host0", 32.0}, {"host1", 64.0}, {"host2", 128.0}};
  memoryCapacity.limit() = std::move(limit);
  solver.addConstraint(std::move(memoryCapacity));
  // per_scope_item_limits_end
}

void relative_limit_example() {
  /**
   * Relative limits - limit as a fraction of a capacity dimension.
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
  solver.addObjectDimension(
      "memory", std::map<std::string, double>{{"task0", 4.0}, {"task1", 4.0}});
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{{"host0", 64.0}, {"host1", 64.0}});

  // relative_limit_example_start
  // Use 80% of available capacity
  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 0.8;
  limit.type() = LimitType::RELATIVE;
  memoryCapacity.limit() = std::move(limit);
  solver.addConstraint(std::move(memoryCapacity));
  // relative_limit_example_end
}

void minimum_utilization_example() {
  /**
   * Minimum utilization - ensure minimum utilization using MIN bound.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2"}},
      });
  solver.addObjectDimension(
      "object_count",
      std::map<std::string, double>{
          {"task0", 1.0}, {"task1", 1.0}, {"task2", 1.0}});

  // minimum_utilization_start
  // Ensure at least 10 objects per host (minimize containers)
  CapacitySpec minObjectsPerHost;
  minObjectsPerHost.name() = "min-objects-per-host";
  minObjectsPerHost.scope() = "host";
  minObjectsPerHost.dimension() = "object_count";
  Limit limit;
  limit.globalLimit() = 10.0;
  minObjectsPerHost.limit() = std::move(limit);
  minObjectsPerHost.bound() = CapacitySpecBound::MIN;
  solver.addGoal(std::move(minObjectsPerHost), 1.0);
  // minimum_utilization_end
}

void automatic_capacity_matching_example() {
  /**
   * Automatic capacity matching with relative limits.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0"}},
          {"host1", {"task1"}},
          {"host2", {"task2"}},
      });

  // Setup
  std::map<std::string, double> memory_per_object = {
      {"task0", 2.0}, {"task1", 4.0}, {"task2", 3.0}};
  std::map<std::string, double> capacity_per_host = {
      {"host0", 64.0}, {"host1", 64.0}, {"host2", 128.0}};

  // automatic_capacity_matching_start
  // Define object utilization
  solver.addObjectDimension("memory", std::move(memory_per_object));

  // Define container capacity
  solver.addContainerDimension("memory_capacity", capacity_per_host);

  // Capacity spec with relative limit
  CapacitySpec memoryLimit;
  memoryLimit.name() = "memory-limit";
  memoryLimit.scope() = "host";
  memoryLimit.dimension() = "memory"; // Utilization
  Limit limit;
  limit.globalLimit() = 0.9; // 90% of capacity
  limit.type() = LimitType::RELATIVE;
  memoryLimit.limit() = std::move(limit);
  solver.addConstraint(std::move(memoryLimit));

  // Rebalancer computes: memory / memory_capacity <= 0.9 for each host
  // automatic_capacity_matching_end
}

void basic_capacity_constraint_example() {
  /**
   * Basic capacity constraint - don't exceed host memory.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0"}},
          {"host1", {"task1"}},
          {"host2", {"task2"}},
      });

  // Setup
  std::map<std::string, double> memory_per_task = {
      {"task0", 2.0}, {"task1", 4.0}, {"task2", 3.0}};
  std::map<std::string, double> memory_per_host = {
      {"host0", 64.0}, {"host1", 64.0}, {"host2", 128.0}};

  // basic_capacity_constraint_start
  solver.addObjectDimension("memory", std::move(memory_per_task));
  solver.addContainerDimension("memory_capacity", memory_per_host);

  CapacitySpec memoryLimit;
  memoryLimit.name() = "memory-limit";
  memoryLimit.scope() = "host";
  memoryLimit.dimension() = "memory";
  solver.addConstraint(std::move(memoryLimit));
  // basic_capacity_constraint_end
}

void multi_resource_capacity_example() {
  /**
   * Multi-resource capacity - constrain both CPU and memory.
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
  solver.addObjectDimension(
      "cpu", std::map<std::string, double>{{"task0", 2.0}, {"task1", 2.0}});
  solver.addObjectDimension(
      "memory", std::map<std::string, double>{{"task0", 4.0}, {"task1", 4.0}});

  // multi_resource_capacity_start
  // CPU constraint
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu";
  Limit cpuLimit;
  cpuLimit.globalLimit() = 16.0;
  cpuCapacity.limit() = std::move(cpuLimit);
  solver.addConstraint(std::move(cpuCapacity));

  // Memory constraint
  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory";
  Limit memLimit;
  memLimit.globalLimit() = 64.0;
  memoryCapacity.limit() = std::move(memLimit);
  solver.addConstraint(std::move(memoryCapacity));
  // multi_resource_capacity_end
}

void hierarchical_capacity_example() {
  /**
   * Hierarchical capacity - apply capacity at multiple levels.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0"}},
          {"host1", {"task1"}},
          {"host2", {"task2"}},
          {"host3", {"task3"}},
      });
  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 2.0}, {"task2", 2.0}, {"task3", 2.0}});

  // Add rack scope
  std::map<std::string, std::string> host_to_rack = {
      {"host0", "rack0"},
      {"host1", "rack0"},
      {"host2", "rack1"},
      {"host3", "rack1"}};
  solver.addScope("rack", host_to_rack);

  // hierarchical_capacity_start
  // Host-level capacity (fine-grained)
  CapacitySpec hostCpu;
  hostCpu.name() = "host-cpu";
  hostCpu.scope() = "host";
  hostCpu.dimension() = "cpu";
  Limit hostLimit;
  hostLimit.globalLimit() = 16.0;
  hostCpu.limit() = std::move(hostLimit);
  solver.addConstraint(std::move(hostCpu));

  // Rack-level capacity (coarse-grained)
  CapacitySpec rackCpu;
  rackCpu.name() = "rack-cpu";
  rackCpu.scope() = "rack";
  rackCpu.dimension() = "cpu";
  Limit rackLimit;
  rackLimit.globalLimit() = 128.0; // 8 hosts × 16 cores
  rackCpu.limit() = std::move(rackLimit);
  solver.addConstraint(std::move(rackCpu));
  // hierarchical_capacity_end
}

void soft_capacity_oversubscription_example() {
  /**
   * Soft capacity (oversubscription) - allow temporary capacity violations.
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
  solver.addObjectDimension(
      "memory", std::map<std::string, double>{{"task0", 4.0}, {"task1", 4.0}});

  // soft_capacity_oversubscription_start
  // Hard constraint: never exceed 200% of capacity
  CapacitySpec hardLimit;
  hardLimit.name() = "hard-limit";
  hardLimit.scope() = "host";
  hardLimit.dimension() = "memory";
  Limit hardLimitValue;
  hardLimitValue.globalLimit() = 128.0; // 2× capacity
  hardLimit.limit() = std::move(hardLimitValue);
  solver.addConstraint(std::move(hardLimit));

  // Soft goal: prefer to stay under 100% capacity
  CapacitySpec softLimit;
  softLimit.name() = "soft-limit";
  softLimit.scope() = "host";
  softLimit.dimension() = "memory";
  Limit softLimitValue;
  softLimitValue.globalLimit() = 64.0; // 1× capacity
  softLimit.limit() = std::move(softLimitValue);
  solver.addGoal(std::move(softLimit), 10.0);
  // soft_capacity_oversubscription_end
}

void reserve_capacity_example() {
  /**
   * Reserve capacity - keep some capacity free.
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
  solver.addObjectDimension(
      "cpu", std::map<std::string, double>{{"task0", 2.0}, {"task1", 2.0}});
  solver.addContainerDimension(
      "cpu_capacity",
      std::map<std::string, double>{{"host0", 16.0}, {"host1", 16.0}});

  // reserve_capacity_start
  // Use at most 80% of capacity (reserve 20%)
  CapacitySpec reservedCapacity;
  reservedCapacity.name() = "reserved-capacity";
  reservedCapacity.scope() = "host";
  reservedCapacity.dimension() = "cpu";
  Limit limit;
  limit.globalLimit() = 0.8;
  limit.type() = LimitType::RELATIVE;
  reservedCapacity.limit() = std::move(limit);
  solver.addConstraint(std::move(reservedCapacity));
  // reserve_capacity_end
}

void group_utilization_bounds_example() {
  /**
   * Advanced: Group utilization bounds - limit utilization from specific
   * groups.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 4.0}, {"task1", 4.0}, {"task2", 4.0}, {"task3", 4.0}});

  // Setup partition
  std::map<std::string, std::string> object_to_tenant = {
      {"task0", "tenant_a"},
      {"task1", "tenant_a"},
      {"task2", "tenant_b"},
      {"task3", "tenant_b"}};

  // group_utilization_bounds_start
  // Partition: group by tenant
  solver.addPartition("tenant", std::move(object_to_tenant));

  // Constraint: Each tenant can use at most 20GB per host
  CapacitySpec perTenantMemory;
  perTenantMemory.name() = "per-tenant-memory";
  perTenantMemory.scope() = "host";
  perTenantMemory.dimension() = "memory";

  GroupUtilizationBound groupBound;
  groupBound.partitionName() = "tenant";
  groupBound.boundType() = UtilizationBoundType::UPPER;
  Limit perGroupLimit;
  perGroupLimit.globalLimit() = 20.0;
  groupBound.perGroupValues() = std::move(perGroupLimit);

  UtilizationBound utilizationBound;
  utilizationBound.groupUtilizationBound() = groupBound;
  perTenantMemory.utilizationBound() = std::move(utilizationBound);

  solver.addConstraint(std::move(perTenantMemory));
  // group_utilization_bounds_end
}

void combining_capacity_and_balance_example() {
  /**
   * Combining capacity + balance - respect capacity while balancing load.
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
  solver.addObjectDimension(
      "cpu", std::map<std::string, double>{{"task0", 2.0}, {"task1", 2.0}});

  // combining_capacity_balance_start
  // Constraint: capacity
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu";
  Limit limit;
  limit.globalLimit() = 16.0;
  cpuCapacity.limit() = std::move(limit);
  solver.addConstraint(std::move(cpuCapacity));

  // Goal: balance
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu";
  solver.addGoal(std::move(balanceCpu), 1.0);
  // combining_capacity_balance_end
}

void combining_capacity_and_minimize_movement_example() {
  /**
   * Combining capacity + minimize movement - respect capacity with minimal
   * disruption.
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
  solver.addObjectDimension(
      "memory", std::map<std::string, double>{{"task0", 4.0}, {"task1", 4.0}});

  // combining_capacity_minimize_movement_start
  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 64.0;
  memoryCapacity.limit() = std::move(limit);
  solver.addConstraint(std::move(memoryCapacity));

  MinimizeMovementSpec minimizeMoves;
  minimizeMoves.name() = "minimize-moves";
  solver.addGoal(std::move(minimizeMoves), 1.0);
  // combining_capacity_minimize_movement_end
}

void troubleshooting_capacity_dimension_example() {
  /**
   * Troubleshooting: Ensure container dimension exists for capacity.
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

  // Setup
  std::map<std::string, double> capacity_per_host = {
      {"host0", 64.0}, {"host1", 64.0}};

  // troubleshooting_capacity_dimension_start
  solver.addContainerDimension("cpu_capacity", capacity_per_host);

  // Or use dimension name that matches
  solver.addContainerDimension("cpu", capacity_per_host);
  // troubleshooting_capacity_dimension_end
}

void troubleshooting_relative_limit_example() {
  /**
   * Troubleshooting: Provide matching capacity dimension for relative limits.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  // Setup
  std::map<std::string, double> memory_per_task = {
      {"task0", 2.0}, {"task1", 4.0}};
  std::map<std::string, double> capacity_per_host = {
      {"host0", 64.0}, {"host1", 64.0}};

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0"}},
          {"host1", {"task1"}},
      });

  // troubleshooting_relative_limit_start
  // Utilization dimension (object)
  solver.addObjectDimension("memory", std::move(memory_per_task));

  // Capacity dimension (container) - must exist!
  solver.addContainerDimension("memory_capacity", capacity_per_host);

  // Now relative limit works
  CapacitySpec memoryLimit;
  memoryLimit.name() = "memory-limit";
  memoryLimit.scope() = "host";
  memoryLimit.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 0.8;
  limit.type() = LimitType::RELATIVE;
  memoryLimit.limit() = std::move(limit);
  solver.addConstraint(std::move(memoryLimit));
  // troubleshooting_relative_limit_end
}

void troubleshooting_zero_utilization_example() {
  /**
   * Troubleshooting: Allow zero utilization if empty containers are
   * acceptable.
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
  solver.addObjectDimension(
      "cpu", std::map<std::string, double>{{"task0", 2.0}, {"task1", 2.0}});

  // troubleshooting_zero_utilization_start
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu";
  Limit limit;
  limit.globalLimit() = 16.0;
  cpuCapacity.limit() = std::move(limit);
  cpuCapacity.zeroAllowed() = true;
  solver.addConstraint(std::move(cpuCapacity));
  // troubleshooting_zero_utilization_end
}

void complete_example() {
  /**
   * Complete runnable example demonstrating CapacitySpec.
   *
   * This example shows how to use CapacitySpec to enforce capacity constraints
   * while balancing CPU and memory across hosts.
   */
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "capacity-example", "production");

  solver.setObjectName("task");
  solver.setContainerName("host");

  // Initial assignment (some hosts overloaded)
  std::map<std::string, std::vector<std::string>> assignment;
  assignment["host0"] = {"task0", "task1", "task2", "task3", "task4"};
  assignment["host1"] = {"task5", "task6"};
  assignment["host2"] = {"task7"};
  assignment["host3"] = {};
  solver.setAssignment(assignment);

  // Task resources
  std::map<std::string, double> cpu_usage;
  std::map<std::string, double> memory_usage;
  for (const auto i : folly::irange(8)) {
    cpu_usage[fmt::format("task{}", i)] = 2.0 + (i % 3);
    memory_usage[fmt::format("task{}", i)] = 4.0 + (i % 4);
  }

  solver.addObjectDimension("cpu", cpu_usage);
  solver.addObjectDimension("memory", memory_usage);

  // Host capacities
  std::map<std::string, double> host_cpu_capacity;
  std::map<std::string, double> host_memory_capacity;
  for (const auto i : folly::irange(4)) {
    host_cpu_capacity[fmt::format("host{}", i)] = 16.0;
    host_memory_capacity[fmt::format("host{}", i)] = 32.0;
  }

  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);
  solver.addContainerDimension("memory_capacity", host_memory_capacity);

  // Add capacity constraints (hard limits)
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu";
  Limit cpuLimit;
  cpuLimit.globalLimit() = 16.0;
  cpuCapacity.limit() = cpuLimit;
  solver.addConstraint(cpuCapacity);

  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory";
  Limit memLimit;
  memLimit.globalLimit() = 32.0;
  memoryCapacity.limit() = memLimit;
  solver.addConstraint(memoryCapacity);

  // Add balance goals (soft objectives)
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu";
  solver.addGoal(balanceCpu, 1.0);

  BalanceSpec balanceMemory;
  balanceMemory.name() = "balance-memory";
  balanceMemory.scope() = "host";
  balanceMemory.dimension() = "memory";
  solver.addGoal(balanceMemory, 1.0);

  // Solve
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 10;
  solver.addSolver(localSearch);
  auto solution = solver.solve();

  // Print results
  std::cout << "AssignmentSolution found in "
            << *solution.problemProfile()->solveSec() << "ms\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all Capacity examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Global Limit Example...\n";
  global_limit_example();

  std::cout << "\n3. Per Scope Item Limits Example...\n";
  per_scope_item_limits_example();

  std::cout << "\n4. Relative Limit Example...\n";
  relative_limit_example();

  std::cout << "\n5. Minimum Utilization Example...\n";
  minimum_utilization_example();

  std::cout << "\n6. Automatic Capacity Matching Example...\n";
  automatic_capacity_matching_example();

  std::cout << "\n7. Basic Capacity Constraint Example...\n";
  basic_capacity_constraint_example();

  std::cout << "\n8. Multi Resource Capacity Example...\n";
  multi_resource_capacity_example();

  std::cout << "\n9. Hierarchical Capacity Example...\n";
  hierarchical_capacity_example();

  std::cout << "\n10. Soft Capacity Oversubscription Example...\n";
  soft_capacity_oversubscription_example();

  std::cout << "\n11. Reserve Capacity Example...\n";
  reserve_capacity_example();

  std::cout << "\n12. Group Utilization Bounds Example...\n";
  group_utilization_bounds_example();

  std::cout << "\n13. Combining Capacity And Balance Example...\n";
  combining_capacity_and_balance_example();

  std::cout << "\n14. Combining Capacity And Minimize Movement Example...\n";
  combining_capacity_and_minimize_movement_example();

  std::cout << "\n15. Troubleshooting Capacity Dimension Example...\n";
  troubleshooting_capacity_dimension_example();

  std::cout << "\n16. Troubleshooting Relative Limit Example...\n";
  troubleshooting_relative_limit_example();

  std::cout << "\n17. Troubleshooting Zero Utilization Example...\n";
  troubleshooting_zero_utilization_example();

  std::cout << "\n18. Complete Example...\n";
  complete_example();

  std::cout << "\n✓ All Capacity examples completed successfully!\n";
  return 0;
}
// example_end
