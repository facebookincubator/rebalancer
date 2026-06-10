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
 * ThrottlingSpec Reference Examples
 *
 * This file demonstrates all the usage patterns for ThrottlingSpec shown in the
 * reference documentation. Each function is a complete, runnable example.
 */

// example_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void quick_example() {
  /**
   * Quick example showing basic ThrottlingSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_host_1", {"shard0", "shard1"}},
          {"old_host_2", {"shard2"}},
          {"host0", {"shard3"}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"shard0", 100.0},
          {"shard1", 150.0},
          {"shard2", 200.0},
          {"shard3", 120.0}});

  // quick_example_start
  // Limit data moving OUT of hosts being drained (max 500 GB)
  std::vector<std::string> draining_hosts = {"old_host_1", "old_host_2"};

  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "throttle-drain";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::OUT; // Count moves OUT

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 500.0; // Max 500 GB total
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(draining_hosts);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // quick_example_end
}

void throttle_host_draining() {
  /**
   * Throttle host draining - limit data leaving hosts being drained.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_host_1", {"shard0", "shard1"}},
          {"old_host_2", {"shard2"}},
          {"old_host_3", {"shard3"}},
          {"host0", {"shard4"}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"shard0", 100.0},
          {"shard1", 150.0},
          {"shard2", 200.0},
          {"shard3", 120.0},
          {"shard4", 180.0}});

  // throttle_drain_start
  // Hosts being decommissioned
  std::vector<std::string> draining_hosts = {
      "old_host_1", "old_host_2", "old_host_3"};

  // Limit outbound data movement (network bandwidth protection)
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "drain-throttle";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size"; // GB
  throttle_spec.definition() = ThrottlingSpecDefinition::OUT; // Leaving hosts

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 1000.0; // Max 1TB total across all draining hosts
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(draining_hosts);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // throttle_drain_end
}

void limit_rack_network_usage() {
  /**
   * Limit rack network usage - prevent saturating rack network.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0"}},
          {"host1", {"shard1"}},
          {"host2", {"shard2"}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"shard0", 100.0}, {"shard1", 200.0}, {"shard2", 300.0}});
  solver.addScope(
      "rack",
      std::map<std::string, std::string>{
          {"host0", "rack5"}, {"host1", "rack8"}, {"host2", "rack5"}});

  // limit_rack_network_start
  // Rack with limited network capacity
  std::vector<std::string> bandwidth_limited_racks = {"rack5", "rack8"};

  // Limit total data movement through these racks
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "rack-bandwidth-limit";
  throttle_spec.scope() = "rack";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::ANY; // In or out

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 2000.0; // Max 2TB moved through rack
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(bandwidth_limited_racks);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // limit_rack_network_end
}

void per_host_throttling() {
  /**
   * Per-host throttling - different limits for different hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"slow_network_host_1", {"shard0"}},
          {"slow_network_host_2", {"shard1"}},
          {"fast_network_host_1", {"shard2"}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"shard0", 50.0}, {"shard1", 80.0}, {"shard2", 200.0}});

  // per_host_throttling_start
  // Per-host bandwidth limits
  std::map<std::string, double> host_bandwidth_limits = {
      {"slow_network_host_1", 100.0}, // 100 GB
      {"slow_network_host_2", 100.0},
      {"fast_network_host_1", 500.0}, // 500 GB
  };

  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "per-host-throttle";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::ANY;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.scopeItemLimits() = std::move(host_bandwidth_limits);
  limit.globalLimit() = 200.0; // Default for other hosts
  throttle_spec.limit() = std::move(limit);

  solver.addConstraint(std::move(throttle_spec));
  // per_host_throttling_end
}

void gradual_migration() {
  /**
   * Gradual migration - limit incoming data to new datacenter.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_host", {"shard0", "shard1"}},
          {"new_dc_host_1", {}},
          {"new_dc_host_2", {}},
          {"new_dc_host_3", {}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{{"shard0", 200.0}, {"shard1", 300.0}});

  // gradual_migration_start
  // New datacenter hosts
  std::vector<std::string> new_dc_hosts = {
      "new_dc_host_1", "new_dc_host_2", "new_dc_host_3"};

  // Limit incoming data (gradual migration)
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "gradual-migration";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::IN; // Coming in

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 500.0; // Max 500 GB migrated in this round
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(new_dc_hosts);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // gradual_migration_end
}

void protect_production_hosts() {
  /**
   * Protect production hosts - limit churn on production hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"prod_host_1", {"shard0"}},
          {"prod_host_2", {"shard1"}},
          {"other_host", {"shard2"}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"shard0", 100.0}, {"shard1", 150.0}, {"shard2", 200.0}});

  // protect_prod_start
  // Critical production hosts
  std::vector<std::string> prod_hosts = {"prod_host_1", "prod_host_2"};

  // Limit any movement (stability)
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "prod-stability";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::ANY;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 50.0; // Very small limit - minimize disruption
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(prod_hosts);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // protect_prod_end
}

void network_aware_throttling() {
  /**
   * Network-aware throttling - throttle based on network traffic instead of
   * data size.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"drain_host_1", {"obj0", "obj1", "obj2"}},
      });

  // network_aware_start
  // Dimension: estimated network transfer time
  std::vector<std::string> all_objects = {"obj0", "obj1", "obj2"};
  std::map<std::string, double> data_size = {
      {"obj0", 100.0}, {"obj1", 200.0}, {"obj2", 300.0}};
  std::map<std::string, double> network_bandwidth = {
      {"obj0", 10.0}, {"obj1", 10.0}, {"obj2", 10.0}};
  std::vector<std::string> hosts_to_drain = {"drain_host_1"};

  std::map<std::string, double> network_transfer_time;
  for (const auto& obj : all_objects) {
    network_transfer_time[obj] = data_size[obj] / network_bandwidth[obj];
  }
  solver.addObjectDimension("transfer_time", std::move(network_transfer_time));

  // Limit total transfer time (network hours)
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "network-time-limit";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "transfer_time"; // Hours
  throttle_spec.definition() = ThrottlingSpecDefinition::OUT;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 10.0; // Max 10 hours of network transfer
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(hosts_to_drain);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // network_aware_end
}

void object_count_throttling() {
  /**
   * Object count throttling - limit number of objects moved (not data size).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"sensitive_host_1", {"obj0", "obj1", "obj2", "obj3"}},
      });

  // object_count_throttling_start
  // Use count dimension
  std::vector<std::string> all_objects = {"obj0", "obj1", "obj2", "obj3"};
  std::vector<std::string> sensitive_hosts = {"sensitive_host_1"};

  std::map<std::string, double> object_count;
  for (const auto& obj : all_objects) {
    object_count[obj] = 1.0;
  }
  solver.addObjectDimension("count", std::move(object_count));

  // Limit number of objects moved OUT
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "object-count-limit";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "count";
  throttle_spec.definition() = ThrottlingSpecDefinition::OUT;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 20.0; // Max 20 objects move out
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(sensitive_hosts);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // object_count_throttling_end
}

void pitfall_limit_too_restrictive_bad() {
  /**
   * PITFALL: Limit too restrictive - need to drain 1TB but limit=10GB.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host_with_1TB", {"shard0", "shard1"}},
          {"other_host", {}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{{"shard0", 500.0}, {"shard1", 500.0}});

  // pitfall_restrictive_bad_start
  std::vector<std::string> hosts_with_1TB = {"host_with_1TB"};

  // BAD: Need to drain 1TB but limit=10GB
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "too-restrictive";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::OUT;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 10.0; // Only 10GB!
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(hosts_with_1TB);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // Will take 100 rounds to drain!
  // pitfall_restrictive_bad_end
}

void pitfall_limit_too_restrictive_good() {
  /**
   * SOLUTION: Calculate reasonable limit based on network capacity.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host_to_drain", {"shard0", "shard1"}},
          {"other_host", {}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{{"shard0", 500.0}, {"shard1", 500.0}});

  // pitfall_restrictive_good_start
  std::vector<std::string> hosts_to_drain = {"host_to_drain"};

  // GOOD: Calculate reasonable limit
  double network_bandwidth_gbps = 10.0; // 10 Gbps
  double migration_window_hours = 1.0;
  double max_transfer_gb =
      network_bandwidth_gbps * migration_window_hours * 3600.0 / 8.0;

  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "reasonable-limit";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::OUT;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = max_transfer_gb; // ~4500 GB
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(hosts_to_drain);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // pitfall_restrictive_good_end
}

void pitfall_wrong_definition_bad() {
  /**
   * PITFALL: Using wrong direction for intended throttling.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host_to_drain", {"shard0"}},
          {"other_host", {}},
      });
  solver.addObjectDimension(
      "data_size", std::map<std::string, double>{{"shard0", 500.0}});

  // pitfall_wrong_definition_bad_start
  std::vector<std::string> hosts_to_drain = {"host_to_drain"};

  // BAD: Want to limit draining (OUT), but using IN
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "wrong-direction";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::IN; // Wrong!

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 500.0;
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(hosts_to_drain);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // This limits moves INTO draining hosts, not OUT
  // pitfall_wrong_definition_bad_end
}

void pitfall_wrong_definition_good() {
  /**
   * SOLUTION: Use correct definition for draining.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host_to_drain", {"shard0"}},
          {"other_host", {}},
      });
  solver.addObjectDimension(
      "data_size", std::map<std::string, double>{{"shard0", 500.0}});

  // pitfall_wrong_definition_good_start
  std::vector<std::string> hosts_to_drain = {"host_to_drain"};

  // GOOD: OUT for draining
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "correct-direction";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::OUT; // Correct

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 500.0;
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(hosts_to_drain);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // pitfall_wrong_definition_good_end
}

void pitfall_filter_not_specified_bad() {
  /**
   * PITFALL: No filter means throttling applies to ALL containers.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0"}},
          {"host1", {"shard1"}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{{"shard0", 50.0}, {"shard1", 80.0}});

  // pitfall_no_filter_bad_start
  // BAD: Throttles all hosts to 100GB total
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "no-filter";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::ANY;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 100.0;
  throttle_spec.limit() = std::move(limit);

  // No filter!

  solver.addConstraint(std::move(throttle_spec));
  // Only 100GB can move across entire cluster!
  // pitfall_no_filter_bad_end
}

void pitfall_filter_not_specified_good() {
  /**
   * SOLUTION: Specify filter for target hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"target_host_1", {"shard0"}},
          {"target_host_2", {"shard1"}},
          {"other_host", {}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{{"shard0", 50.0}, {"shard1", 80.0}});

  // pitfall_no_filter_good_start
  std::vector<std::string> target_hosts = {"target_host_1", "target_host_2"};

  // GOOD: Only throttle specific hosts
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "with-filter";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::ANY;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 100.0;
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(target_hosts); // Specified
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // pitfall_no_filter_good_end
}

void pitfall_dimension_not_defined_bad() {
  /**
   * PITFALL: Dimension doesn't exist.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0"}},
          {"host1", {"shard1"}},
      });
  solver.addObjectDimension(
      "bandwidth",
      std::map<std::string, double>{{"shard0", 100.0}, {"shard1", 200.0}});

  // pitfall_no_dimension_bad_start
  // BAD: Dimension "bandwidth" not added
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "missing-dimension";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "bandwidth"; // Doesn't exist!

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 1000.0;
  throttle_spec.limit() = std::move(limit);

  solver.addConstraint(std::move(throttle_spec));
  // pitfall_no_dimension_bad_end
}

void pitfall_dimension_not_defined_good() {
  /**
   * SOLUTION: Define dimension first.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0"}},
          {"host1", {"shard1"}},
      });

  // pitfall_no_dimension_good_start
  // GOOD: Define dimension
  std::map<std::string, double> data_sizes = {
      {"shard0", 100.0}, {"shard1", 200.0}};
  solver.addObjectDimension("data_size", std::move(data_sizes));

  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "with-dimension";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 1000.0;
  throttle_spec.limit() = std::move(limit);

  solver.addConstraint(std::move(throttle_spec));
  // pitfall_no_dimension_good_end
}

void combining_with_minimize_movement() {
  /**
   * Combining with MinimizeMovementSpec - soft + hard movement limits.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0", "shard1"}},
          {"host1", {"shard2"}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"shard0", 100.0}, {"shard1", 150.0}, {"shard2", 200.0}});

  // combining_minimize_movement_start
  // Soft: Prefer minimal movement
  MinimizeMovementSpec min_move_spec;
  min_move_spec.name() = "prefer-minimal";
  min_move_spec.dimension() = "data_size";
  solver.addGoal(std::move(min_move_spec), 2.0);

  // Hard: Never exceed bandwidth limit
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "bandwidth-limit";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::ANY;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 1000.0;
  throttle_spec.limit() = std::move(limit);

  solver.addConstraint(std::move(throttle_spec));
  // combining_minimize_movement_end
}

void combining_with_to_free() {
  /**
   * Combining with ToFreeSpec - throttled draining.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"drain_host_1", {"shard0", "shard1"}},
          {"drain_host_2", {"shard2"}},
          {"host0", {}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"shard0", 100.0}, {"shard1", 150.0}, {"shard2", 200.0}});

  // combining_to_free_start
  std::vector<std::string> hosts_to_drain = {"drain_host_1", "drain_host_2"};

  // Goal: Drain these hosts
  ToFreeSpec to_free_spec;
  to_free_spec.name() = "drain-hosts";
  to_free_spec.containers() = hosts_to_drain;
  solver.addConstraint(std::move(to_free_spec));

  // Constraint: But don't exceed bandwidth limit
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "throttle-drain-rate";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::OUT;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 500.0;
  throttle_spec.limit() = std::move(limit);

  Filter filter;
  filter.itemsWhitelist() = std::move(hosts_to_drain);
  throttle_spec.filter() = std::move(filter);

  solver.addConstraint(std::move(throttle_spec));
  // combining_to_free_end
}

bool verify_throttling(
    const std::map<std::string, std::vector<std::string>>& initial_assignment,
    const std::map<std::string, std::vector<std::string>>& final_assignment,
    const std::map<std::string, double>& dimension_values,
    const std::vector<std::string>& filtered_containers,
    ThrottlingSpecDefinition definition,
    double max_limit) {
  /**
   * Verify throttling limit was respected.
   *
   * Args:
   *     initial_assignment: Initial assignment
   *     final_assignment: Final assignment
   *     dimension_values: Object dimension values
   *     filtered_containers: Filtered containers
   *     definition: ThrottlingSpecDefinition (IN/OUT/ANY)
   *     max_limit: Maximum allowed movement volume
   *
   * Returns:
   *     True if limit respected
   */
  // verification_start
  double total_volume = 0.0;

  for (const auto& container : filtered_containers) {
    std::set<std::string> initial_objects;
    if (initial_assignment.find(container) != initial_assignment.end()) {
      const auto& objects = initial_assignment.at(container);
      initial_objects.insert(objects.begin(), objects.end());
    }

    std::set<std::string> final_objects;
    if (final_assignment.find(container) != final_assignment.end()) {
      const auto& objects = final_assignment.at(container);
      final_objects.insert(objects.begin(), objects.end());
    }

    if (definition == ThrottlingSpecDefinition::OUT ||
        definition == ThrottlingSpecDefinition::ANY) {
      // Objects that left
      std::vector<std::string> left;
      std::set_difference(
          initial_objects.begin(),
          initial_objects.end(),
          final_objects.begin(),
          final_objects.end(),
          std::back_inserter(left));

      double volume_out = 0.0;
      for (const auto& obj : left) {
        auto it = dimension_values.find(obj);
        volume_out += (it != dimension_values.end()) ? it->second : 0.0;
      }
      total_volume += volume_out;
    }

    if (definition == ThrottlingSpecDefinition::IN ||
        definition == ThrottlingSpecDefinition::ANY) {
      // Objects that arrived
      std::vector<std::string> arrived;
      std::set_difference(
          final_objects.begin(),
          final_objects.end(),
          initial_objects.begin(),
          initial_objects.end(),
          std::back_inserter(arrived));

      double volume_in = 0.0;
      for (const auto& obj : arrived) {
        auto it = dimension_values.find(obj);
        volume_in += (it != dimension_values.end()) ? it->second : 0.0;
      }
      total_volume += volume_in;
    }
  }

  if (total_volume <= max_limit) {
    std::cout << "✓ Throttling respected: " << total_volume
              << " <= " << max_limit << "\n";
    return true;
  } else {
    std::cout << "✗ Throttling violated: " << total_volume << " > " << max_limit
              << "\n";
    std::cout << "   Excess: " << (total_volume - max_limit) << "\n";
    return false;
  }
  // verification_end
}

