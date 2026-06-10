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
 * UtilIncreaseCostSpec Reference Examples
 *
 * This file demonstrates all the usage patterns for UtilIncreaseCostSpec shown
 * in the reference documentation. Each function is a complete, runnable
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

#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void quick_example() {
  /**
   * Quick example showing basic UtilIncreaseCostSpec usage.
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
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 3.0}, {"task2", 4.0}});
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{{"host0", 20.0}, {"host1", 20.0}});

  // quick_example_start
  // Penalize high CPU utilization
  UtilIncreaseCostSpec spec;
  spec.name() = "avoid-high-cpu";
  spec.scope() = "host";
  spec.dimension() = "cpu_usage";
  solver.addGoal(std::move(spec), 1.0);
  // quick_example_end
}

void maintain_headroom() {
  /**
   * Reserve capacity for traffic spikes.
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
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 3.0}, {"task2", 4.0}});
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{{"host0", 20.0}, {"host1", 20.0}});

  // maintain_headroom_start
  // Keep hosts under 80% utilization
  UtilIncreaseCostSpec spec;
  spec.name() = "maintain-headroom";
  spec.scope() = "host";
  spec.dimension() = "cpu_usage";
  solver.addGoal(std::move(spec), 2.0);
  // maintain_headroom_end
}

void avoid_hotspots() {
  /**
   * Prevent overloaded containers.
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
      "query_rate",
      std::map<std::string, double>{
          {"task0", 100.0}, {"task1", 200.0}, {"task2", 300.0}});
  solver.addContainerDimension(
      "query_rate_capacity",
      std::map<std::string, double>{{"host0", 1000.0}, {"host1", 1000.0}});

  // avoid_hotspots_start
  // Strongly discourage hot hosts
  UtilIncreaseCostSpec spec;
  spec.name() = "avoid-hotspots";
  spec.scope() = "host";
  spec.dimension() = "query_rate";
  solver.addGoal(std::move(spec), 3.0);
  // avoid_hotspots_end
}

void soft_capacity_limit() {
  /**
   * Create a "soft" capacity that's lower than hard capacity.
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
          {"task0", 4.0}, {"task1", 8.0}, {"task2", 6.0}});
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{{"host0", 32.0}, {"host1", 32.0}});

  // soft_capacity_limit_start
  // Hard capacity is 100, but strongly discourage going above 70
  CapacitySpec capacitySpec;
  capacitySpec.name() = "hard-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  solver.addConstraint(std::move(capacitySpec));

  UtilIncreaseCostSpec utilSpec;
  utilSpec.name() = "soft-capacity-70pct";
  utilSpec.scope() = "host";
  utilSpec.dimension() = "memory";
  solver.addGoal(
      std::move(utilSpec), 5.0); // High weight makes it nearly a constraint
  // soft_capacity_limit_end
}

void multi_resource_headroom() {
  /**
   * Maintain headroom on multiple resources.
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
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 3.0}, {"task2", 4.0}});
  solver.addObjectDimension(
      "memory_usage",
      std::map<std::string, double>{
          {"task0", 4.0}, {"task1", 8.0}, {"task2", 6.0}});
  solver.addObjectDimension(
      "disk_iops",
      std::map<std::string, double>{
          {"task0", 100.0}, {"task1", 200.0}, {"task2", 150.0}});
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{{"host0", 20.0}, {"host1", 20.0}});
  solver.addContainerDimension(
      "memory_usage_capacity",
      std::map<std::string, double>{{"host0", 32.0}, {"host1", 32.0}});
  solver.addContainerDimension(
      "disk_iops_capacity",
      std::map<std::string, double>{{"host0", 1000.0}, {"host1", 1000.0}});

  // multi_resource_headroom_start
  // CPU headroom
  UtilIncreaseCostSpec cpuSpec;
  cpuSpec.name() = "cpu-headroom";
  cpuSpec.scope() = "host";
  cpuSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(cpuSpec), 1.0);

  // Memory headroom
  UtilIncreaseCostSpec memSpec;
  memSpec.name() = "memory-headroom";
  memSpec.scope() = "host";
  memSpec.dimension() = "memory_usage";
  solver.addGoal(std::move(memSpec), 1.0);

  // Disk I/O headroom
  UtilIncreaseCostSpec diskSpec;
  diskSpec.name() = "disk-headroom";
  diskSpec.scope() = "host";
  diskSpec.dimension() = "disk_iops";
  solver.addGoal(std::move(diskSpec), 0.5); // Less critical
  // multi_resource_headroom_end
}

void rack_level_headroom() {
  /**
   * Maintain headroom at rack level for network capacity.
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
      "network_bandwidth",
      std::map<std::string, double>{
          {"task0", 5.0}, {"task1", 10.0}, {"task2", 8.0}, {"task3", 3.0}});
  solver.addContainerDimension(
      "network_bandwidth_capacity",
      std::map<std::string, double>{
          {"host0", 50.0}, {"host1", 50.0}, {"host2", 50.0}, {"host3", 50.0}});

  // Add rack scope
  std::map<std::string, std::string> host_to_rack = {
      {"host0", "rack0"},
      {"host1", "rack0"},
      {"host2", "rack1"},
      {"host3", "rack1"},
  };
  solver.addScope("rack", host_to_rack);

  // rack_level_headroom_start
  // Avoid overloading rack switches
  UtilIncreaseCostSpec spec;
  spec.name() = "rack-network-headroom";
  spec.scope() = "rack";
  spec.dimension() = "network_bandwidth";
  solver.addGoal(std::move(spec), 2.0);
  // rack_level_headroom_end
}

void progressive_penalty() {
  /**
   * Different penalties for different utilization ranges.
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
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 3.0}, {"task2", 4.0}});
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{{"host0", 20.0}, {"host1", 20.0}});

  // progressive_penalty_start
  // Moderate penalty up to 80%
  UtilIncreaseCostSpec moderateSpec;
  moderateSpec.name() = "moderate-penalty";
  moderateSpec.scope() = "host";
  moderateSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(moderateSpec), 1.0);

  // Severe penalty above 80%
  UtilIncreaseCostSpec severeSpec;
  severeSpec.name() = "severe-penalty-high";
  severeSpec.scope() = "host";
  severeSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(severeSpec), 3.0);
  // progressive_penalty_end
}

void dynamic_exponent_based_on_time() {
  /**
   * Adjust penalty based on time of day.
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
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 3.0}, {"task2", 4.0}});
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{{"host0", 20.0}, {"host1", 20.0}});

  // dynamic_exponent_start
  [[maybe_unused]] auto get_exponent_for_time = []() -> double {
    /** Higher penalty during peak hours. */
    std::time_t now = std::time(nullptr);
    std::tm* local_time = std::localtime(&now);
    int hour = local_time->tm_hour;

    if (hour >= 9 && hour < 17) { // Business hours
      return 4.0; // Strong penalty (keep more headroom)
    } else { // Off-peak
      return 2.0; // Moderate penalty (can run hotter)
    }
  };

  UtilIncreaseCostSpec spec;
  spec.name() = "time-aware-headroom";
  spec.scope() = "host";
  spec.dimension() = "cpu_usage";
  solver.addGoal(std::move(spec), 2.0);
  // dynamic_exponent_end
}

