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
 * DisasterRecoveryCapacitySpec Reference Examples
 *
 * This file demonstrates all the usage patterns for
 * DisasterRecoveryCapacitySpec shown in the reference documentation. Each
 * function is a complete, runnable example.
 */

// example_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <fmt/format.h>
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
   * Quick example showing basic DisasterRecoveryCapacitySpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"db_primary_0", "db_replica_1_a", "db_replica_2_a"}},
          {"rack2", {"db_primary_1", "db_replica_0_a", "db_replica_2_b"}},
          {"rack3", {"db_primary_2", "db_replica_0_b", "db_replica_1_b"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"db_primary_0", 10.0},
          {"db_primary_1", 12.0},
          {"db_primary_2", 8.0},
          {"db_replica_0_a", 0.0},
          {"db_replica_0_b", 0.0},
          {"db_replica_1_a", 0.0},
          {"db_replica_1_b", 0.0},
          {"db_replica_2_a", 0.0},
          {"db_replica_2_b", 0.0},
      });
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{
          {"rack1", 50.0},
          {"rack2", 50.0},
          {"rack3", 50.0},
      });

  // quick_example_start
  // Ensure capacity after single rack failure
  DisasterRecoveryCapacitySpec spec;
  spec.name() = "rack-failure-dr";
  spec.scope() = "rack";
  spec.dimension() = "cpu_usage";
  spec.sharedDisasterGroups() = {
      {"rack1"}, // Scenario: rack1 fails
      {"rack2"}, // Scenario: rack2 fails
      {"rack3"}, // Scenario: rack3 fails
  };
  spec.primaryToSetOfSecondaryObjects() = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
      {"db_primary_1", {"db_replica_1_a", "db_replica_1_b"}},
      {"db_primary_2", {"db_replica_2_a", "db_replica_2_b"}},
  };
  solver.addConstraint(std::move(spec));
  // quick_example_end
}

