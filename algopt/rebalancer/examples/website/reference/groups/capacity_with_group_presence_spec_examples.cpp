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
 * CapacityWithGroupPresenceSpec Reference Examples

This file demonstrates all the usage patterns for CapacityWithGroupPresenceSpec
shown in the reference documentation. Each function is a complete, runnable
example.
 *
 * This file demonstrates all the usage patterns for
CapacityWithGroupPresenceSpec shown in the
 * reference documentation. Each function is a complete, runnable example.
 */

// example_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void quick_example() {
  /**
   * Quick example showing basic CapacityWithGroupPresenceSpec usage.
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
          {"task0", 10.0},
          {"task1", 20.0},
          {"task2", 15.0},
          {"task3", 25.0},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
      });

  // quick_example_start
  // Each service has 10 GB overhead when present, plus actual usage
  CapacityWithGroupPresenceSpec spec;
  spec.name() = "capacity-with-overhead";
  spec.scope() = "host";
  spec.partition() = "service";
  spec.dimension() = "memory";

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.globalLimit() =
      10.0; // Each service adds 10 GB when present
  spec.groupToPresenceWeight() = groupToPresenceWeight;

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  scopeItemToLimit.globalLimit() = 100.0; // Max 100 GB per host
  spec.scopeItemToLimit() = scopeItemToLimit;

  spec.bound() = CapacityWithGroupPresenceBound::MAX;

  solver.addConstraint(spec);
  // quick_example_end
}

void service_overhead_costs() {
  /**
   * Model fixed overhead per service.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"web_0", "web_1", "api_0"}},
          {"host1", {"web_2", "api_1", "api_2", "cache_0", "cache_1"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"web_0", 2.0},
          {"web_1", 3.0},
          {"web_2", 1.0},
          {"api_0", 4.0},
          {"api_1", 5.0},
          {"api_2", 3.0},
          {"cache_0", 2.0},
          {"cache_1", 1.0},
      });

  // service_overhead_costs_start
  // Each service has 5 GB metadata/connection overhead
  std::map<std::string, std::vector<std::string>> services = {
      {"web_service", {"web_0", "web_1", "web_2"}},
      {"api_service", {"api_0", "api_1", "api_2"}},
      {"cache_service", {"cache_0", "cache_1"}},
  };
  solver.addPartition("service", std::move(services));

  CapacityWithGroupPresenceSpec spec;
  spec.name() = "memory-with-service-overhead";
  spec.scope() = "host";
  spec.partition() = "service";
  spec.dimension() = "memory";

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.globalLimit() = 5.0; // 5 GB overhead per service
  spec.groupToPresenceWeight() = std::move(groupToPresenceWeight);

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  scopeItemToLimit.globalLimit() = 128.0; // 128 GB per host
  spec.scopeItemToLimit() = std::move(scopeItemToLimit);

  solver.addConstraint(std::move(spec));

  // Example:
  // Host has web_service (2 GB actual) + api_service (3 GB actual)
  // Contribution = max(5, 2) + max(5, 3) = 5 + 5 = 10 GB
  // Even though actual usage is only 2+3=5 GB, overhead makes it 10 GB
  // service_overhead_costs_end
}

void per_service_custom_overhead() {
  /**
   * Different overhead per service.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3", "task4", "task5"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 5.0},
          {"task1", 8.0},
          {"task2", 3.0},
          {"task3", 6.0},
          {"task4", 4.0},
          {"task5", 7.0},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"web_service", {"task0", "task1"}},
          {"api_service", {"task2", "task3"}},
          {"cache_service", {"task4", "task5"}},
      });

  // per_service_custom_overhead_start
  // Premium services have higher overhead
  CapacityWithGroupPresenceSpec spec;
  spec.name() = "variable-service-overhead";
  spec.scope() = "host";
  spec.partition() = "service";
  spec.dimension() = "memory";

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.groupLimits() = {
      {"web_service", 10.0}, // 10 GB overhead
      {"api_service", 15.0}, // 15 GB overhead
      {"cache_service", 5.0}, // 5 GB overhead
  };
  groupToPresenceWeight.globalLimit() = 8.0; // Default: 8 GB
  spec.groupToPresenceWeight() = std::move(groupToPresenceWeight);

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  scopeItemToLimit.globalLimit() = 128.0;
  spec.scopeItemToLimit() = std::move(scopeItemToLimit);

  solver.addConstraint(std::move(spec));
  // per_service_custom_overhead_end
}

void connection_pool_overhead() {
  /**
   * Model connection pool reserves.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("db_host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"db_host0", {"task0", "task1"}},
          {"db_host1", {"task2", "task3"}},
      });
  solver.addObjectDimension(
      "connection_count",
      std::map<std::string, double>{
          {"task0", 10.0},
          {"task1", 20.0},
          {"task2", 15.0},
          {"task3", 25.0},
      });
  solver.addPartition(
      "client_service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
      });

  // connection_pool_overhead_start
  // Databases reserve connections per client service
  CapacityWithGroupPresenceSpec spec;
  spec.name() = "db-connection-reserve";
  spec.scope() = "db_host";
  spec.partition() = "client_service";
  spec.dimension() = "connection_count";

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.globalLimit() =
      50.0; // Reserve 50 connections per service
  spec.groupToPresenceWeight() = std::move(groupToPresenceWeight);

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  scopeItemToLimit.globalLimit() = 1000.0; // Max 1000 connections per DB
  spec.scopeItemToLimit() = std::move(scopeItemToLimit);

  solver.addConstraint(std::move(spec));
  // connection_pool_overhead_end
}

void rounding_for_integer_resources() {
  /**
   * Round up group contributions to whole units.
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
      "cpu_cores",
      std::map<std::string, double>{
          {"task0", 0.5},
          {"task1", 2.3},
          {"task2", 1.0},
          {"task3", 0.1},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
      });

  // rounding_for_integer_resources_start
  // Each service rounds up to whole CPU cores
  CapacityWithGroupPresenceSpec spec;
  spec.name() = "cpu-rounded";
  spec.scope() = "host";
  spec.partition() = "service";
  spec.dimension() = "cpu_cores";

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.globalLimit() = 1.0; // Min 1 core when present
  spec.groupToPresenceWeight() = std::move(groupToPresenceWeight);

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  scopeItemToLimit.globalLimit() = 64.0; // 64 cores per host
  spec.scopeItemToLimit() = std::move(scopeItemToLimit);

  spec.roundUpGroupUtilOnScopeItem() = true; // Round up to integer cores

  solver.addConstraint(std::move(spec));

  // Example:
  // Service uses 2.3 cores → rounds up to 3 cores
  // Service uses 0.1 cores → max(1, 0.1) = 1 core (presence weight)
  // rounding_for_integer_resources_end
}

void multipliers_for_replica_overhead() {
  /**
   * Apply multipliers for replication.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("replica");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"replica0", "replica1"}},
          {"host1", {"replica2", "replica3"}},
          {"host2", {"replica4", "replica5"}},
          {"host3", {"replica6", "replica7"}},
      });
  solver.addObjectDimension(
      "storage",
      std::map<std::string, double>{
          {"replica0", 100.0},
          {"replica1", 200.0},
          {"replica2", 300.0},
          {"replica3", 150.0},
          {"replica4", 250.0},
          {"replica5", 500.0},
          {"replica6", 400.0},
          {"replica7", 350.0},
      });
  solver.addPartition(
      "shard",
      std::map<std::string, std::vector<std::string>>{
          {"shard_a", {"replica0", "replica1", "replica2", "replica3"}},
          {"shard_b", {"replica4", "replica5", "replica6", "replica7"}},
      });
  solver.addScope(
      "rack",
      std::map<std::string, std::string>{
          {"host0", "rack0"},
          {"host1", "rack0"},
          {"host2", "rack1"},
          {"host3", "rack1"},
      });

  // multipliers_for_replica_overhead_start
  // Primary + replica replication overhead (2x multiplier)
  CapacityWithGroupPresenceSpec spec;
  spec.name() = "replicated-capacity";
  spec.scope() = "rack";
  spec.partition() = "shard";
  spec.dimension() = "storage";

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.globalLimit() = 100.0; // 100 GB min per shard
  spec.groupToPresenceWeight() = std::move(groupToPresenceWeight);

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  scopeItemToLimit.globalLimit() = 5000.0; // 5 TB per rack
  spec.scopeItemToLimit() = std::move(scopeItemToLimit);

  Limit multiplier;
  multiplier.type() = LimitType::ABSOLUTE;
  multiplier.globalLimit() = 2.0; // 2x for replication
  spec.multiplierList() = {std::move(multiplier)};

  solver.addConstraint(std::move(spec));

  // Example:
  // Shard uses 500 GB → max(100, 500) = 500 GB
  // With 2x multiplier: ceil(500 * 2) = 1000 GB counted
  // multipliers_for_replica_overhead_end
}

void multi_level_multipliers() {
  /**
   * Cascading multipliers (e.g., replication + compression).
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
      "storage",
      std::map<std::string, double>{
          {"task0", 100.0},
          {"task1", 200.0},
          {"task2", 150.0},
          {"task3", 400.0},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
      });

  // multi_level_multipliers_start
  // 2x for replication, then 0.5x for compression
  CapacityWithGroupPresenceSpec spec;
  spec.name() = "multi-multiplier";
  spec.scope() = "host";
  spec.partition() = "service";
  spec.dimension() = "storage";

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.globalLimit() = 50.0;
  spec.groupToPresenceWeight() = std::move(groupToPresenceWeight);

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  scopeItemToLimit.globalLimit() = 1000.0;
  spec.scopeItemToLimit() = std::move(scopeItemToLimit);

  Limit m1;
  m1.type() = LimitType::ABSOLUTE;
  m1.globalLimit() = 2.0; // Replication
  Limit m2;
  m2.type() = LimitType::ABSOLUTE;
  m2.globalLimit() = 0.5; // Compression
  spec.multiplierList() = {std::move(m1), std::move(m2)};

  solver.addConstraint(std::move(spec));

  // Example:
  // Service uses 400 GB
  // After m1: ceil(400 * 2) = 800 GB
  // After m2: ceil(800 * 0.5) = 400 GB
  // multi_level_multipliers_end
}

void aggregation_scope() {
  /**
   * Aggregate utilization at different scope level.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
          {"host2", {"task4", "task5"}},
          {"host3", {"task6", "task7"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 10.0},
          {"task1", 20.0},
          {"task2", 15.0},
          {"task3", 25.0},
          {"task4", 12.0},
          {"task5", 18.0},
          {"task6", 22.0},
          {"task7", 14.0},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1", "task2", "task3"}},
          {"svc_b", {"task4", "task5", "task6", "task7"}},
      });
  solver.addScope(
      "rack",
      std::map<std::string, std::string>{
          {"host0", "rack0"},
          {"host1", "rack0"},
          {"host2", "rack1"},
          {"host3", "rack1"},
      });

  // aggregation_scope_start
  // Limit per rack, but aggregate at host level
  CapacityWithGroupPresenceSpec spec;
  spec.name() = "rack-limit-host-aggregation";
  spec.scope() = "rack";
  spec.partition() = "service";
  spec.dimension() = "memory";
  spec.aggregationScope() = "host"; // Aggregate at host level first

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.globalLimit() =
      10.0; // 10 GB overhead per service per host
  spec.groupToPresenceWeight() = std::move(groupToPresenceWeight);

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  scopeItemToLimit.globalLimit() = 500.0; // 500 GB per rack
  spec.scopeItemToLimit() = std::move(scopeItemToLimit);

  solver.addConstraint(std::move(spec));

  // Util(rack) = sum_{host in rack} sum_{service} max(10, service_util_on_host)
  // aggregation_scope_end
}

void per_group_and_scope_item_limits() {
  /**
   * Different limits per (group, scope item) combination.
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
      "cpu",
      std::map<std::string, double>{
          {"task0", 4.0},
          {"task1", 8.0},
          {"task2", 6.0},
          {"task3", 10.0},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
      });

  // per_group_and_scope_item_limits_start
  // Different service limits on different hosts
  CapacityWithGroupPresenceSpec spec;
  spec.name() = "per-service-per-host-limit";
  spec.scope() = "host";
  spec.partition() = "service";
  spec.dimension() = "cpu";
  spec.intent() =
      CapacityWithGroupPresenceUsageIntent::PER_GROUP_AND_SCOPE_ITEM;

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.globalLimit() = 1.0;
  spec.groupToPresenceWeight() = std::move(groupToPresenceWeight);

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  // Per-(service, host) limits are not directly supported via tuples.
  // Use globalLimit as default.
  scopeItemToLimit.globalLimit() = 50.0; // Default
  spec.scopeItemToLimit() = std::move(scopeItemToLimit);

  solver.addConstraint(std::move(spec));
  // per_group_and_scope_item_limits_end
}

void combining_with_balance() {
  /**
   * Capacity with overhead + balance.
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
          {"task0", 10.0},
          {"task1", 20.0},
          {"task2", 15.0},
          {"task3", 25.0},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
      });

  // combining_with_balance_start
  // Hard: Capacity with overhead
  CapacityWithGroupPresenceSpec cap_spec;
  cap_spec.name() = "capacity-overhead";
  cap_spec.scope() = "host";
  cap_spec.partition() = "service";
  cap_spec.dimension() = "memory";

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.globalLimit() = 10.0;
  cap_spec.groupToPresenceWeight() = std::move(groupToPresenceWeight);

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  scopeItemToLimit.globalLimit() = 128.0;
  cap_spec.scopeItemToLimit() = std::move(scopeItemToLimit);

  solver.addConstraint(std::move(cap_spec));

  // Soft: Balance load
  BalanceSpec balance_spec;
  balance_spec.name() = "balance-memory";
  balance_spec.scope() = "host";
  balance_spec.dimension() = "memory";

  solver.addGoal(std::move(balance_spec), 1.0);
  // combining_with_balance_end
}

void combining_with_group_count() {
  /**
   * Limit overhead sources + capacity.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3", "task4", "task5"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 5.0},
          {"task1", 8.0},
          {"task2", 3.0},
          {"task3", 6.0},
          {"task4", 4.0},
          {"task5", 7.0},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
          {"svc_c", {"task4", "task5"}},
      });

  // combining_with_group_count_start
  // Limit number of services per host (to limit overhead)
  GroupCountSpec gc_spec;
  gc_spec.name() = "max-services-per-host";
  gc_spec.scope() = "host";
  gc_spec.partitionName() = "service";
  gc_spec.bound() = GroupCountSpecBound::MAX;

  Limit gc_limit;
  gc_limit.type() = LimitType::ABSOLUTE;
  gc_limit.globalLimit() = 5; // Max 5 services
  gc_spec.limit() = std::move(gc_limit);

  solver.addConstraint(std::move(gc_spec));

  // Capacity with per-service overhead
  CapacityWithGroupPresenceSpec cap_spec;
  cap_spec.name() = "capacity-with-overhead";
  cap_spec.scope() = "host";
  cap_spec.partition() = "service";
  cap_spec.dimension() = "memory";

  Limit groupToPresenceWeight;
  groupToPresenceWeight.type() = LimitType::ABSOLUTE;
  groupToPresenceWeight.globalLimit() = 10.0; // 10 GB per service
  cap_spec.groupToPresenceWeight() = std::move(groupToPresenceWeight);

  Limit scopeItemToLimit;
  scopeItemToLimit.type() = LimitType::ABSOLUTE;
  scopeItemToLimit.globalLimit() = 100.0;
  cap_spec.scopeItemToLimit() = std::move(scopeItemToLimit);

  solver.addConstraint(std::move(cap_spec));
  // combining_with_group_count_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "2. Service Overhead Costs...\n";
  service_overhead_costs();

  std::cout << "3. Per Service Custom Overhead...\n";
  per_service_custom_overhead();

  std::cout << "4. Connection Pool Overhead...\n";
  connection_pool_overhead();

  std::cout << "5. Rounding For Integer Resources...\n";
  rounding_for_integer_resources();

  std::cout << "6. Multipliers For Replica Overhead...\n";
  multipliers_for_replica_overhead();

  std::cout << "7. Multi Level Multipliers...\n";
  multi_level_multipliers();

  std::cout << "8. Aggregation Scope...\n";
  aggregation_scope();

  std::cout << "9. Per Group And Scope Item Limits...\n";
  per_group_and_scope_item_limits();

  std::cout << "10. Combining With Balance...\n";
  combining_with_balance();

  std::cout << "11. Combining With Group Count...\n";
  combining_with_group_count();

  std::cout << "\n✓ All examples completed successfully!\n";
  return 0;
}
// example_end
