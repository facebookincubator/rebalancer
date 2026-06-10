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
 * Movement Specs Reference Examples
 *
 * This file demonstrates all the usage patterns for movement-related specs
 * (MinimizeMovementSpec, MovesInProgressSpec, AvoidMovingSpec) shown in the
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

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

// ============================================================================
// MinimizeMovementSpec Examples
// ============================================================================

void minimize_movement_quick_example() {
  /**
   * Quick example showing basic MinimizeMovementSpec usage.
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
          {"shard0", 10.0}, {"shard1", 15.0}, {"shard2", 20.0}});

  // minimize_movement_quick_example_start
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-moves";
  minMove.dimension() = "data_size";
  solver.addGoal(std::move(minMove), 0.1); // Lower priority than balance
  // minimize_movement_quick_example_end
}

void minimize_data_movement() {
  /**
   * Minimize moving large objects while rebalancing.
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
      "data_size_gb",
      std::map<std::string, double>{
          {"shard0", 10.0}, {"shard1", 15.0}, {"shard2", 20.0}});

  // minimize_data_movement_start
  // Prefer moving small objects over large ones
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-data-movement";
  minMove.dimension() = "data_size_gb";
  solver.addGoal(std::move(minMove), 0.2);
  // minimize_data_movement_end
}

void stability_with_allowance() {
  /**
   * Allow some movement, but penalize beyond a threshold.
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
      "data_size_gb",
      std::map<std::string, double>{
          {"shard0", 10.0}, {"shard1", 15.0}, {"shard2", 20.0}});

  // stability_with_allowance_start
  // Allow up to 10GB of movement without penalty
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-movement";
  minMove.dimension() = "data_size_gb";
  minMove.doNotNormalize() = true; // Required when using allowance
  minMove.allowance() = 10.0; // Free movement budget
  solver.addGoal(std::move(minMove), 0.5);
  // stability_with_allowance_end
}

void minimize_move_count() {
  /**
   * Minimize number of objects moved (use count dimension).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  const std::vector<std::string> all_objects = {"shard0", "shard1", "shard2"};

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0", "shard1"}},
          {"host1", {"shard2"}},
      });

  // minimize_move_count_start
  // Add a count dimension first
  std::map<std::string, double> count_dim;
  for (const auto& obj : all_objects) {
    count_dim[obj] = 1.0;
  }
  solver.addObjectDimension("count", std::move(count_dim));

  // Minimize number of moves
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-move-count";
  minMove.dimension() = "count";
  solver.addGoal(std::move(minMove), 0.1);
  // minimize_move_count_end
}

void hierarchical_movement_minimization() {
  /**
   * Prefer moving within racks over across racks.
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
          {"host3", {"shard3"}},
      });

  // Add rack scope
  const std::map<std::string, std::string> host_to_rack = {
      {"host0", "rack0"},
      {"host1", "rack0"},
      {"host2", "rack1"},
      {"host3", "rack1"},
  };
  solver.addScope("rack", host_to_rack);

  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"shard0", 10.0},
          {"shard1", 15.0},
          {"shard2", 20.0},
          {"shard3", 12.0}});

  // hierarchical_movement_start
  // Higher penalty for cross-rack movement
  MinimizeMovementSpec minRackMove;
  minRackMove.name() = "minimize-rack-movement";
  minRackMove.dimension() = "data_size";
  solver.addGoal(std::move(minRackMove), 0.3);

  // Lower penalty for intra-rack movement
  MinimizeMovementSpec minHostMove;
  minHostMove.name() = "minimize-host-movement";
  minHostMove.dimension() = "data_size";
  solver.addGoal(std::move(minHostMove), 0.1);
  // hierarchical_movement_end
}

void disable_magic_scaling() {
  /**
   * Use raw movement values without heuristic scaling.
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
          {"shard0", 10.0}, {"shard1", 15.0}, {"shard2", 20.0}});

  // disable_magic_scaling_start
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-movement";
  minMove.dimension() = "data_size";
  minMove.magicScaling() = false; // Use exact dimension values
  solver.addGoal(std::move(minMove), 0.2);
  // disable_magic_scaling_end
}

void minimize_movement_with_balance() {
  /**
   * Combining MinimizeMovementSpec with BalanceSpec.
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
      "cpu",
      std::map<std::string, double>{
          {"shard0", 2.0}, {"shard1", 3.0}, {"shard2", 4.0}});

  // minimize_with_balance_start
  // Primary: Achieve balance
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;
  solver.addGoal(std::move(balanceCpu), 1.0);

  // Secondary: Minimize disruption
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-moves";
  minMove.dimension() = "cpu";
  solver.addGoal(std::move(minMove), 0.2);
  // minimize_with_balance_end
}

void minimize_movement_with_capacity() {
  /**
   * Combining MinimizeMovementSpec with CapacitySpec.
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
      "cpu",
      std::map<std::string, double>{
          {"shard0", 2.0}, {"shard1", 3.0}, {"shard2", 4.0}});

  // minimize_with_capacity_start
  // Constraint: Don't exceed capacity
  CapacitySpec cpuCap;
  cpuCap.name() = "cpu-capacity";
  cpuCap.scope() = "host";
  cpuCap.dimension() = "cpu";
  solver.addConstraint(std::move(cpuCap));

  // Goal: Minimize moves while respecting capacity
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-moves";
  minMove.dimension() = "cpu";
  solver.addGoal(std::move(minMove), 1.0);
  // minimize_with_capacity_end
}

// ============================================================================
// MovesInProgressSpec Examples
// ============================================================================

void moves_in_progress_quick_example() {
  /**
   * Quick example showing basic MovesInProgressSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard_42", "shard_17"}},
          {"host3", {}},
          {"host5", {}},
      });

  // moves_in_progress_quick_example_start
  // Shard migrations currently happening
  std::vector<MoveInProgress> ongoing_moves;

  MoveInProgress move1;
  move1.objName() = "shard_42";
  move1.toContainer() = "host5"; // Moving to host5
  ongoing_moves.push_back(move1);

  MoveInProgress move2;
  move2.objName() = "shard_17";
  move2.toContainer() = "host3";
  ongoing_moves.push_back(move2);

  MovesInProgressSpec movesSpec;
  movesSpec.name() = "ongoing-migrations";
  movesSpec.moves() = std::move(ongoing_moves);
  solver.addConstraint(std::move(movesSpec));
  // moves_in_progress_quick_example_end
}

void basic_production_rebalancing() {
  /**
   * Track ongoing moves between rebalancing rounds.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  [[maybe_unused]] auto get_ongoing_moves_from_db = []() {
    // Mock function to get ongoing moves
    std::vector<std::map<std::string, std::string>> moves = {
        {{"object", "shard_1"}, {"destination", "host2"}},
        {{"object", "shard_3"}, {"destination", "host4"}},
    };
    return moves;
  };

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard_1", "shard_3"}},
          {"host2", {}},
          {"host4", {}},
      });

  // basic_production_rebalancing_start
  // Track moves initiated in previous round
  auto ongoing_moves_data = get_ongoing_moves_from_db();

  std::vector<MoveInProgress> moves_in_progress;
  for (const auto& move : ongoing_moves_data) {
    MoveInProgress mip;
    mip.objName() = move.at("object");
    mip.toContainer() = move.at("destination");
    moves_in_progress.push_back(mip);
  }

  MovesInProgressSpec movesSpec;
  movesSpec.name() = "ongoing-moves";
  movesSpec.moves() = std::move(moves_in_progress);
  solver.addConstraint(std::move(movesSpec));

  // auto solution = solver.solve();
  // New solution won't touch objects currently being moved
  // basic_production_rebalancing_end
}

void multi_round_migration() {
  /**
   * Track moves across multiple rebalancing rounds.
   */
  [[maybe_unused]] auto apply_moves_async = [](const AssignmentSolution&) {
    // Mock function to apply moves
    return std::vector<MoveInProgress>();
  };

  // multi_round_migration_start
  [[maybe_unused]] auto multi_round_rebalancing = [](int rounds = 5) {
    /**
     * Rebalance over multiple rounds, tracking ongoing moves.
     */
    std::vector<MoveInProgress> ongoing_moves;

    for (const auto round_num : folly::irange(rounds)) {
      auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
      ProblemSolver solver(executor, "example", "test");
      solver.setObjectName("shard");
      solver.setContainerName("host");

      // Mark moves from previous round as in-progress
      if (!ongoing_moves.empty()) {
        MovesInProgressSpec movesSpec;
        movesSpec.name() = "ongoing-moves";
        movesSpec.moves() = ongoing_moves;
        solver.addConstraint(movesSpec);
      }

      // auto solution = solver.solve();

      // Apply moves
      // ongoing_moves = apply_moves_async(solution);

      // ongoing_moves now contains moves that haven't finished yet
      std::cout << fmt::format(
          "Round {}: {} moves in progress\n",
          round_num + 1,
          ongoing_moves.size());

      // Next round will respect these ongoing moves
    }
  };
  // multi_round_migration_end
}