void single_rack_failure() {
  /**
   * Pattern 1: Single Rack Failure - ensure capacity after any single rack
   * fails.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1",
           {"db_shard_0_primary",
            "db_shard_1_replica_a",
            "db_shard_2_replica_a"}},
          {"rack2",
           {"db_shard_1_primary",
            "db_shard_0_replica_a",
            "db_shard_2_replica_b"}},
          {"rack3",
           {"db_shard_2_primary",
            "db_shard_0_replica_b",
            "db_shard_1_replica_b"}},
          {"rack4", {}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"db_shard_0_primary", 10.0},
          {"db_shard_1_primary", 12.0},
          {"db_shard_2_primary", 8.0},
          {"db_shard_0_replica_a", 0.0},
          {"db_shard_0_replica_b", 0.0},
          {"db_shard_1_replica_a", 0.0},
          {"db_shard_1_replica_b", 0.0},
          {"db_shard_2_replica_a", 0.0},
          {"db_shard_2_replica_b", 0.0},
      });
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{
          {"rack1", 50.0},
          {"rack2", 50.0},
          {"rack3", 50.0},
          {"rack4", 50.0},
      });

  // single_rack_failure_start
  // Database with primary-replica setup
  std::vector<std::string> primaries = {
      "db_shard_0_primary", "db_shard_1_primary", "db_shard_2_primary"};
  std::map<std::string, std::vector<std::string>> replicas = {
      {"db_shard_0_primary", {"db_shard_0_replica_a", "db_shard_0_replica_b"}},
      {"db_shard_1_primary", {"db_shard_1_replica_a", "db_shard_1_replica_b"}},
      {"db_shard_2_primary", {"db_shard_2_replica_a", "db_shard_2_replica_b"}},
  };

  // Each rack as separate disaster group
  std::vector<std::string> all_racks = {"rack1", "rack2", "rack3", "rack4"};

  DisasterRecoveryCapacitySpec spec;
  spec.name() = "single-rack-failure";
  spec.scope() = "rack";
  spec.dimension() = "cpu_usage";
  // Each rack fails independently
  std::vector<std::set<std::string>> disaster_groups;
  disaster_groups.reserve(all_racks.size());
  for (const auto& rack : all_racks) {
    disaster_groups.push_back({rack});
  }
  spec.sharedDisasterGroups() = std::move(disaster_groups);
  spec.primaryToSetOfSecondaryObjects() = std::move(replicas);
  solver.addConstraint(std::move(spec));
  // single_rack_failure_end
}

void datacenter_failure() {
  /**
   * Pattern 2: Datacenter Failure - ensure capacity after entire datacenter
   * fails.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("service");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"web_service_dc1", "api_service_dc1"}},
          {"rack2", {"db_service_dc1"}},
          {"rack3", {}},
          {"rack4", {"web_service_dc2"}},
          {"rack5", {"api_service_dc2"}},
          {"rack6", {"db_service_dc2"}},
          {"rack7", {"web_service_dc3"}},
          {"rack8", {"api_service_dc3"}},
          {"rack9", {"db_service_dc3"}},
      });
  solver.addObjectDimension(
      "network_bandwidth",
      std::map<std::string, double>{
          {"web_service_dc1", 10.0},
          {"web_service_dc2", 0.0},
          {"web_service_dc3", 0.0},
          {"api_service_dc1", 8.0},
          {"api_service_dc2", 0.0},
          {"api_service_dc3", 0.0},
          {"db_service_dc1", 5.0},
          {"db_service_dc2", 0.0},
          {"db_service_dc3", 0.0},
      });
  solver.addContainerDimension(
      "network_bandwidth_capacity",
      std::map<std::string, double>{
          {"rack1", 50.0},
          {"rack2", 50.0},
          {"rack3", 50.0},
          {"rack4", 50.0},
          {"rack5", 50.0},
          {"rack6", 50.0},
          {"rack7", 50.0},
          {"rack8", 50.0},
          {"rack9", 50.0},
      });

  // datacenter_failure_start
  // Services with cross-datacenter replicas
  std::map<std::string, std::vector<std::string>> services = {
      {"web_service_dc1", {"web_service_dc2", "web_service_dc3"}},
      {"api_service_dc1", {"api_service_dc2", "api_service_dc3"}},
      {"db_service_dc1", {"db_service_dc2", "db_service_dc3"}},
  };

  // Datacenters
  std::map<std::string, std::vector<std::string>> datacenters = {
      {"dc1", {"rack1", "rack2", "rack3"}},
      {"dc2", {"rack4", "rack5", "rack6"}},
      {"dc3", {"rack7", "rack8", "rack9"}},
  };

  DisasterRecoveryCapacitySpec spec;
  spec.name() = "datacenter-failure";
  spec.scope() = "rack";
  spec.dimension() = "network_bandwidth";
  spec.sharedDisasterGroups() = {
      {"rack1", "rack2", "rack3"}, // All racks in DC1 fail together
      {"rack4", "rack5", "rack6"}, // All racks in DC2 fail together
      {"rack7", "rack8", "rack9"}, // All racks in DC3 fail together
  };
  spec.primaryToSetOfSecondaryObjects() = std::move(services);
  solver.addConstraint(std::move(spec));
  // datacenter_failure_end
}

void availability_zone_failure() {
  /**
   * Pattern 3: Availability Zone Failure - cloud provider AZ failure tolerance.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host_1a_1", {"shard_0_primary_1a", "shard_1_replica_1a"}},
          {"host_1a_2", {"shard_2_replica_1a"}},
          {"host_1a_3", {}},
          {"host_1b_1", {"shard_1_primary_1b", "shard_0_replica_1b"}},
          {"host_1b_2", {"shard_2_replica_1b"}},
          {"host_1b_3", {}},
          {"host_1c_1", {"shard_2_primary_1c", "shard_0_replica_1c"}},
          {"host_1c_2", {"shard_1_replica_1c"}},
          {"host_1c_3", {}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"shard_0_primary_1a", 10.0},
          {"shard_0_replica_1b", 0.0},
          {"shard_0_replica_1c", 0.0},
          {"shard_1_primary_1b", 12.0},
          {"shard_1_replica_1a", 0.0},
          {"shard_1_replica_1c", 0.0},
          {"shard_2_primary_1c", 8.0},
          {"shard_2_replica_1a", 0.0},
          {"shard_2_replica_1b", 0.0},
      });
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{
          {"host_1a_1", 50.0},
          {"host_1a_2", 50.0},
          {"host_1a_3", 50.0},
          {"host_1b_1", 50.0},
          {"host_1b_2", 50.0},
          {"host_1b_3", 50.0},
          {"host_1c_1", 50.0},
          {"host_1c_2", 50.0},
          {"host_1c_3", 50.0},
      });

  // availability_zone_failure_start
  // Objects per AZ
  std::map<std::string, std::vector<std::string>> az_hosts = {
      {"us-east-1a", {"host_1a_1", "host_1a_2", "host_1a_3"}},
      {"us-east-1b", {"host_1b_1", "host_1b_2", "host_1b_3"}},
      {"us-east-1c", {"host_1c_1", "host_1c_2", "host_1c_3"}},
  };

  // Primary to secondary mapping
  std::map<std::string, std::vector<std::string>> primary_to_secondary = {
      {"shard_0_primary_1a", {"shard_0_replica_1b", "shard_0_replica_1c"}},
      {"shard_1_primary_1b", {"shard_1_replica_1a", "shard_1_replica_1c"}},
      {"shard_2_primary_1c", {"shard_2_replica_1a", "shard_2_replica_1b"}},
  };

  DisasterRecoveryCapacitySpec spec;
  spec.name() = "az-failure-tolerance";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.sharedDisasterGroups() = {
      {"host_1a_1", "host_1a_2", "host_1a_3"}, // AZ-a fails
      {"host_1b_1", "host_1b_2", "host_1b_3"}, // AZ-b fails
      {"host_1c_1", "host_1c_2", "host_1c_3"}, // AZ-c fails
  };
  spec.primaryToSetOfSecondaryObjects() = std::move(primary_to_secondary);
  solver.addConstraint(std::move(spec));
  // availability_zone_failure_end
}

void multi_replica_failover() {
  /**
   * Pattern 4: Multi-Replica Failover - multiple replica promotion levels.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1",
           {"db_primary_0", "db_replica_1_level1", "db_replica_2_level2"}},
          {"rack2",
           {"db_primary_1", "db_replica_0_level1", "db_replica_2_level1"}},
          {"rack3",
           {"db_primary_2", "db_replica_0_level2", "db_replica_1_level2"}},
      });
  solver.addObjectDimension(
      "storage",
      std::map<std::string, double>{
          {"db_primary_0", 100.0},
          {"db_primary_1", 120.0},
          {"db_primary_2", 80.0},
          {"db_replica_0_level1", 0.0},
          {"db_replica_0_level2", 0.0},
          {"db_replica_1_level1", 0.0},
          {"db_replica_1_level2", 0.0},
          {"db_replica_2_level1", 0.0},
          {"db_replica_2_level2", 0.0},
      });
  solver.addContainerDimension(
      "storage_capacity",
      std::map<std::string, double>{
          {"rack1", 500.0}, {"rack2", 500.0}, {"rack3", 500.0}});

  // multi_replica_failover_start
  // Primary → replica1 (first backup) → replica2 (second backup)
  std::map<std::string, std::vector<std::string>> multi_level_replicas = {
      {"db_primary_0", {"db_replica_0_level1", "db_replica_0_level2"}},
      {"db_primary_1", {"db_replica_1_level1", "db_replica_1_level2"}},
      {"db_primary_2", {"db_replica_2_level1", "db_replica_2_level2"}},
  };

  DisasterRecoveryCapacitySpec spec;
  spec.name() = "multi-level-failover";
  spec.scope() = "rack";
  spec.dimension() = "storage";
  spec.sharedDisasterGroups() = {{"rack1"}, {"rack2"}, {"rack3"}};
  spec.primaryToSetOfSecondaryObjects() = std::move(multi_level_replicas);
  solver.addConstraint(std::move(spec));

  // When primary fails, level1 replica activates
  // If level1 also fails, level2 replica activates
  // multi_replica_failover_end
}

void partial_failure_groups() {
  /**
   * Pattern 5: Partial Failure Groups - some scope items share disaster fate.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"db_primary_0"}},
          {"rack2", {"db_replica_0_a"}},
          {"rack3", {"db_primary_1"}},
          {"rack4", {"db_replica_1_a"}},
          {"rack5", {"db_replica_0_b"}},
          {"rack6", {"db_replica_1_b"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"db_primary_0", 10.0},
          {"db_primary_1", 12.0},
          {"db_replica_0_a", 0.0},
          {"db_replica_0_b", 0.0},
          {"db_replica_1_a", 0.0},
          {"db_replica_1_b", 0.0},
      });
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{
          {"rack1", 50.0},
          {"rack2", 50.0},
          {"rack3", 50.0},
          {"rack4", 50.0},
          {"rack5", 50.0},
          {"rack6", 50.0},
      });

  // Assume replica_mapping is defined elsewhere
  std::map<std::string, std::vector<std::string>> replica_mapping = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
      {"db_primary_1", {"db_replica_1_a", "db_replica_1_b"}},
  };

  // partial_failure_groups_start
  // Racks on same power circuit fail together
  std::vector<std::set<std::string>> power_circuit_groups = {
      {"rack1", "rack2"}, // Circuit A
      {"rack3", "rack4"}, // Circuit B
      {"rack5", "rack6"}, // Circuit C
  };

  DisasterRecoveryCapacitySpec spec;
  spec.name() = "power-circuit-failure";
  spec.scope() = "rack";
  spec.dimension() = "cpu_usage";
  spec.sharedDisasterGroups() = std::move(power_circuit_groups);
  spec.primaryToSetOfSecondaryObjects() = std::move(replica_mapping);
  solver.addConstraint(std::move(spec));
  // partial_failure_groups_end
}

void no_shared_disaster_groups() {
  /**
   * Pattern 6: No Shared Disaster Groups - each scope item fails independently.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host1", {"primary_0", "replica_1_a"}},
          {"host2", {"primary_1", "replica_0_a"}},
          {"host3", {"replica_0_b", "replica_1_b"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"primary_0", 10.0},
          {"primary_1", 12.0},
          {"replica_0_a", 0.0},
          {"replica_0_b", 0.0},
          {"replica_1_a", 0.0},
          {"replica_1_b", 0.0},
      });
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{
          {"host1", 50.0}, {"host2", 50.0}, {"host3", 50.0}});

  // no_shared_disaster_groups_start
  // If sharedDisasterGroups not specified, each scope item is independent
  DisasterRecoveryCapacitySpec spec;
  spec.name() = "independent-failures";
  spec.scope() = "host";
  spec.dimension() = "memory";
  // sharedDisasterGroups omitted → each host fails alone
  spec.primaryToSetOfSecondaryObjects() = {
      {"primary_0", {"replica_0_a", "replica_0_b"}},
      {"primary_1", {"replica_1_a", "replica_1_b"}},
  };
  solver.addConstraint(std::move(spec));

  // Tests capacity for: host1 fails, host2 fails, host3 fails (separately)
  // no_shared_disaster_groups_end
}

void cross_dimension_dr() {
  /**
   * Pattern 7: Cross-Dimension DR - different dimensions for different failure
   * modes.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"db_primary_0", "db_replica_1_a"}},
          {"rack2", {"db_primary_1", "db_replica_0_a"}},
          {"rack3", {"db_replica_0_b", "db_replica_1_b"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"db_primary_0", 10.0},
          {"db_primary_1", 12.0},
          {"db_replica_0_a", 0.0},
          {"db_replica_0_b", 0.0},
          {"db_replica_1_a", 0.0},
          {"db_replica_1_b", 0.0},
      });
  solver.addObjectDimension(
      "network_bandwidth",
      std::map<std::string, double>{
          {"db_primary_0", 5.0},
          {"db_primary_1", 8.0},
          {"db_replica_0_a", 0.0},
          {"db_replica_0_b", 0.0},
          {"db_replica_1_a", 0.0},
          {"db_replica_1_b", 0.0},
      });
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{
          {"rack1", 50.0}, {"rack2", 50.0}, {"rack3", 50.0}});
  solver.addContainerDimension(
      "network_bandwidth_capacity",
      std::map<std::string, double>{
          {"rack1", 50.0}, {"rack2", 50.0}, {"rack3", 50.0}});

  // Assume these are defined elsewhere
  std::map<std::string, std::vector<std::string>> cpu_replicas = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
      {"db_primary_1", {"db_replica_1_a", "db_replica_1_b"}},
  };
  std::map<std::string, std::vector<std::string>> network_replicas = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
      {"db_primary_1", {"db_replica_1_a", "db_replica_1_b"}},
  };

  // cross_dimension_dr_start
  // CPU capacity for rack failure
  DisasterRecoveryCapacitySpec cpuSpec;
  cpuSpec.name() = "rack-failure-cpu";
  cpuSpec.scope() = "rack";
  cpuSpec.dimension() = "cpu_usage";
  cpuSpec.sharedDisasterGroups() = {{"rack1"}, {"rack2"}};
  cpuSpec.primaryToSetOfSecondaryObjects() = std::move(cpu_replicas);
  solver.addConstraint(std::move(cpuSpec));

  // Network capacity for same failure
  DisasterRecoveryCapacitySpec networkSpec;
  networkSpec.name() = "rack-failure-network";
  networkSpec.scope() = "rack";
  networkSpec.dimension() = "network_bandwidth";
  networkSpec.sharedDisasterGroups() = {{"rack1"}, {"rack2"}};
  networkSpec.primaryToSetOfSecondaryObjects() = std::move(network_replicas);
  solver.addConstraint(std::move(networkSpec));
  // cross_dimension_dr_end
}

void asymmetric_replica_distribution() {
  /**
   * Pattern 8: Asymmetric Replica Distribution - different numbers of replicas
   * per primary.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"critical_db_primary", "non_critical_db_replica_1"}},
          {"rack2", {"critical_db_replica_1", "non_critical_db_primary"}},
          {"rack3", {"critical_db_replica_2"}},
          {"rack4", {"critical_db_replica_3"}},
      });
  solver.addObjectDimension(
      "storage",
      std::map<std::string, double>{
          {"critical_db_primary", 100.0},
          {"critical_db_replica_1", 0.0},
          {"critical_db_replica_2", 0.0},
          {"critical_db_replica_3", 0.0},
          {"non_critical_db_primary", 50.0},
          {"non_critical_db_replica_1", 0.0},
      });
  solver.addContainerDimension(
      "storage_capacity",
      std::map<std::string, double>{
          {"rack1", 500.0},
          {"rack2", 500.0},
          {"rack3", 500.0},
          {"rack4", 500.0},
      });

  // asymmetric_replica_distribution_start
  // Critical services have more replicas
  std::map<std::string, std::vector<std::string>> asymmetric_replicas = {
      {"critical_db_primary",
       {
           "critical_db_replica_1",
           "critical_db_replica_2",
           "critical_db_replica_3", // 3 replicas
       }},
      {"non_critical_db_primary",
       {
           "non_critical_db_replica_1", // Only 1 replica
       }},
  };

  DisasterRecoveryCapacitySpec spec;
  spec.name() = "asymmetric-dr";
  spec.scope() = "rack";
  spec.dimension() = "storage";
  spec.sharedDisasterGroups() = {{"rack1"}, {"rack2"}};
  spec.primaryToSetOfSecondaryObjects() = std::move(asymmetric_replicas);
  solver.addConstraint(std::move(spec));
  // asymmetric_replica_distribution_end
}

void pitfall_insufficient_capacity_bad() {
  /**
   * Pitfall 1 BAD: Insufficient capacity for failover.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  // pitfall_insufficient_capacity_bad_start
  // BAD: 3 racks, each at 90% capacity
  // If one rack fails, remaining 2 racks must absorb 90% extra load
  // New load per rack: 90% (existing) + 45% (failover) = 135% (exceeds
  // capacity!) pitfall_insufficient_capacity_bad_end
}

void pitfall_insufficient_capacity_good() {
  /**
   * Pitfall 1 GOOD: Ensure N+1 or N+2 capacity.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"db_primary_0", "db_replica_1_a"}},
          {"rack2", {"db_primary_1", "db_replica_0_a"}},
          {"rack3", {"db_replica_0_b", "db_replica_1_b"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"db_primary_0", 10.0},
          {"db_primary_1", 12.0},
          {"db_replica_0_a", 0.0},
          {"db_replica_0_b", 0.0},
          {"db_replica_1_a", 0.0},
          {"db_replica_1_b", 0.0},
      });
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{
          {"rack1", 50.0}, {"rack2", 50.0}, {"rack3", 50.0}});

  // Assume replicas is defined elsewhere
  std::map<std::string, std::vector<std::string>> replicas = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
      {"db_primary_1", {"db_replica_1_a", "db_replica_1_b"}},
  };

  // pitfall_insufficient_capacity_good_start
  // GOOD: Size for N-1 racks (if you have N racks)
  // 3 racks: each at max 50% capacity
  // If one fails: 2 racks × 50% = 100% (can handle failover)

  // Or use DR as soft goal with lower weight
  DisasterRecoveryCapacitySpec spec;
  spec.name() = "best-effort-dr";
  spec.scope() = "rack";
  spec.dimension() = "cpu_usage";
  spec.sharedDisasterGroups() = {{"rack1"}, {"rack2"}, {"rack3"}};
  spec.primaryToSetOfSecondaryObjects() = std::move(replicas);
  solver.addGoal(std::move(spec), 0.5); // Soft goal - do best effort
  // pitfall_insufficient_capacity_good_end
}

void pitfall_missing_mapping_bad() {
  /**
   * Pitfall 2 BAD: Primary objects without secondaries.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  // pitfall_missing_mapping_bad_start
  // BAD: primary_2 has no secondaries
  std::map<std::string, std::vector<std::string>>
      primaryToSetOfSecondaryObjects = {
          {"primary_0", {"replica_0"}},
          {"primary_1", {"replica_1"}},
          // primary_2 missing!
      };
  // If primary_2 fails, no failover!
  // pitfall_missing_mapping_bad_end
}

void pitfall_missing_mapping_good() {
  /**
   * Pitfall 2 GOOD: Ensure all primaries have secondaries.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  // pitfall_missing_mapping_good_start
  // GOOD: All primaries mapped
  std::map<std::string, std::vector<std::string>>
      primaryToSetOfSecondaryObjects = {
          {"primary_0", {"replica_0"}},
          {"primary_1", {"replica_1"}},
          {"primary_2", {"replica_2"}}, // Added
      };
  // pitfall_missing_mapping_good_end
}

void pitfall_same_disaster_group_bad() {
  /**
   * Pitfall 3 BAD: Primary and secondary in same failure domain.
   */
  // pitfall_same_disaster_group_bad_start
  // BAD: Primary and replica on same rack
  // rack1: [primary_0, replica_0]  // Both on rack1
  // If rack1 fails, BOTH primary AND replica fail! (no DR)
  // pitfall_same_disaster_group_bad_end
}