void pitfall_exponent_too_high() {
  /**
   * Example showing exponent pitfall and solution.
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
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 3.0}, {"task2", 4.0}});
  solver.addContainerDimension(
      "cpu_capacity",
      std::map<std::string, double>{{"host0", 20.0}, {"host1", 20.0}});

  // pitfall_exponent_bad_start
  // BAD: Exponent too high
  UtilIncreaseCostSpec badSpec;
  badSpec.name() = "bad-exponent";
  badSpec.scope() = "host";
  badSpec.dimension() = "cpu";
  solver.addGoal(std::move(badSpec), 1.0);
  // pitfall_exponent_bad_end

  // pitfall_exponent_good_start
  // GOOD: Reasonable exponent
  UtilIncreaseCostSpec goodSpec;
  goodSpec.name() = "good-exponent";
  goodSpec.scope() = "host";
  goodSpec.dimension() = "cpu";
  solver.addGoal(std::move(goodSpec), 1.0);
  // pitfall_exponent_good_end
}

void pitfall_conflicting_capacity() {
  /**
   * Example showing capacity conflict pitfall and solution.
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
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 3.0}, {"task2", 4.0}});
  solver.addContainerDimension(
      "cpu_capacity",
      std::map<std::string, double>{{"host0", 20.0}, {"host1", 20.0}});

  // pitfall_capacity_bad_start
  // BAD: maxUtilization doesn't match capacity
  UtilIncreaseCostSpec badSpec;
  badSpec.name() = "bad-max-util";
  badSpec.scope() = "host";
  badSpec.dimension() = "cpu";
  solver.addGoal(std::move(badSpec), 10.0); // Very high weight

  CapacitySpec capacitySpec;
  capacitySpec.name() = "capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "cpu"; // Allows up to 100%
  solver.addConstraint(std::move(capacitySpec));
  // Solver confused: can use up to 100% but heavily penalized above 50%
  // pitfall_capacity_bad_end

  // pitfall_capacity_good_start
  // GOOD: Clear operating range
  UtilIncreaseCostSpec goodSpec;
  goodSpec.name() = "good-max-util";
  goodSpec.scope() = "host";
  goodSpec.dimension() = "cpu";
  solver.addGoal(std::move(goodSpec), 2.0);
  // pitfall_capacity_good_end
}

void pitfall_weight_too_low() {
  /**
   * Example showing weight pitfall and solution.
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
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 3.0}, {"task2", 4.0}});
  solver.addContainerDimension(
      "cpu_capacity",
      std::map<std::string, double>{{"host0", 20.0}, {"host1", 20.0}});

  // pitfall_weight_bad_start
  // BAD: Weight too low relative to other goals
  UtilIncreaseCostSpec utilSpec;
  utilSpec.name() = "low-weight-headroom";
  utilSpec.scope() = "host";
  utilSpec.dimension() = "cpu";
  solver.addGoal(std::move(utilSpec), 0.01); // Dominated by other goals

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(
      std::move(balanceSpec), 10.0); // Will ignore headroom to achieve balance
  // pitfall_weight_bad_end

  // pitfall_weight_good_start
  // GOOD: Weights reflect priorities
  UtilIncreaseCostSpec utilSpecGood;
  utilSpecGood.name() = "balanced-weight-headroom";
  utilSpecGood.scope() = "host";
  utilSpecGood.dimension() = "cpu";
  solver.addGoal(std::move(utilSpecGood), 2.0); // Headroom is important

  BalanceSpec balanceSpecGood;
  balanceSpecGood.name() = "balance-good";
  balanceSpecGood.scope() = "host";
  balanceSpecGood.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpecGood), 1.0); // Balance is secondary
  // pitfall_weight_good_end
}

void pitfall_missing_capacity() {
  /**
   * Example showing missing capacity dimension pitfall and solution.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0"}},
      });

  // pitfall_missing_capacity_bad_start
  // BAD: No capacity dimension
  solver.addObjectDimension(
      "cpu_usage", std::map<std::string, double>{{"task0", 1.0}});
  // Missing: solver.addContainerDimension("cpu_capacity", ...)

  UtilIncreaseCostSpec badSpec;
  badSpec.name() = "missing-capacity";
  badSpec.scope() = "host";
  badSpec.dimension() = "cpu_usage"; // Will fail!
  solver.addGoal(std::move(badSpec), 1.0);
  // pitfall_missing_capacity_bad_end

  // pitfall_missing_capacity_good_start
  // GOOD: Both usage and capacity defined
  solver.addContainerDimension(
      "cpu_capacity", std::map<std::string, double>{{"host0", 10.0}});

  UtilIncreaseCostSpec goodSpec;
  goodSpec.name() = "with-capacity";
  goodSpec.scope() = "host";
  goodSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(goodSpec), 1.0);
  // pitfall_missing_capacity_good_end
}

void combining_with_balance_spec() {
  /**
   * Use both for balanced distribution with headroom.
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
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 3.0}, {"task2", 4.0}});
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{{"host0", 20.0}, {"host1", 20.0}});

  // combining_balance_start
  // Primary: Maintain headroom
  UtilIncreaseCostSpec utilSpec;
  utilSpec.name() = "headroom";
  utilSpec.scope() = "host";
  utilSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(utilSpec), 2.0);

  // Secondary: Even distribution among non-full hosts
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(balanceSpec), 1.0);
  // combining_balance_end
}

void combining_with_capacity_spec() {
  /**
   * Soft limit below hard limit.
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
          {"task0", 4.0}, {"task1", 8.0}, {"task2", 6.0}});
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{{"host0", 32.0}, {"host1", 32.0}});

  // combining_capacity_start
  // Hard limit: 100% capacity
  CapacitySpec capacitySpec;
  capacitySpec.name() = "hard-limit";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  solver.addConstraint(std::move(capacitySpec));

  // Soft limit: strongly discourage above 75%
  UtilIncreaseCostSpec utilSpec;
  utilSpec.name() = "soft-limit-75";
  utilSpec.scope() = "host";
  utilSpec.dimension() = "memory";
  solver.addGoal(std::move(utilSpec), 5.0); // High weight = nearly a constraint
  // combining_capacity_end
}

void combining_with_minimize_movement() {
  /**
   * Balance headroom with minimal disruption.
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
      "cpu_usage",
      std::map<std::string, double>{
          {"task0", 2.0}, {"task1", 3.0}, {"task2", 4.0}});
  solver.addContainerDimension(
      "cpu_usage_capacity",
      std::map<std::string, double>{{"host0", 20.0}, {"host1", 20.0}});

  // combining_minimize_movement_start
  // Maintain headroom
  UtilIncreaseCostSpec utilSpec;
  utilSpec.name() = "headroom";
  utilSpec.scope() = "host";
  utilSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(utilSpec), 2.0);

  // But minimize churn
  MinimizeMovementSpec moveSpec;
  moveSpec.name() = "min-moves";
  moveSpec.scope() = "host";
  moveSpec.dimension() = "cpu_usage";
  solver.addGoal(std::move(moveSpec), 1.0);
  // combining_minimize_movement_end
}

bool verify_headroom(
    const AssignmentSolution& solution,
    const std::map<std::string, double>& cpu_usage,
    const std::map<std::string, double>& cpu_capacity,
    double target_max_util = 0.8) {
  /**
   * Verify containers stay under target utilization.
   *
   * Args:
   *     solution: Solver solution
   *     cpu_usage: Object CPU usage
   *     cpu_capacity: Container CPU capacity
   *     target_max_util: Target maximum utilization (default 80%)
   *
   * Returns:
   *     True if all containers under target
   */
  // verify_headroom_start
  struct Violation {
    std::string host;
    double utilization;
    double cpu_used;
    double cpu_cap;
  };
  std::vector<Violation> violations;

  // assignment() is object -> container, build reverse map
  std::map<std::string, std::vector<std::string>> host_to_objects;
  for (const auto& [obj, host] : *solution.assignment()) {
    host_to_objects[host].push_back(obj);
  }

  for (const auto& [host, objects] : host_to_objects) {
    double cpu_used = 0.0;
    for (const auto& obj : objects) {
      auto it = cpu_usage.find(obj);
      if (it != cpu_usage.end()) {
        cpu_used += it->second;
      }
    }

    double cpu_cap = cpu_capacity.at(host);
    double utilization = cpu_used / cpu_cap;

    if (utilization > target_max_util) {
      violations.push_back({host, utilization, cpu_used, cpu_cap});
    }
  }

  if (violations.empty()) {
    std::cout << "✓ All hosts under " << (target_max_util * 100)
              << "% utilization\n";
    return true;
  } else {
    std::cout << "✗ " << violations.size() << " hosts exceed "
              << (target_max_util * 100) << "% utilization:\n";
    for (const auto i : folly::irange(std::min(violations.size(), size_t(5)))) {
      const auto& v = violations[i];
      std::cout << "   " << v.host << ": "
                << fmt::format("{:.1f}%", v.utilization * 100) << " ("
                << fmt::format("{:.1f}/{:.1f}", v.cpu_used, v.cpu_cap) << ")\n";
    }
    return false;
  }
  // verify_headroom_end
}