void separate_migration_and_rebalancing() {
  /**
   * Use for systems with background migration separate from rebalancing.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard_10", "shard_20"}},
          {"new_host", {}},
      });

  // separate_migration_start
  // Background process is moving shards for maintenance
  std::vector<MoveInProgress> background_migrations;

  MoveInProgress move1;
  move1.objName() = "shard_10";
  move1.toContainer() = "new_host";
  background_migrations.push_back(move1);

  MoveInProgress move2;
  move2.objName() = "shard_20";
  move2.toContainer() = "new_host";
  background_migrations.push_back(move2);

  // Rebalancer respects background migrations
  MovesInProgressSpec movesSpec;
  movesSpec.name() = "background-migrations";
  movesSpec.moves() = std::move(background_migrations);
  solver.addConstraint(std::move(movesSpec));

  // Rebalancing won't interfere with background migrations
  // separate_migration_end
}

void resumable_rebalancing() {
  /**
   * Resume rebalancing after interruption.
   */
  class CheckpointDB {
    /**
     * Mock checkpoint database.
     */
   public:
    static std::vector<std::map<std::string, std::string>>
    get_unfinished_moves() {
      return {
          {{"object", "shard_1"}, {"dest", "host2"}},
          {{"object", "shard_2"}, {"dest", "host3"}},
      };
    }
  };

  // resumable_rebalancing_start
  [[maybe_unused]] auto resume_rebalancing = [](CheckpointDB& checkpoint_db) {
    /**
     * Resume rebalancing from last checkpoint.
     */
    auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
    ProblemSolver solver(executor, "example", "test");
    solver.setObjectName("shard");
    solver.setContainerName("host");

    solver.setAssignment(
        std::map<std::string, std::vector<std::string>>{
            {"host0", {"shard_1", "shard_2"}},
            {"host2", {}},
            {"host3", {}},
        });

    // Load moves that were in progress when we stopped
    auto unfinished_moves = checkpoint_db.get_unfinished_moves();

    std::vector<MoveInProgress> moves_in_progress;
    for (const auto& move : unfinished_moves) {
      MoveInProgress mip;
      mip.objName() = move.at("object");
      mip.toContainer() = move.at("dest");
      moves_in_progress.push_back(mip);
    }

    MovesInProgressSpec movesSpec;
    movesSpec.name() = "resume-moves";
    movesSpec.moves() = std::move(moves_in_progress);
    solver.addConstraint(std::move(movesSpec));

    // Continue from where we left off
    // auto solution = solver.solve();
  };
  // resumable_rebalancing_end
}

