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
 * GroupMoveLimitSpec Reference Examples
 *
 * This file demonstrates all the usage patterns for GroupMoveLimitSpec shown in
 * the reference documentation. Each function is a complete, runnable example.
 */

// example_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

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
   * Quick example showing basic GroupMoveLimitSpec usage.
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
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
          {"svc_b", {"task2", "task3"}},
      });

  // quick_example_start
  // Limit moves per service
  GroupMoveLimitSpec spec;
  spec.name() = "limit-moves-per-service";
  spec.partitionName() = "service";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 5.0; // Max 5 objects move per service
  spec.limit() = limit;

  solver.addConstraint(spec);
  // quick_example_end
}

void uniform_move_limit() {
  /**
   * Uniform move limit per service.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"frontend_0", "frontend_1", "api_0", "api_1"}},
          {"host1",
           {"frontend_2", "api_2", "api_3", "db_shard_0", "db_shard_1"}},
      });

  // uniform_move_limit_start
  // Define service partition
  std::map<std::string, std::vector<std::string>> service_partition = {
      {"web_frontend", {"frontend_0", "frontend_1", "frontend_2"}},
      {"api_backend", {"api_0", "api_1", "api_2", "api_3"}},
      {"database", {"db_shard_0", "db_shard_1"}},
  };
  solver.addPartition("service", std::move(service_partition));

  // Limit: Max 3 objects move per service
  GroupMoveLimitSpec spec;
  spec.name() = "uniform-move-limit";
  spec.partitionName() = "service";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 3.0;
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // uniform_move_limit_end
}

void per_service_custom_limits() {
  /**
   * Different limits for different services.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"inst0", "inst1", "inst2"}},
          {"host1", {"inst3", "inst4", "inst5"}},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"critical_database", {"inst0", "inst1"}},
          {"important_api", {"inst2", "inst3"}},
          {"batch_processing", {"inst4", "inst5"}},
      });

  // per_service_custom_limits_start
  // Critical services get lower limits
  std::map<std::string, double> service_limits = {
      {"critical_database", 1.0}, // Only 1 shard at a time
      {"important_api", 3.0}, // Max 3 instances
      {"batch_processing", 10.0}, // Can move more
  };

  GroupMoveLimitSpec spec;
  spec.name() = "per-service-limits";
  spec.partitionName() = "service";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.groupLimits() = std::move(service_limits);
  limit.globalLimit() = 5.0; // Default for services not in groupLimits
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // per_service_custom_limits_end
}

void weighted_move_limit_by_size() {
  /**
   * Limit based on data volume moved, not object count.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0", "shard1"}},
          {"host1", {"shard2", "shard3"}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"shard0", 100.0},
          {"shard1", 200.0},
          {"shard2", 150.0},
          {"shard3", 250.0},
      });
  solver.addPartition(
      "database",
      std::map<std::string, std::vector<std::string>>{
          {"db_a", {"shard0", "shard1"}},
          {"db_b", {"shard2", "shard3"}},
      });

  // weighted_move_limit_by_size_start
  // Use data_size dimension instead of count
  GroupMoveLimitSpec spec;
  spec.name() = "data-volume-limit";
  spec.partitionName() = "database";
  spec.dimension() = "data_size"; // GB

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1000.0; // Max 1TB moved per database
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // weighted_move_limit_by_size_end
}

void directional_move_limits() {
  /**
   * Only count moves FROM certain hosts (e.g., draining).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_host_1", {"inst0", "inst1"}},
          {"old_host_2", {"inst2", "inst3"}},
          {"new_host_1", {"inst4", "inst5"}},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"inst0", "inst1", "inst2"}},
          {"svc_b", {"inst3", "inst4", "inst5"}},
      });

  // directional_move_limits_start
  // Only limit moves FROM hosts being drained
  std::vector<std::string> draining_hosts = {"old_host_1", "old_host_2"};

  GroupMoveLimitSpec spec;
  spec.name() = "drain-move-limit";
  spec.partitionName() = "service";

  Filter filter;
  filter.itemsWhitelist() =
      std::move(draining_hosts); // Only count moves from these hosts
  spec.sourceScopeItemsAffectingLimitFilter() = std::move(filter);

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 5.0;
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // directional_move_limits_end
}

void multi_round_gradual_rebalancing() {
  /**
   * Gradually increase move limit over rounds.
   */
  // multi_round_gradual_rebalancing_start
  [[maybe_unused]] auto multi_round_with_increasing_limit = [](int rounds = 5) {
    /**
     * Gradually increase move limit over rounds.
     */
    for (const auto round_num : folly::irange(rounds)) {
      auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
      ProblemSolver solver(executor, "example", "test");

      // Increase limit each round: 5, 10, 15, 20, 25
      double limit_value = 5.0 * (round_num + 1);

      GroupMoveLimitSpec spec;
      spec.name() = "gradual-move-limit";
      spec.partitionName() = "service";

      Limit limit;
      limit.type() = LimitType::ABSOLUTE;
      limit.globalLimit() = limit_value;
      spec.limit() = limit;

      solver.addConstraint(spec);

      // auto solution = solver.solve();
      // apply_moves(solution);
      // std::cout << "Round " << (round_num + 1) << ": Limit " << limit_value
      //           << ", Moved " << count_moves(solution) << std::endl;
    }
  };
  // multi_round_gradual_rebalancing_end
}