void pitfall_same_disaster_group_good() {
  /**
   * Pitfall 3 GOOD: Use anti-affinity to spread primaries and secondaries.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"primary_0", "replica_0"}},
          {"rack2", {"primary_1", "replica_1"}},
      });
  solver.addPartition(
      "replica_set",
      std::map<std::string, std::vector<std::string>>{
          {"set0", {"primary_0", "replica_0"}},
          {"set1", {"primary_1", "replica_1"}},
      });

  // pitfall_same_disaster_group_good_start
  // Ensure primary and replicas are on different racks
  GroupCountSpec spreadSpec;
  spreadSpec.name() = "spread-replicas";
  spreadSpec.scope() = "rack";
  spreadSpec.partitionName() = "replica_set"; // Group primary+replicas
  spreadSpec.bound() = GroupCountSpecBound::MAX;
  Limit limit;
  limit.globalLimit() = 1; // Max 1 per rack
  spreadSpec.limit() = std::move(limit);
  solver.addConstraint(std::move(spreadSpec));
  // pitfall_same_disaster_group_good_end
}

void pitfall_disaster_groups_confusion_bad() {
  /**
   * Pitfall 4 BAD: Misunderstanding how disaster groups work.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  // pitfall_disaster_groups_confusion_bad_start
  // CONFUSION: Thinking this tests "any 2 racks fail"
  std::vector<std::set<std::string>> sharedDisasterGroups = {
      {"rack1"},
      {"rack2"},
  };
  // Actually tests: "rack1 fails" OR "rack2 fails" (separately)
  // Does NOT test "rack1 AND rack2 both fail together"
  // pitfall_disaster_groups_confusion_bad_end
}

void pitfall_disaster_groups_confusion_good() {
  /**
   * Pitfall 4 GOOD: Disaster groups are OR scenarios, not AND.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  // pitfall_disaster_groups_confusion_good_start
  // To test "rack1 AND rack2 fail together"
  std::vector<std::set<std::string>> sharedDisasterGroups = {
      {"rack1", "rack2"}, // Both fail in this scenario
  };
  // pitfall_disaster_groups_confusion_good_end
}

void pitfall_dimension_mismatch_bad() {
  /**
   * Pitfall 5 BAD: Dimension doesn't exist or isn't relevant.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"db_primary_0", "db_replica_0_b"}},
          {"rack2", {"db_replica_0_a"}},
      });
  solver.addObjectDimension(
      "object_count",
      std::map<std::string, double>{
          {"db_primary_0", 1.0},
          {"db_replica_0_a", 1.0},
          {"db_replica_0_b", 1.0},
      });
  solver.addContainerDimension(
      "object_count_capacity",
      std::map<std::string, double>{{"rack1", 10.0}, {"rack2", 10.0}});

  // Assume replicas is defined elsewhere
  std::map<std::string, std::vector<std::string>> replicas = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
  };

  // pitfall_dimension_mismatch_bad_start
  // BAD: Using dimension that doesn't represent failover load
  DisasterRecoveryCapacitySpec spec;
  spec.name() = "bad-dimension-dr";
  spec.scope() = "rack";
  spec.dimension() = "object_count"; // Not meaningful for DR!
  spec.sharedDisasterGroups() = {{"rack1"}};
  spec.primaryToSetOfSecondaryObjects() = std::move(replicas);
  solver.addConstraint(std::move(spec));
  // pitfall_dimension_mismatch_bad_end
}

void pitfall_dimension_mismatch_good() {
  /**
   * Pitfall 5 GOOD: Use resource dimensions (CPU, memory, network).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"db_primary_0", "db_replica_0_b"}},
          {"rack2", {"db_replica_0_a"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"db_primary_0", 10.0},
          {"db_replica_0_a", 0.0},
          {"db_replica_0_b", 0.0},
      });
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{{"rack1", 50.0}, {"rack2", 50.0}});

  // Assume replicas is defined elsewhere
  std::map<std::string, std::vector<std::string>> replicas = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
  };

  // pitfall_dimension_mismatch_good_start
  // GOOD: Use actual resource dimensions
  DisasterRecoveryCapacitySpec spec;
  spec.name() = "good-dimension-dr";
  spec.scope() = "rack";
  spec.dimension() = "cpu_usage"; // Actual resource
  spec.sharedDisasterGroups() = {{"rack1"}};
  spec.primaryToSetOfSecondaryObjects() = std::move(replicas);
  solver.addConstraint(std::move(spec));
  // pitfall_dimension_mismatch_good_end
}

void combining_with_capacity_spec() {
  /**
   * Combining: Normal capacity + DR capacity.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"db_primary_0", "db_replica_1_a"}},
          {"rack2", {"db_primary_1", "db_replica_0_a"}},
          {"rack3", {"db_replica_0_b", "db_replica_1_b"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"db_primary_0", 10.0},
          {"db_primary_1", 12.0},
          {"db_replica_0_a", 0.0},
          {"db_replica_0_b", 0.0},
          {"db_replica_1_a", 0.0},
          {"db_replica_1_b", 0.0},
      });
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{
          {"rack1", 50.0}, {"rack2", 50.0}, {"rack3", 50.0}});

  // Assume replicas is defined elsewhere
  std::map<std::string, std::vector<std::string>> replicas = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
      {"db_primary_1", {"db_replica_1_a", "db_replica_1_b"}},
  };

  // combining_with_capacity_spec_start
  // Hard: Normal capacity limits
  CapacitySpec normalCapacity;
  normalCapacity.name() = "normal-capacity";
  normalCapacity.scope() = "rack";
  normalCapacity.dimension() = "cpu_usage";
  solver.addConstraint(std::move(normalCapacity));

  // Hard: DR capacity (must handle failover)
  DisasterRecoveryCapacitySpec drCapacity;
  drCapacity.name() = "dr-capacity";
  drCapacity.scope() = "rack";
  drCapacity.dimension() = "cpu_usage";
  drCapacity.sharedDisasterGroups() = {{"rack1"}, {"rack2"}, {"rack3"}};
  drCapacity.primaryToSetOfSecondaryObjects() = std::move(replicas);
  solver.addConstraint(std::move(drCapacity));
  // combining_with_capacity_spec_end
}

void combining_with_group_count_spec() {
  /**
   * Combining: Spread replicas + DR capacity.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"db_primary_0", "db_replica_1_a"}},
          {"rack2", {"db_primary_1", "db_replica_0_a"}},
          {"rack3", {"db_replica_0_b", "db_replica_1_b"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"db_primary_0", 10.0},
          {"db_primary_1", 12.0},
          {"db_replica_0_a", 0.0},
          {"db_replica_0_b", 0.0},
          {"db_replica_1_a", 0.0},
          {"db_replica_1_b", 0.0},
      });
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{
          {"rack1", 50.0}, {"rack2", 50.0}, {"rack3", 50.0}});
  solver.addPartition(
      "shard",
      std::map<std::string, std::vector<std::string>>{
          {"shard0", {"db_primary_0", "db_replica_0_a", "db_replica_0_b"}},
          {"shard1", {"db_primary_1", "db_replica_1_a", "db_replica_1_b"}},
      });

  // Assume replicas is defined elsewhere
  std::map<std::string, std::vector<std::string>> replicas = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
      {"db_primary_1", {"db_replica_1_a", "db_replica_1_b"}},
  };

  // combining_with_group_count_spec_start
  // Ensure replicas spread across racks
  GroupCountSpec spreadSpec;
  spreadSpec.name() = "replica-spread";
  spreadSpec.scope() = "rack";
  spreadSpec.partitionName() = "shard";
  spreadSpec.bound() = GroupCountSpecBound::MAX;
  Limit limit;
  limit.globalLimit() = 1;
  spreadSpec.limit() = std::move(limit);
  solver.addConstraint(std::move(spreadSpec));

  // Ensure DR capacity
  DisasterRecoveryCapacitySpec drCapacity;
  drCapacity.name() = "dr-capacity";
  drCapacity.scope() = "rack";
  drCapacity.dimension() = "memory";
  drCapacity.sharedDisasterGroups() = {{"rack1"}, {"rack2"}};
  drCapacity.primaryToSetOfSecondaryObjects() = std::move(replicas);
  solver.addConstraint(std::move(drCapacity));
  // combining_with_group_count_spec_end
}

void combining_with_balance_spec() {
  /**
   * Combining: Balance load + ensure DR capacity.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("database");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"db_primary_0", "db_replica_1_a"}},
          {"rack2", {"db_primary_1", "db_replica_0_a"}},
          {"rack3", {"db_replica_0_b", "db_replica_1_b"}},
      });
  solver.addObjectDimension(
      "cpu_usage",
      std::map<std::string, double>{
          {"db_primary_0", 10.0},
          {"db_primary_1", 12.0},
          {"db_replica_0_a", 0.0},
          {"db_replica_0_b", 0.0},
          {"db_replica_1_a", 0.0},
          {"db_replica_1_b", 0.0},
      });
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{
          {"rack1", 50.0}, {"rack2", 50.0}, {"rack3", 50.0}});

  // Assume replicas is defined elsewhere
  std::map<std::string, std::vector<std::string>> replicas = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
      {"db_primary_1", {"db_replica_1_a", "db_replica_1_b"}},
  };

  // combining_with_balance_spec_start
  // Soft: Balance load across racks
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-racks";
  balanceSpec.scope() = "rack";
  balanceSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Hard: Ensure DR capacity
  DisasterRecoveryCapacitySpec drCapacity;
  drCapacity.name() = "rack-failure-dr";
  drCapacity.scope() = "rack";
  drCapacity.dimension() = "cpu_usage";
  drCapacity.sharedDisasterGroups() = {{"rack1"}, {"rack2"}};
  drCapacity.primaryToSetOfSecondaryObjects() = std::move(replicas);
  solver.addConstraint(std::move(drCapacity));
  // combining_with_balance_spec_end
}

void complete_example() {
  /**
   * Complete runnable example demonstrating DisasterRecoveryCapacitySpec.
   *
   * This example shows how to use DisasterRecoveryCapacitySpec to ensure
   * capacity for database failover after rack failures.
   */
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "dr-capacity-example", "production");

  solver.setObjectName("database");
  solver.setContainerName("rack");

  // Define racks
  std::vector<std::string> racks = {"rack0", "rack1", "rack2", "rack3"};

  // Initial assignment - primaries and replicas distributed across racks
  std::map<std::string, std::vector<std::string>> assignment = {
      {"rack0", {"db_primary_0", "db_replica_1_a"}},
      {"rack1", {"db_primary_1", "db_replica_0_a"}},
      {"rack2", {"db_replica_0_b", "db_replica_1_b"}},
      {"rack3", {"db_primary_2", "db_replica_2_a"}},
  };
  solver.setAssignment(assignment);

  // Database resource usage (primaries have usage, replicas are standby)
  std::map<std::string, double> cpu_usage = {
      // Primaries
      {"db_primary_0", 10.0},
      {"db_primary_1", 12.0},
      {"db_primary_2", 8.0},
      // Replicas (standby, no CPU until failover)
      {"db_replica_0_a", 0.0},
      {"db_replica_0_b", 0.0},
      {"db_replica_1_a", 0.0},
      {"db_replica_1_b", 0.0},
      {"db_replica_2_a", 0.0},
  };

  std::map<std::string, double> memory_usage = {
      // Primaries
      {"db_primary_0", 16.0},
      {"db_primary_1", 20.0},
      {"db_primary_2", 12.0},
      // Replicas
      {"db_replica_0_a", 0.0},
      {"db_replica_0_b", 0.0},
      {"db_replica_1_a", 0.0},
      {"db_replica_1_b", 0.0},
      {"db_replica_2_a", 0.0},
  };

  solver.addObjectDimension("cpu", cpu_usage);
  solver.addObjectDimension("memory", memory_usage);

  // Rack capacities (sized for N+1 redundancy)
  std::map<std::string, double> rack_cpu_capacity;
  std::map<std::string, double> rack_memory_capacity;
  for (const auto& rack : racks) {
    rack_cpu_capacity[rack] = 50.0;
    rack_memory_capacity[rack] = 80.0;
  }

  solver.addContainerDimension("cpu_capacity", rack_cpu_capacity);
  solver.addContainerDimension("memory_capacity", rack_memory_capacity);

  // Primary to secondary mapping
  std::map<std::string, std::vector<std::string>> primary_to_secondary = {
      {"db_primary_0", {"db_replica_0_a", "db_replica_0_b"}},
      {"db_primary_1", {"db_replica_1_a", "db_replica_1_b"}},
      {"db_primary_2", {"db_replica_2_a"}},
  };

  // Add normal capacity constraints
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "rack";
  cpuCapacity.dimension() = "cpu";
  solver.addConstraint(cpuCapacity);

  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "rack";
  memoryCapacity.dimension() = "memory";
  solver.addConstraint(memoryCapacity);

  // Add DR capacity constraints for single rack failure
  DisasterRecoveryCapacitySpec drCpuCapacity;
  drCpuCapacity.name() = "rack-failure-dr-cpu";
  drCpuCapacity.scope() = "rack";
  drCpuCapacity.dimension() = "cpu";
  std::vector<std::set<std::string>> disaster_groups;
  disaster_groups.reserve(racks.size());
  for (const auto& rack : racks) {
    disaster_groups.push_back({rack});
  }
  drCpuCapacity.sharedDisasterGroups() = disaster_groups;
  drCpuCapacity.primaryToSetOfSecondaryObjects() = primary_to_secondary;
  solver.addConstraint(drCpuCapacity);

  DisasterRecoveryCapacitySpec drMemoryCapacity;
  drMemoryCapacity.name() = "rack-failure-dr-memory";
  drMemoryCapacity.scope() = "rack";
  drMemoryCapacity.dimension() = "memory";
  drMemoryCapacity.sharedDisasterGroups() = disaster_groups;
  drMemoryCapacity.primaryToSetOfSecondaryObjects() = primary_to_secondary;
  solver.addConstraint(drMemoryCapacity);

  // Add balance goal
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "rack";
  balanceCpu.dimension() = "cpu";
  solver.addGoal(balanceCpu, 1.0);

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
  std::cout << "Running all DrCapacity examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Single Rack Failure...\n";
  single_rack_failure();

  std::cout << "\n3. Datacenter Failure...\n";
  datacenter_failure();

  std::cout << "\n4. Availability Zone Failure...\n";
  availability_zone_failure();

  std::cout << "\n5. Multi Replica Failover...\n";
  multi_replica_failover();

  std::cout << "\n6. Partial Failure Groups...\n";
  partial_failure_groups();

  std::cout << "\n7. No Shared Disaster Groups...\n";
  no_shared_disaster_groups();

  std::cout << "\n8. Cross Dimension Dr...\n";
  cross_dimension_dr();

  std::cout << "\n9. Asymmetric Replica Distribution...\n";
  asymmetric_replica_distribution();

  std::cout << "\n10. Pitfall Insufficient Capacity Bad...\n";
  pitfall_insufficient_capacity_bad();

  std::cout << "\n11. Pitfall Insufficient Capacity Good...\n";
  pitfall_insufficient_capacity_good();

  std::cout << "\n12. Pitfall Missing Mapping Bad...\n";
  pitfall_missing_mapping_bad();

  std::cout << "\n13. Pitfall Missing Mapping Good...\n";
  pitfall_missing_mapping_good();

  std::cout << "\n14. Pitfall Same Disaster Group Bad...\n";
  pitfall_same_disaster_group_bad();

  std::cout << "\n15. Pitfall Same Disaster Group Good...\n";
  pitfall_same_disaster_group_good();

  std::cout << "\n16. Pitfall Disaster Groups Confusion Bad...\n";
  pitfall_disaster_groups_confusion_bad();

  std::cout << "\n17. Pitfall Disaster Groups Confusion Good...\n";
  pitfall_disaster_groups_confusion_good();

  std::cout << "\n18. Pitfall Dimension Mismatch Bad...\n";
  pitfall_dimension_mismatch_bad();

  std::cout << "\n19. Pitfall Dimension Mismatch Good...\n";
  pitfall_dimension_mismatch_good();

  std::cout << "\n20. Combining With Capacity Spec...\n";
  combining_with_capacity_spec();

  std::cout << "\n21. Combining With Group Count Spec...\n";
  combining_with_group_count_spec();

  std::cout << "\n22. Combining With Balance Spec...\n";
  combining_with_balance_spec();

  std::cout << "\n23. Complete Example...\n";
  complete_example();

  std::cout << "\n✓ All DrCapacity examples completed successfully!\n";
  return 0;
}
// example_end