void prioritized_migration() {
  /**
   * Allow high-priority moves to complete before new rebalancing.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  [[maybe_unused]] auto get_critical_moves_in_progress = []() {
    // Mock function to get critical moves
    std::vector<MoveInProgress> moves;
    MoveInProgress move;
    move.objName() = "critical_shard_1";
    move.toContainer() = "host5";
    moves.push_back(move);
    return moves;
  };

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"critical_shard_1"}},
          {"host5", {}},
      });

  // prioritized_migration_start
  // High-priority moves from previous critical rebalancing
  auto critical_moves = get_critical_moves_in_progress();

  MovesInProgressSpec movesSpec;
  movesSpec.name() = "critical-moves";
  movesSpec.moves() = std::move(critical_moves);
  solver.addConstraint(std::move(movesSpec));

  // New rebalancing won't interfere with critical moves
  // Can plan additional moves for other objects
  // prioritized_migration_end
}

void rack_level_moves() {
  /**
   * Track moves at rack level.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("service");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack0", {"service_frontend", "service_backend"}},
          {"rack2", {}},
          {"rack3", {}},
      });

  // rack_level_moves_start
  // Moving entire services across racks
  std::vector<MoveInProgress> rack_moves;

  MoveInProgress move1;
  move1.objName() = "service_frontend";
  move1.toContainer() = "rack2";
  rack_moves.push_back(move1);

  MoveInProgress move2;
  move2.objName() = "service_backend";
  move2.toContainer() = "rack3";
  rack_moves.push_back(move2);

  MovesInProgressSpec movesSpec;
  movesSpec.name() = "rack-migrations";
  movesSpec.moves() = std::move(rack_moves);
  solver.addConstraint(std::move(movesSpec));
  // rack_level_moves_end
}

void integration_with_move_tracking() {
  /**
   * Integrate with production move tracking.
   */
  // move_tracker_class_start
  class MoveTracker {
    /**
     * Track moves in production.
     */
   public:
    MoveTracker() {}

    void start_move(
        const std::string& object_id,
        const std::string& destination) {
      // Record move start
      moves_db[object_id] = {
          {"destination", destination},
          {"start_time",
           std::to_string(
               std::chrono::system_clock::now().time_since_epoch().count())},
          {"status", "in_progress"},
      };
    }

    void complete_move(const std::string& object_id) {
      // Record move completion
      if (moves_db.find(object_id) != moves_db.end()) {
        moves_db[object_id]["status"] = "completed";
      }
    }

    std::vector<MoveInProgress> get_in_progress_moves() {
      // Get all in-progress moves for MovesInProgressSpec
      std::vector<MoveInProgress> moves;
      for (const auto& [obj, info] : moves_db) {
        if (info.at("status") == "in_progress") {
          MoveInProgress mip;
          mip.objName() = obj;
          mip.toContainer() = info.at("destination");
          moves.push_back(mip);
        }
      }
      return moves;
    }

   private:
    std::map<std::string, std::map<std::string, std::string>> moves_db;
  };
  // move_tracker_class_end

  [[maybe_unused]] auto enumerate_moves = [](const AssignmentSolution&) {
    // Mock function to enumerate moves
    return std::vector<
        std::pair<std::string, std::pair<std::string, std::string>>>();
  };

  [[maybe_unused]] auto initiate_migration = [](const std::string&,
                                                const std::string&) {
    // Mock function to initiate migration
  };

  // move_tracker_usage_start
  // Usage
  MoveTracker tracker;
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  // Apply solution and track moves
  // auto solution = solver.solve();
  // for (const auto& [obj, hosts] : enumerate_moves(solution)) {
  //     const auto& [old_host, new_host] = hosts;
  //     tracker.start_move(obj, new_host);
  //     initiate_migration(obj, new_host);
  // }

  // Later: next rebalancing round
  auto ongoing = tracker.get_in_progress_moves();
  MovesInProgressSpec movesSpec;
  movesSpec.name() = "tracked-moves";
  movesSpec.moves() = std::move(ongoing);
  solver.addConstraint(std::move(movesSpec));
  // move_tracker_usage_end
}