void replica_group_move_limits() {
  /**
   * Limit moves per replica group for safety.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("replica");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard_0_replica_0", "shard_0_replica_1"}},
          {"host1", {"shard_0_replica_2", "shard_1_replica_0"}},
          {"host2", {"shard_1_replica_1", "shard_1_replica_2"}},
      });

  // replica_group_move_limits_start
  // Partition by replica group
  std::map<std::string, std::vector<std::string>> replica_groups = {
      {"shard_0_replicas",
       {"shard_0_replica_0", "shard_0_replica_1", "shard_0_replica_2"}},
      {"shard_1_replicas",
       {"shard_1_replica_0", "shard_1_replica_1", "shard_1_replica_2"}},
  };
  solver.addPartition("replica_group", std::move(replica_groups));

  // Don't move more than 1 replica per shard at a time
  GroupMoveLimitSpec spec;
  spec.name() = "replica-safety";
  spec.partitionName() = "replica_group";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1.0; // Max 1 replica move per shard
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // replica_group_move_limits_end
}

void combined_source_destination_filters() {
  /**
   * Count moves only between specific hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_dc_host_1", {"inst0", "inst1"}},
          {"old_dc_host_2", {"inst2", "inst3"}},
          {"new_dc_host_1", {"inst4"}},
          {"new_dc_host_2", {"inst5"}},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"inst0", "inst1", "inst2"}},
          {"svc_b", {"inst3", "inst4", "inst5"}},
      });

  // combined_source_destination_filters_start
  // Only count moves FROM old datacenter TO new datacenter
  std::vector<std::string> old_dc_hosts = {"old_dc_host_1", "old_dc_host_2"};
  std::vector<std::string> new_dc_hosts = {"new_dc_host_1", "new_dc_host_2"};

  GroupMoveLimitSpec spec;
  spec.name() = "dc-migration-limit";
  spec.partitionName() = "service";

  Filter source_filter;
  source_filter.itemsWhitelist() = std::move(old_dc_hosts);
  spec.sourceScopeItemsAffectingLimitFilter() = std::move(source_filter);

  Filter dest_filter;
  dest_filter.itemsWhitelist() = std::move(new_dc_hosts);
  spec.destinationScopeItemsAffectingLimitFilter() = std::move(dest_filter);

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 10.0;
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // combined_source_destination_filters_end
}

void pitfall_restrictive_bad() {
  /**
   * Pitfall: Limit too restrictive (BAD example).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  // pitfall_restrictive_bad_start
  // BAD: Service has 100 imbalanced objects, but limit=1
  GroupMoveLimitSpec spec;
  spec.name() = "too-restrictive";
  spec.partitionName() = "large_service";

  Limit limit;
  limit.globalLimit() = 1.0; // Too restrictive!
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // Rebalancing will take 100 rounds!
  // pitfall_restrictive_bad_end
}

void pitfall_restrictive_good() {
  /**
   * Pitfall: Limit too restrictive (GOOD example).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  // pitfall_restrictive_good_start
  // GOOD: Allow 10-20% of objects to move per round
  int service_size = 100;
  double reasonable_limit = service_size * 0.15; // 15 objects

  GroupMoveLimitSpec spec;
  spec.name() = "reasonable-limit";
  spec.partitionName() = "large_service";

  Limit limit;
  limit.globalLimit() = reasonable_limit;
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // pitfall_restrictive_good_end
}

void pitfall_forgot_partition_bad() {
  /**
   * Pitfall: Partition not defined (BAD example).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  // pitfall_forgot_partition_bad_start
  // BAD: Partition "service" not added
  GroupMoveLimitSpec spec;
  spec.name() = "forgot-partition";
  spec.partitionName() = "service"; // Error: partition doesn't exist!

  Limit limit;
  limit.globalLimit() = 5.0;
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // pitfall_forgot_partition_bad_end
}

void pitfall_forgot_partition_good() {
  /**
   * Pitfall: Partition not defined (GOOD example).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  // pitfall_forgot_partition_good_start
  // GOOD: Define partition first
  std::map<std::string, std::vector<std::string>> service_partition = {
      {"service_a", {"obj1", "obj2"}},
      {"service_b", {"obj3"}},
  };
  solver.addPartition("service", std::move(service_partition));

  GroupMoveLimitSpec spec;
  spec.name() = "with-partition";
  spec.partitionName() = "service";

  Limit limit;
  limit.globalLimit() = 5.0;
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // pitfall_forgot_partition_good_end
}

void pitfall_wrong_dimension_bad() {
  /**
   * Pitfall: Dimension doesn't exist (BAD example).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  // pitfall_wrong_dimension_bad_start
  // BAD: Using dimension "priority" but objects don't have it
  GroupMoveLimitSpec spec;
  spec.name() = "wrong-dimension";
  spec.partitionName() = "service";
  spec.dimension() = "priority"; // Doesn't exist!

  Limit limit;
  limit.globalLimit() = 10.0;
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // pitfall_wrong_dimension_bad_end
}

void pitfall_wrong_dimension_good() {
  /**
   * Pitfall: Dimension doesn't exist (GOOD example).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  // pitfall_wrong_dimension_good_start
  // GOOD: Use existing dimension or default
  std::map<std::string, double> data_sizes = {
      {"shard_0", 100.0},
      {"shard_1", 200.0},
  };
  solver.addObjectDimension("data_size", std::move(data_sizes));

  GroupMoveLimitSpec spec;
  spec.name() = "valid-dimension";
  spec.partitionName() = "service";
  spec.dimension() = "data_size"; // Exists!

  Limit limit;
  limit.globalLimit() = 1000.0;
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // pitfall_wrong_dimension_good_end
}

void combining_minimize_movement() {
  /**
   * Soft limit (goal) + hard limit (constraint).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  // combining_minimize_movement_start
  // Soft: Prefer minimal movement
  MinimizeMovementSpec minimize_spec;
  minimize_spec.name() = "prefer-minimal-moves";
  minimize_spec.scope() = "host";
  minimize_spec.dimension() = "data_size";
  solver.addGoal(std::move(minimize_spec), 1.0);

  // Hard: Never exceed per-service limit
  GroupMoveLimitSpec limit_spec;
  limit_spec.name() = "enforce-move-limit";
  limit_spec.partitionName() = "service";
  limit_spec.dimension() = "data_size";

  Limit limit;
  limit.globalLimit() = 500.0; // Hard limit
  limit_spec.limit() = std::move(limit);

  solver.addConstraint(std::move(limit_spec));
  // combining_minimize_movement_end
}

void combining_avoid_moving() {
  /**
   * Pin + limit moves for others.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  // combining_avoid_moving_start
  // Pin critical objects (don't move at all)
  std::vector<std::string> critical_objects = {
      "critical_db_0", "critical_api_0"};

  AvoidMovingSpec avoid_spec;
  avoid_spec.name() = "pin-critical";
  avoid_spec.objects() = std::move(critical_objects);
  solver.addConstraint(std::move(avoid_spec));

  // Limit moves for non-critical
  GroupMoveLimitSpec limit_spec;
  limit_spec.name() = "limit-non-critical";
  limit_spec.partitionName() = "service";

  Limit limit;
  limit.globalLimit() = 10.0;
  limit_spec.limit() = std::move(limit);

  solver.addConstraint(std::move(limit_spec));
  // combining_avoid_moving_end
}

void verification_example() {
  /**
   * Verify move limits respected.
   */
  // verification_start
  [[maybe_unused]] auto verify_group_move_limits =
      [](const std::map<std::string, std::vector<std::string>>&
             initial_assignment,
         const std::map<std::string, std::vector<std::string>>&
             final_assignment,
         const std::map<std::string, std::vector<std::string>>& partition,
         int max_moves_per_group) -> bool {
    /**
     * Verify group move limits were respected.
     *
     * Args:
     *     initial_assignment: Initial assignment
     *     final_assignment: Final assignment
     *     partition: Partition dict (group -> objects)
     *     max_moves_per_group: Maximum allowed moves per group
     *
     * Returns:
     *     True if all groups within limit
     */
    auto find_host =
        [](const std::string& obj,
           const std::map<std::string, std::vector<std::string>>& assignment)
        -> std::string {
      for (const auto& [host, objects] : assignment) {
        if (std::find(objects.begin(), objects.end(), obj) != objects.end()) {
          return host;
        }
      }
      return "";
    };

    std::vector<std::map<std::string, std::string>> violations;

    for (const auto& [group, objects] : partition) {
      // Count moves for this group
      int moves = 0;
      for (const auto& obj : objects) {
        std::string initial_host = find_host(obj, initial_assignment);
        std::string final_host = find_host(obj, final_assignment);
        if (initial_host != final_host) {
          moves++;
        }
      }

      if (moves > max_moves_per_group) {
        violations.push_back({
            {"group", group},
            {"moves", std::to_string(moves)},
            {"limit", std::to_string(max_moves_per_group)},
        });
      }
    }

    if (violations.empty()) {
      std::cout << "✅ All groups within move limit (≤" << max_moves_per_group
                << " moves)" << std::endl;
      return true;
    } else {
      std::cout << "❌ " << violations.size()
                << " groups exceeded move limit:" << std::endl;
      for (const auto i :
           folly::irange(std::min(violations.size(), size_t(5)))) {
        const auto& v = violations[i];
        std::cout << "   " << v.at("group") << ": " << v.at("moves")
                  << " moves (limit: " << v.at("limit") << ")" << std::endl;
      }
      return false;
    }
  };
  // verification_end
}