AssignmentSolution complete_example() {
  /**
   * Complete runnable example demonstrating ThrottlingSpec.
   *
   * This example shows how to use ThrottlingSpec to throttle data movement
   * while draining hosts, protecting network bandwidth.
   */
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "throttling-example", "production");

  solver.setObjectName("shard");
  solver.setContainerName("host");

  // Initial assignment with hosts to drain
  std::map<std::string, std::vector<std::string>> assignment = {
      {"host0", {"shard0", "shard1", "shard2"}},
      {"host1", {"shard3", "shard4"}},
      {"host2", {}},
      {"host3", {}},
      {"drain_host", {"shard5", "shard6", "shard7", "shard8"}},
  };
  solver.setAssignment(assignment);

  // Shard data sizes (GB)
  std::map<std::string, double> data_sizes = {
      {"shard0", 100.0},
      {"shard1", 150.0},
      {"shard2", 200.0},
      {"shard3", 120.0},
      {"shard4", 180.0},
      {"shard5", 300.0},
      {"shard6", 250.0},
      {"shard7", 200.0},
      {"shard8", 150.0},
  };
  solver.addObjectDimension("data_size", data_sizes);

  // Host capacities (GB)
  std::map<std::string, double> host_capacities = {
      {"host0", 1000.0},
      {"host1", 1000.0},
      {"host2", 1000.0},
      {"host3", 1000.0},
      {"drain_host", 1000.0},
  };
  solver.addContainerDimension("capacity", host_capacities);

  // Drain the drain_host
  ToFreeSpec to_free_spec;
  to_free_spec.name() = "drain-host";
  to_free_spec.containers() = {"drain_host"};
  solver.addConstraint(to_free_spec);

  // Throttle data movement OUT of drain_host (max 500 GB)
  ThrottlingSpec throttle_spec;
  throttle_spec.name() = "throttle-drain";
  throttle_spec.scope() = "host";
  throttle_spec.dimension() = "data_size";
  throttle_spec.definition() = ThrottlingSpecDefinition::OUT;

  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 500.0; // Max 500 GB in this round
  throttle_spec.limit() = limit;

  Filter filter;
  filter.itemsWhitelist() = {"drain_host"};
  throttle_spec.filter() = filter;

  solver.addConstraint(throttle_spec);

  // Solve
  LocalSearchSolverSpec ls_solver;
  ls_solver.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  ls_solver.solveTime() = 10;
  solver.addSolver(ls_solver);

  AssignmentSolution solution = solver.solve();

  // Print results
  std::cout << "AssignmentSolution found in "
            << *solution.problemProfile()->solveSec() << "ms\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";

  return solution;
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all Throttling examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Throttle Host Draining...\n";
  throttle_host_draining();

  std::cout << "\n3. Limit Rack Network Usage...\n";
  limit_rack_network_usage();

  std::cout << "\n4. Per Host Throttling...\n";
  per_host_throttling();

  std::cout << "\n5. Gradual Migration...\n";
  gradual_migration();

  std::cout << "\n6. Protect Production Hosts...\n";
  protect_production_hosts();

  std::cout << "\n7. Network Aware Throttling...\n";
  network_aware_throttling();

  std::cout << "\n8. Object Count Throttling...\n";
  object_count_throttling();

  std::cout << "\n9. Pitfall Limit Too Restrictive Bad...\n";
  pitfall_limit_too_restrictive_bad();

  std::cout << "\n10. Pitfall Limit Too Restrictive Good...\n";
  pitfall_limit_too_restrictive_good();

  std::cout << "\n11. Pitfall Wrong Definition Bad...\n";
  pitfall_wrong_definition_bad();

  std::cout << "\n12. Pitfall Wrong Definition Good...\n";
  pitfall_wrong_definition_good();

  std::cout << "\n13. Pitfall Filter Not Specified Bad...\n";
  pitfall_filter_not_specified_bad();

  std::cout << "\n14. Pitfall Filter Not Specified Good...\n";
  pitfall_filter_not_specified_good();

  std::cout << "\n15. Pitfall Dimension Not Defined Bad...\n";
  pitfall_dimension_not_defined_bad();

  std::cout << "\n16. Pitfall Dimension Not Defined Good...\n";
  pitfall_dimension_not_defined_good();

  std::cout << "\n17. Combining With Minimize Movement...\n";
  combining_with_minimize_movement();

  std::cout << "\n18. Combining With To Free...\n";
  combining_with_to_free();

  std::cout << "\n19. Complete Example...\n";
  complete_example();

  std::cout << "\n✓ All Throttling examples completed successfully!\n";
  return 0;
}
// example_end