void moves_with_avoid_moving() {
  /**
   * Combine MovesInProgressSpec with AvoidMovingSpec.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard_1", "shard_5", "shard_8"}},
          {"host2", {}},
      });

  std::vector<MoveInProgress> ongoing_moves;
  MoveInProgress move;
  move.objName() = "shard_1";
  move.toContainer() = "host2";
  ongoing_moves.push_back(move);

  std::vector<std::string> recently_moved_objects = {"shard_5", "shard_8"};

  // moves_with_avoid_moving_start
  // Objects being moved right now
  MovesInProgressSpec movesSpec;
  movesSpec.name() = "ongoing";
  movesSpec.moves() = std::move(ongoing_moves);
  solver.addConstraint(std::move(movesSpec));

  // Objects recently moved (don't move again)
  AvoidMovingSpec avoidSpec;
  avoidSpec.name() = "recently-moved";
  avoidSpec.objects() = std::move(recently_moved_objects);
  solver.addConstraint(std::move(avoidSpec));
  // moves_with_avoid_moving_end
}

void moves_with_minimize_movement() {
  /**
   * Soft movement limit with in-progress tracking.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0", "shard_1"}},
          {"host1", {"shard1"}},
          {"host2", {"shard2"}},
      });
  solver.addObjectDimension(
      "data_size",
      std::map<std::string, double>{
          {"shard0", 10.0},
          {"shard1", 15.0},
          {"shard2", 20.0},
          {"shard_1", 12.0}});

  std::vector<MoveInProgress> ongoing_moves;
  MoveInProgress move;
  move.objName() = "shard_1";
  move.toContainer() = "host2";
  ongoing_moves.push_back(move);

  // moves_with_minimize_movement_start
  // Hard constraint: Don't touch ongoing moves
  MovesInProgressSpec movesSpec;
  movesSpec.name() = "ongoing";
  movesSpec.moves() = std::move(ongoing_moves);
  solver.addConstraint(std::move(movesSpec));

  // Soft goal: Minimize new moves
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-new-moves";
  minMove.dimension() = "data_size";
  solver.addGoal(std::move(minMove), 2.0);
  // moves_with_minimize_movement_end
}

void verify_move_tracking_example() {
  /**
   * Verify move tracking is accurate.
   */
  // verify_move_tracking_start
  [[maybe_unused]] auto verify_move_tracking =
      [](const std::vector<MoveInProgress>& ongoing_moves,
         const std::vector<std::map<std::string, std::string>>&
             actual_migrations) {
        /**
         * Verify MovesInProgressSpec matches reality.
         *
         * Args:
         *     ongoing_moves: List of MoveInProgress objects in spec
         *     actual_migrations: Actual migrations from production system
         *
         * Returns:
         *     True if tracking is accurate
         */
        std::set<std::pair<std::string, std::string>> spec_moves;
        for (const auto& m : ongoing_moves) {
          spec_moves.insert({*m.objName(), *m.toContainer()});
        }

        std::set<std::pair<std::string, std::string>> actual_moves;
        for (const auto& m : actual_migrations) {
          actual_moves.insert({m.at("object"), m.at("destination")});
        }

        // Find discrepancies
        std::set<std::pair<std::string, std::string>> in_spec_not_actual;
        std::set_difference(
            spec_moves.begin(),
            spec_moves.end(),
            actual_moves.begin(),
            actual_moves.end(),
            std::inserter(in_spec_not_actual, in_spec_not_actual.begin()));

        std::set<std::pair<std::string, std::string>> in_actual_not_spec;
        std::set_difference(
            actual_moves.begin(),
            actual_moves.end(),
            spec_moves.begin(),
            spec_moves.end(),
            std::inserter(in_actual_not_spec, in_actual_not_spec.begin()));

        if (in_spec_not_actual.empty() && in_actual_not_spec.empty()) {
          std::cout << fmt::format(
              "✓ Move tracking accurate: {} moves\n", spec_moves.size());
          return true;
        } else {
          std::cout << "✗ Move tracking discrepancies:\n";
          if (!in_spec_not_actual.empty()) {
            std::cout << fmt::format(
                "   Stale (in spec but not in progress): {}\n",
                in_spec_not_actual.size());
            int count = 0;
            for (const auto& [obj, dest] : in_spec_not_actual) {
              if (count++ >= 5) {
                break;
              }
              std::cout << fmt::format("     - {} → {}\n", obj, dest);
            }
          }
          if (!in_actual_not_spec.empty()) {
            std::cout << fmt::format(
                "   Missing (in progress but not in spec): {}\n",
                in_actual_not_spec.size());
            int count = 0;
            for (const auto& [obj, dest] : in_actual_not_spec) {
              if (count++ >= 5) {
                break;
              }
              std::cout << fmt::format("     - {} → {}\n", obj, dest);
            }
          }
          return false;
        }
      };
  // verify_move_tracking_end
}