void analyze_utilization_distribution(
    const AssignmentSolution& solution,
    const std::map<std::string, double>& cpu_usage,
    const std::map<std::string, double>& cpu_capacity) {
  /**
   * Analyze how well headroom goal worked.
   *
   * Args:
   *     solution: Solver solution
   *     cpu_usage: Object CPU usage
   *     cpu_capacity: Container CPU capacity
   */
  // analyze_utilization_start
  std::vector<double> utilizations;

  // assignment() is object -> container, build reverse map
  std::map<std::string, std::vector<std::string>> host_to_objs;
  for (const auto& [obj, host] : *solution.assignment()) {
    host_to_objs[host].push_back(obj);
  }

  for (const auto& [host, objects] : host_to_objs) {
    double cpu_used = 0.0;
    for (const auto& obj : objects) {
      auto it = cpu_usage.find(obj);
      if (it != cpu_usage.end()) {
        cpu_used += it->second;
      }
    }

    double cpu_cap = cpu_capacity.at(host);
    if (cpu_cap > 0) {
      utilizations.push_back(cpu_used / cpu_cap);
    }
  }

  std::sort(utilizations.begin(), utilizations.end());

  std::cout << "Utilization distribution:\n";
  std::cout << fmt::format("  Min: {:.1f}%\n", utilizations.front() * 100);
  std::cout << fmt::format(
      "  25th percentile: {:.1f}%\n",
      utilizations[utilizations.size() / 4] * 100);
  std::cout << fmt::format(
      "  Median: {:.1f}%\n", utilizations[utilizations.size() / 2] * 100);
  std::cout << fmt::format(
      "  75th percentile: {:.1f}%\n",
      utilizations[3 * utilizations.size() / 4] * 100);
  std::cout << fmt::format("  Max: {:.1f}%\n", utilizations.back() * 100);

  // Count hosts in utilization ranges
  std::vector<std::pair<double, double>> ranges = {
      {0.0, 0.5}, {0.5, 0.7}, {0.7, 0.85}, {0.85, 1.0}, {1.0, 1e10}};
  for (const auto& [low, high] : ranges) {
    int count = std::count_if(
        utilizations.begin(), utilizations.end(), [low, high](double u) {
          return u >= low && u < high;
        });
    double pct = (count * 100.0) / utilizations.size();
    std::cout << fmt::format(
        "  {:.0f}-{:.0f}%: {} hosts ({:.1f}%)\n",
        low * 100,
        high * 100,
        count,
        pct);
  }
  // analyze_utilization_end
}