void move_count_analysis() {
  /**
   * Analyze moves per group.
   */
  // move_count_analysis_start
  [[maybe_unused]] auto analyze_moves_per_group =
      [](const std::map<std::string, std::vector<std::string>>& initial,
         const std::map<std::string, std::vector<std::string>>& final,
         const std::map<std::string, std::vector<std::string>>& partition)
      -> std::map<std::string, int> {
    /**
     * Analyze how many moves occurred per group.
     *
     * Args:
     *     initial: Initial assignment
     *     final: Final assignment
     *     partition: Partition dict
     *
     * Returns:
     *     Map of group -> move count
     */
    auto find_host =
        [](const std::string& obj,
           const std::map<std::string, std::vector<std::string>>& assignment)
        -> std::string {
      for (const auto& [host, objects] : assignment) {
        if (std::find(objects.begin(), objects.end(), obj) != objects.end()) {
          return host;
        }
      }
      return "";
    };

    std::map<std::string, int> move_counts;

    for (const auto& [group, objects] : partition) {
      int moves = 0;
      for (const auto& obj : objects) {
        std::string initial_host = find_host(obj, initial);
        std::string final_host = find_host(obj, final);
        if (initial_host != final_host) {
          moves++;
        }
      }
      move_counts[group] = moves;
    }

    std::cout << "Moves per group:" << std::endl;
    for (const auto& [group, count] : move_counts) {
      std::cout << "  " << group << ": " << count << " moves" << std::endl;
    }

    return move_counts;
  };
  // move_count_analysis_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all GroupMoveLimitSpec examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "2. Uniform Move Limit...\n";
  uniform_move_limit();

  std::cout << "3. Per-Service Custom Limits...\n";
  per_service_custom_limits();

  std::cout << "4. Weighted Move Limit by Size...\n";
  weighted_move_limit_by_size();

  std::cout << "5. Directional Move Limits...\n";
  directional_move_limits();

  std::cout << "6. Multi-Round Gradual Rebalancing...\n";
  multi_round_gradual_rebalancing();

  std::cout << "7. Replica Group Move Limits...\n";
  replica_group_move_limits();

  std::cout << "8. Combined Source/Destination Filters...\n";
  combined_source_destination_filters();

  std::cout << "\n✓ All GroupMoveLimitSpec examples completed successfully!\n";
  return 0;
}
// example_end