void move_lifecycle_manager() {
  /**
   * Complete example of tracking move lifecycle.
   */
  // move_lifecycle_manager_start
  class MoveLifecycleManager {
    /**
     * Manage complete move lifecycle with rebalancer integration.
     */
   public:
    MoveLifecycleManager() {}

    static std::string get_current_host(const std::string&) {
      // Mock function to get current host
      return "host0";
    }

    void trigger_migration(const std::string&, const std::string&) {
      // Mock function to trigger migration
    }

    static std::vector<std::map<std::string, std::string>> plan_moves(
        const AssignmentSolution& solution) {
      // Extract moves from solution
      // assignment() is object -> container (F14FastMap<string, string>)
      std::vector<std::map<std::string, std::string>> planned;
      for (const auto& [obj, new_host] : *solution.assignment()) {
        std::string old_host = get_current_host(obj);
        if (old_host != new_host) {
          planned.push_back({
              {"object", obj},
              {"source", old_host},
              {"destination", new_host},
              {"state", "planned"},
          });
        }
      }
      return planned;
    }

    void start_moves(
        const std::vector<std::map<std::string, std::string>>& planned_moves) {
      // Start executing planned moves
      for (const auto& move : planned_moves) {
        moves[move.at("object")] = {
            {"destination", move.at("destination")},
            {"state", "in_progress"},
            {"start_time",
             std::to_string(
                 std::chrono::system_clock::now().time_since_epoch().count())},
        };
        // Trigger actual migration
        trigger_migration(move.at("object"), move.at("destination"));
      }
    }

    std::vector<MoveInProgress> get_moves_for_spec() {
      // Get MovesInProgress list for next rebalancing
      std::vector<MoveInProgress> result;
      for (const auto& [obj, info] : moves) {
        if (info.at("state") == "in_progress") {
          MoveInProgress mip;
          mip.objName() = obj;
          mip.toContainer() = info.at("destination");
          result.push_back(mip);
        }
      }
      return result;
    }

    void mark_complete(const std::string& object_id) {
      // Mark move as complete
      if (moves.find(object_id) != moves.end()) {
        moves[object_id]["state"] = "completed";
        moves[object_id]["end_time"] = std::to_string(
            std::chrono::system_clock::now().time_since_epoch().count());
      }
    }

    void cleanup_completed() {
      // Remove completed moves (periodic cleanup)
      std::map<std::string, std::map<std::string, std::string>> new_moves;
      for (const auto& [obj, info] : moves) {
        if (info.at("state") != "completed") {
          new_moves[obj] = info;
        }
      }
      moves = std::move(new_moves);
    }

   private:
    std::map<std::string, std::map<std::string, std::string>> moves;
  };
  // move_lifecycle_manager_end

  // move_lifecycle_usage_start
  // Usage:
  MoveLifecycleManager manager;
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  // Round 1: Plan and start moves
  // auto solution = solver.solve();
  // auto planned = manager.plan_moves(solution);
  // manager.start_moves(planned);

  // Round 2: Respect ongoing moves
  auto ongoing_moves = manager.get_moves_for_spec();
  MovesInProgressSpec movesSpec;
  movesSpec.name() = "ongoing";
  movesSpec.moves() = std::move(ongoing_moves);
  solver.addConstraint(std::move(movesSpec));

  // auto solution2 = solver.solve();
  // This won't interfere with moves from round 1
  // move_lifecycle_usage_end
}