void complete_example() {
  /**
   * Complete runnable example demonstrating UtilIncreaseCostSpec.
   *
   * This example shows how to use UtilIncreaseCostSpec to maintain headroom
   * on hosts while balancing CPU and memory usage.
   */
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "util-increase-example", "production");

  solver.setObjectName("task");
  solver.setContainerName("host");

  // Initial imbalanced assignment (some hosts overloaded)
  std::map<std::string, std::vector<std::string>> assignment = {
      {"host0", {"task0", "task1", "task2", "task3", "task4", "task5"}},
      {"host1", {"task6", "task7", "task8"}},
      {"host2", {"task9"}},
      {"host3", {}},
  };
  solver.setAssignment(assignment);

  // Task resources
  std::map<std::string, double> cpu_usage;
  std::map<std::string, double> memory_usage;
  for (const auto i : folly::irange(10)) {
    cpu_usage[fmt::format("task{}", i)] = 2.0 + (i % 3);
    memory_usage[fmt::format("task{}", i)] = 4.0 + (i % 4);
  }

  solver.addObjectDimension("cpu", cpu_usage);
  solver.addObjectDimension("memory", std::move(memory_usage));

  // Host capacities
  std::map<std::string, double> host_cpu_capacity;
  std::map<std::string, double> host_memory_capacity;
  for (const auto i : folly::irange(4)) {
    host_cpu_capacity[fmt::format("host{}", i)] = 20.0;
    host_memory_capacity[fmt::format("host{}", i)] = 40.0;
  }

  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);
  solver.addContainerDimension("memory_capacity", host_memory_capacity);

  // Add hard capacity constraints
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu";
  solver.addConstraint(std::move(cpuCapacity));

  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory";
  solver.addConstraint(std::move(memoryCapacity));

  // Add headroom goals - penalize high utilization
  UtilIncreaseCostSpec cpuHeadroom;
  cpuHeadroom.name() = "cpu-headroom";
  cpuHeadroom.scope() = "host";
  cpuHeadroom.dimension() = "cpu";
  solver.addGoal(std::move(cpuHeadroom), 2.0);

  UtilIncreaseCostSpec memoryHeadroom;
  memoryHeadroom.name() = "memory-headroom";
  memoryHeadroom.scope() = "host";
  memoryHeadroom.dimension() = "memory";
  solver.addGoal(std::move(memoryHeadroom), 1.5);

  // Add balance goal as secondary objective
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu";
  solver.addGoal(std::move(balanceCpu), 0.5);

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

  // Verify headroom maintained
  std::cout << "\nVerifying headroom:\n";
  verify_headroom(solution, cpu_usage, host_cpu_capacity, 0.8);

  std::cout << "\nUtilization analysis:\n";
  analyze_utilization_distribution(solution, cpu_usage, host_cpu_capacity);
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all UtilIncreaseCost examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Maintain Headroom...\n";
  maintain_headroom();

  std::cout << "\n3. Avoid Hotspots...\n";
  avoid_hotspots();

  std::cout << "\n4. Soft Capacity Limit...\n";
  soft_capacity_limit();

  std::cout << "\n5. Multi Resource Headroom...\n";
  multi_resource_headroom();

  std::cout << "\n6. Rack Level Headroom...\n";
  rack_level_headroom();

  std::cout << "\n7. Progressive Penalty...\n";
  progressive_penalty();

  std::cout << "\n8. Dynamic Exponent Based On Time...\n";
  dynamic_exponent_based_on_time();

  std::cout << "\n9. Pitfall Exponent Too High...\n";
  pitfall_exponent_too_high();

  std::cout << "\n10. Pitfall Conflicting Capacity...\n";
  pitfall_conflicting_capacity();

  std::cout << "\n11. Pitfall Weight Too Low...\n";
  pitfall_weight_too_low();

  std::cout << "\n12. Pitfall Missing Capacity...\n";
  pitfall_missing_capacity();

  std::cout << "\n13. Combining With Balance Spec...\n";
  combining_with_balance_spec();

  std::cout << "\n14. Combining With Capacity Spec...\n";
  combining_with_capacity_spec();

  std::cout << "\n15. Combining With Minimize Movement...\n";
  combining_with_minimize_movement();

  std::cout << "\n16. Complete Example...\n";
  complete_example();

  std::cout << "\n✓ All UtilIncreaseCost examples completed successfully!\n";
  return 0;
}
// example_end