// ============================================================================
// AvoidMovingSpec Examples
// ============================================================================

void avoid_moving_quick_example() {
  /**
   * Quick example showing basic AvoidMovingSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard_0", "shard_1", "shard_5"}},
      });

  // avoid_moving_quick_example_start
  // Pin critical database shards
  AvoidMovingSpec avoidSpec;
  avoidSpec.name() = "pin-critical-shards";
  avoidSpec.objects() = {"shard_0", "shard_1", "shard_5"};
  solver.addConstraint(std::move(avoidSpec));
  // avoid_moving_quick_example_end
}

void pin_critical_objects() {
  /**
   * Prevent moving objects with strict SLAs.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("instance");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0",
           {"primary_db_shard_0", "cache_master", "coordinator_instance"}},
      });

  // pin_critical_objects_start
  std::vector<std::string> critical_objects = {
      "primary_db_shard_0", "cache_master", "coordinator_instance"};

  AvoidMovingSpec avoidSpec;
  avoidSpec.name() = "pin-critical";
  avoidSpec.objects() = std::move(critical_objects);
  solver.addConstraint(std::move(avoidSpec));
  // pin_critical_objects_end
}

void pin_objects_with_ongoing_operations() {
  /**
   * Prevent moving objects currently being migrated or under maintenance.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  [[maybe_unused]] auto get_objects_being_migrated = []() {
    // Mock function to get objects being migrated
    return std::vector<std::string>{"shard_10", "shard_15"};
  };

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard_10", "shard_15"}},
      });

  // pin_ongoing_operations_start
  // Objects with active migrations
  auto in_flight_migrations = get_objects_being_migrated();

  AvoidMovingSpec avoidSpec;
  avoidSpec.name() = "pin-in-flight";
  avoidSpec.objects() = std::move(in_flight_migrations);
  solver.addConstraint(std::move(avoidSpec));
  // pin_ongoing_operations_end
}

void pin_recently_moved_objects() {
  /**
   * Avoid moving objects that were recently relocated.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  auto get_recently_moved_objects = [](int) {
    // Mock function to get recently moved objects
    return std::vector<std::string>{"shard_5", "shard_12"};
  };

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard_5", "shard_12"}},
      });

  // pin_recently_moved_start
  // Get objects moved in the last 24 hours
  auto recently_moved = get_recently_moved_objects(24);

  AvoidMovingSpec avoidSpec;
  avoidSpec.name() = "avoid-recent-moves";
  avoidSpec.objects() = std::move(recently_moved);
  solver.addConstraint(std::move(avoidSpec));
  // pin_recently_moved_end
}

void gradual_rebalancing() {
  /**
   * Incrementally rebalance by pinning most objects.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  const std::map<std::string, std::vector<std::string>> current_assignment = {
      {"host0", {"shard0", "shard1"}},
      {"host1", {"shard2", "shard3"}},
  };

  std::vector<std::string> all_objects;
  for (const auto& [host, objs] : current_assignment) {
    all_objects.insert(all_objects.end(), objs.begin(), objs.end());
  }

  solver.setAssignment(current_assignment);

  // gradual_rebalancing_start
  // Only allow 10% of objects to move per round
  std::random_device rd;
  std::mt19937 gen(rd());
  std::vector<std::string> objects_to_pin = all_objects;
  std::shuffle(objects_to_pin.begin(), objects_to_pin.end(), gen);
  objects_to_pin.resize(static_cast<size_t>(0.9 * all_objects.size()));

  AvoidMovingSpec avoidSpec;
  avoidSpec.name() = "pin-most-objects";
  avoidSpec.objects() = std::move(objects_to_pin);
  solver.addConstraint(std::move(avoidSpec));
  // gradual_rebalancing_end
}

void pin_by_tag() {
  /**
   * Pin objects matching certain criteria.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  const std::vector<std::string> all_objects = {"shard0", "shard1", "shard2"};
  std::map<std::string, std::map<std::string, std::string>> object_metadata = {
      {"shard0", {{"type", "stateful"}}},
      {"shard1", {{"type", "stateless"}}},
      {"shard2", {{"type", "stateful"}}},
  };

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0", "shard1", "shard2"}},
      });

  // pin_by_tag_start
  // Pin all objects with tag "stateful"
  std::vector<std::string> stateful_objects;
  for (const auto& obj : all_objects) {
    if (object_metadata[obj]["type"] == "stateful") {
      stateful_objects.push_back(obj);
    }
  }

  AvoidMovingSpec avoidSpec;
  avoidSpec.name() = "pin-stateful";
  avoidSpec.objects() = std::move(stateful_objects);
  solver.addConstraint(std::move(avoidSpec));
  // pin_by_tag_end
}

void avoid_moving_with_capacity() {
  /**
   * Combining AvoidMovingSpec with CapacitySpec.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  std::vector<std::string> critical_objects = {"shard_0", "shard_1"};

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard_0", "shard_1"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{{"shard_0", 4.0}, {"shard_1", 4.0}});
  solver.addContainerDimension(
      "memory_capacity", std::map<std::string, double>{{"host0", 32.0}});

  // avoid_moving_with_capacity_start
  // Ensure pinned objects don't violate capacity
  // (You may need to verify this beforehand)

  AvoidMovingSpec avoidSpec;
  avoidSpec.name() = "pin-critical";
  avoidSpec.objects() = std::move(critical_objects);
  solver.addConstraint(std::move(avoidSpec));

  CapacitySpec capSpec;
  capSpec.name() = "capacity";
  capSpec.scope() = "host";
  capSpec.dimension() = "memory";
  solver.addConstraint(std::move(capSpec));
  // avoid_moving_with_capacity_end
}

void avoid_moving_with_avoid_assignments() {
  /**
   * Combining AvoidMovingSpec with AvoidAssignmentsSpec (if available).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"obj_1", "obj_2"}},
      });

  // avoid_moving_with_avoid_assignments_start
  // Pin some objects, block others from certain hosts

  AvoidMovingSpec avoidSpec;
  avoidSpec.name() = "pin-on-current-hosts";
  avoidSpec.objects() = {"obj_1", "obj_2"};
  solver.addConstraint(std::move(avoidSpec));

  // Note: AvoidAssignmentsSpec would be imported and used here
  // This is a placeholder showing the pattern
  // avoid_moving_with_avoid_assignments_end
}

void gradual_migration_example() {
  /**
   * Gradually migrate objects using rounds.
   */
  [[maybe_unused]] auto apply_moves = [](const AssignmentSolution&) {
    // Mock function to apply moves
  };

  // gradual_migration_start
  [[maybe_unused]] auto gradual_migration =
      [](const std::vector<std::string>& objects, int rounds = 5) {
        /**
         * Migrate objects over multiple rounds.
         */
        const size_t objects_per_round = objects.size() / rounds;

        for (const auto round_num : folly::irange(rounds)) {
          // Pin all except current round's objects
          const size_t start_idx = round_num * objects_per_round;
          const size_t end_idx = (round_num + 1) * objects_per_round;

          std::vector<std::string> allow_moving(
              objects.begin() + start_idx,
              objects.begin() + std::min(end_idx, objects.size()));

          std::vector<std::string> pin_objects;
          for (const auto& obj : objects) {
            if (std::find(allow_moving.begin(), allow_moving.end(), obj) ==
                allow_moving.end()) {
              pin_objects.push_back(obj);
            }
          }

          auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
          ProblemSolver solver(executor, "example", "test");
          solver.setObjectName("shard");
          solver.setContainerName("host");

          AvoidMovingSpec avoidSpec;
          avoidSpec.name() = fmt::format("pin-round-{}", round_num);
          avoidSpec.objects() = pin_objects;
          solver.addConstraint(avoidSpec);

          // auto solution = solver.solve();
          // apply_moves(solution);

          std::cout << fmt::format(
              "Round {}: Moved {} objects\n",
              round_num + 1,
              allow_moving.size());
        }
      };
  // gradual_migration_end
}

void complete_movement_example() {
  /**
   * Complete runnable example demonstrating movement specs together.
   *
   * This example shows how to use all three movement specs together:
   * - MinimizeMovementSpec to prefer stability
   * - MovesInProgressSpec to track ongoing moves
   * - AvoidMovingSpec to pin critical objects
   */
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "movement-example", "production");

  solver.setObjectName("shard");
  solver.setContainerName("host");

  // Initial assignment
  const std::map<std::string, std::vector<std::string>> assignment = {
      {"host0", {"shard0", "shard1", "shard2"}},
      {"host1", {"shard3", "shard4"}},
      {"host2", {"shard5"}},
      {"host3", {"shard6", "shard7", "shard8"}},
  };
  solver.setAssignment(assignment);

  // Shard data sizes
  const std::map<std::string, double> data_sizes = {
      {"shard0", 10.0},
      {"shard1", 15.0},
      {"shard2", 20.0},
      {"shard3", 12.0},
      {"shard4", 18.0},
      {"shard5", 25.0},
      {"shard6", 8.0},
      {"shard7", 22.0},
      {"shard8", 14.0},
  };
  solver.addObjectDimension("data_size_gb", data_sizes);

  // Host capacities
  const std::map<std::string, double> host_capacity = {
      {"host0", 100.0},
      {"host1", 100.0},
      {"host2", 100.0},
      {"host3", 100.0},
  };
  solver.addContainerDimension("capacity_gb", host_capacity);

  // Constraint: Respect capacity
  CapacitySpec capSpec;
  capSpec.name() = "capacity";
  capSpec.scope() = "host";
  capSpec.dimension() = "data_size_gb";
  solver.addConstraint(std::move(capSpec));

  // Constraint: Track moves in progress from previous round
  std::vector<MoveInProgress> ongoing_moves;
  MoveInProgress move;
  move.objName() = "shard5";
  move.toContainer() = "host0";
  ongoing_moves.push_back(move);

  MovesInProgressSpec movesSpec;
  movesSpec.name() = "ongoing-migrations";
  movesSpec.moves() = ongoing_moves;
  solver.addConstraint(std::move(movesSpec));

  // Constraint: Pin critical shards
  std::vector<std::string> critical_shards = {"shard0", "shard1"};
  AvoidMovingSpec avoidSpec;
  avoidSpec.name() = "pin-critical";
  avoidSpec.objects() = critical_shards;
  solver.addConstraint(std::move(avoidSpec));

  // Goal: Achieve balance
  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-data";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "data_size_gb";
  balanceSpec.formula() = BalanceSpecFormula::LEGACY;
  balanceSpec.fixAverageToInitial() = true;
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Goal: Minimize movement (secondary priority)
  MinimizeMovementSpec minMove;
  minMove.name() = "minimize-moves";
  minMove.dimension() = "data_size_gb";
  solver.addGoal(std::move(minMove), 0.3);

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
  std::cout << fmt::format(
      "Critical shards pinned: {}\n", critical_shards.size());
  std::cout << fmt::format(
      "Moves in progress respected: {}\n", ongoing_moves.size());
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all Movement examples...\n";

  std::cout << "\n1. Minimize Movement Quick Example...\n";
  minimize_movement_quick_example();

  std::cout << "\n2. Minimize Data Movement...\n";
  minimize_data_movement();

  std::cout << "\n3. Stability With Allowance...\n";
  stability_with_allowance();

  std::cout << "\n4. Minimize Move Count...\n";
  minimize_move_count();

  std::cout << "\n5. Hierarchical Movement Minimization...\n";
  hierarchical_movement_minimization();

  std::cout << "\n6. Disable Magic Scaling...\n";
  disable_magic_scaling();

  std::cout << "\n7. Minimize Movement With Balance...\n";
  minimize_movement_with_balance();

  std::cout << "\n8. Minimize Movement With Capacity...\n";
  minimize_movement_with_capacity();

  std::cout << "\n9. Moves In Progress Quick Example...\n";
  moves_in_progress_quick_example();

  std::cout << "\n10. Basic Production Rebalancing...\n";
  basic_production_rebalancing();

  std::cout << "\n11. Multi Round Migration...\n";
  multi_round_migration();

  std::cout << "\n12. Separate Migration And Rebalancing...\n";
  separate_migration_and_rebalancing();

  std::cout << "\n13. Resumable Rebalancing...\n";
  resumable_rebalancing();

  std::cout << "\n14. Prioritized Migration...\n";
  prioritized_migration();

  std::cout << "\n15. Rack Level Moves...\n";
  rack_level_moves();

  std::cout << "\n16. Integration With Move Tracking...\n";
  integration_with_move_tracking();

  std::cout << "\n17. Moves With Avoid Moving...\n";
  moves_with_avoid_moving();

  std::cout << "\n18. Moves With Minimize Movement...\n";
  moves_with_minimize_movement();

  std::cout << "\n19. Move Lifecycle Manager...\n";
  move_lifecycle_manager();

  std::cout << "\n20. Avoid Moving Quick Example...\n";
  avoid_moving_quick_example();

  std::cout << "\n21. Pin Critical Objects...\n";
  pin_critical_objects();

  std::cout << "\n22. Pin Objects With Ongoing Operations...\n";
  pin_objects_with_ongoing_operations();

  std::cout << "\n23. Pin Recently Moved Objects...\n";
  pin_recently_moved_objects();

  std::cout << "\n24. Gradual Rebalancing...\n";
  gradual_rebalancing();

  std::cout << "\n25. Pin By Tag...\n";
  pin_by_tag();

  std::cout << "\n26. Avoid Moving With Capacity...\n";
  avoid_moving_with_capacity();

  std::cout << "\n27. Avoid Moving With Avoid Assignments...\n";
  avoid_moving_with_avoid_assignments();

  std::cout << "\n28. Gradual Migration Example...\n";
  gradual_migration_example();

  std::cout << "\n29. Complete Movement Example...\n";
  complete_movement_example();

  std::cout << "\n✓ All Movement examples completed successfully!\n";
  return 0;
}
// example_end
