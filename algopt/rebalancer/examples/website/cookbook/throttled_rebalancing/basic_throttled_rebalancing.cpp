// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Basic Throttled Rebalancing Example
 *
 * Demonstrates how to gradually rebalance shards over multiple rounds with
 * strict move limits to avoid overwhelming production systems. This example
 * shows:
 * - Multi-round rebalancing with movement throttling
 * - Pinning recently moved shards to prevent ping-pong behavior
 * - Balancing multiple dimensions (data size and query rate)
 * - Tracking progress across rounds
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

// solution_start

std::map<std::string, std::vector<std::string>> initializeImbalancedCluster(
    int shard_count,
    int host_count) {
  std::map<std::string, std::vector<std::string>> assignment;

  for (const auto i : folly::irange(host_count)) {
    assignment["host" + std::to_string(i)] = {};
  }

  // Imbalanced: 80% on first 20 hosts, 20% on last 30 hosts
  for (const auto i : folly::irange(shard_count)) {
    int host_idx;
    if (i < 400) {
      host_idx = i % 20; // Overloaded
    } else {
      host_idx = 20 + (i % 30); // Underutilized
    }
    assignment["host" + std::to_string(host_idx)].push_back(
        "shard" + std::to_string(i));
  }

  return assignment;
}

// Convert from object->container map to container->objects map
std::map<std::string, std::vector<std::string>> convertAssignment(
    const std::map<std::string, std::string>& object_to_container,
    int host_count) {
  std::map<std::string, std::vector<std::string>> result;
  // Initialize empty vectors for all hosts
  for (const auto i : folly::irange(host_count)) {
    result["host" + std::to_string(i)] = {};
  }
  // Populate from object->container mapping
  for (const auto& [object, container] : object_to_container) {
    result[container].push_back(object);
  }
  return result;
}

std::set<std::string> identifyMovedShards(
    const std::map<std::string, std::vector<std::string>>& initial,
    const std::map<std::string, std::vector<std::string>>& final_assignment) {
  std::set<std::string> moved;

  for (const auto& [host, shards] : final_assignment) {
    auto it = initial.find(host);
    if (it == initial.end()) {
      // All shards on this host are new arrivals
      moved.insert(shards.begin(), shards.end());
      continue;
    }
    std::set<std::string> initial_shards(it->second.begin(), it->second.end());
    std::set<std::string> final_shards(shards.begin(), shards.end());

    // Find new arrivals
    for (const auto& shard : final_shards) {
      if (initial_shards.find(shard) == initial_shards.end()) {
        moved.insert(shard);
      }
    }
  }

  return moved;
}

AssignmentSolution solveRebalancingRound(
    const std::map<std::string, std::vector<std::string>>& current_assignment,
    const std::set<std::string>& moved_shards_history,
    int max_moves,
    int shard_count,
    int host_count) {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "throttled-rebalancer", "production");

  solver.setObjectName("shard");
  solver.setContainerName("host");
  solver.setAssignment(current_assignment);

  // Generate random shard sizes and query rates
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> size_dist(10.0, 100.0);
  std::uniform_real_distribution<> qps_dist(100.0, 1000.0);

  std::map<std::string, double> shard_sizes;
  std::map<std::string, double> shard_qps;
  for (const auto i : folly::irange(shard_count)) {
    std::string shard = "shard" + std::to_string(i);
    shard_sizes[shard] = size_dist(gen);
    shard_qps[shard] = qps_dist(gen);
  }
  solver.addObjectDimension("data_size", std::move(shard_sizes));
  solver.addObjectDimension("query_rate", std::move(shard_qps));

  // Host capacities
  std::map<std::string, double> host_capacity;
  for (const auto i : folly::irange(host_count)) {
    host_capacity["host" + std::to_string(i)] = 1000.0;
  }
  solver.addContainerDimension("host_capacity", host_capacity);

  // CONSTRAINT 1: Host capacity
  CapacitySpec capacity;
  capacity.name() = "host-capacity";
  capacity.scope() = "host";
  capacity.dimension() = "data_size";
  solver.addConstraint(std::move(capacity));

  // CONSTRAINT 2: Pin recently moved shards
  if (!moved_shards_history.empty()) {
    AvoidMovingSpec pin;
    pin.name() = "pin-recently-moved";
    pin.objects() = std::vector<std::string>(
        moved_shards_history.begin(), moved_shards_history.end());
    solver.addConstraint(std::move(pin));
  }

  // GOAL 1: Balance data size
  BalanceSpec balance_data;
  balance_data.name() = "balance-data";
  balance_data.scope() = "host";
  balance_data.dimension() = "data_size";
  balance_data.formula() = BalanceSpecFormula::LEGACY;
  balance_data.fixAverageToInitial() = true;
  solver.addGoal(std::move(balance_data), 2.0);

  // GOAL 2: Balance query rate
  BalanceSpec balance_qps;
  balance_qps.name() = "balance-queries";
  balance_qps.scope() = "host";
  balance_qps.dimension() = "query_rate";
  balance_qps.formula() = BalanceSpecFormula::LEGACY;
  balance_qps.fixAverageToInitial() = true;
  solver.addGoal(std::move(balance_qps), 2.0);

  // GOAL 3: Minimize movement with allowance
  MinimizeMovementSpec min_move;
  min_move.name() = "minimize-movement";
  min_move.scope() = "host";
  min_move.dimension() = "data_size";
  min_move.allowance() = max_moves * 50.0;
  min_move.doNotNormalize() = true;
  solver.addGoal(std::move(min_move), 1.0);

  // Use Local Search solver
  LocalSearchSolverSpec ls_solver;
  ls_solver.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  ls_solver.solveTime() = 30000;
  solver.addSolver(ls_solver);

  return solver.solve();
}

void throttledRebalancing() {
  const int MAX_MOVES_PER_ROUND = 50;
  const int TOTAL_ROUNDS = 10;
  const int SHARD_COUNT = 500;
  const int HOST_COUNT = 50;

  std::set<std::string> moved_shards_history;
  auto current_assignment =
      initializeImbalancedCluster(SHARD_COUNT, HOST_COUNT);

  std::cout << "Starting throttled rebalancing (" << TOTAL_ROUNDS
            << " rounds, max " << MAX_MOVES_PER_ROUND << " moves/round)\n";
  std::cout << std::string(80, '=') << "\n";

  for (const auto round_num : folly::irange(TOTAL_ROUNDS)) {
    std::cout << "\nRound " << (round_num + 1) << "/" << TOTAL_ROUNDS << "\n";
    std::cout << std::string(40, '-') << "\n";

    auto solution = solveRebalancingRound(
        current_assignment,
        moved_shards_history,
        MAX_MOVES_PER_ROUND,
        SHARD_COUNT,
        HOST_COUNT);

    // Convert solution assignment from object->container to container->objects
    auto new_assignment = convertAssignment(
        std::map<std::string, std::string>(
            solution.assignment()->begin(), solution.assignment()->end()),
        HOST_COUNT);

    // Identify moved shards
    auto moved_this_round =
        identifyMovedShards(current_assignment, new_assignment);

    // Update tracking
    moved_shards_history.insert(
        moved_this_round.begin(), moved_this_round.end());
    current_assignment = new_assignment;

    // Print results
    std::cout << "  Moves this round: " << moved_this_round.size() << "\n";
    std::cout << "  Total moves so far: " << moved_shards_history.size()
              << "\n";
    std::cout << "  Objective value: " << *solution.finalObjective()->value()
              << "\n";
    std::cout << "  Solve time: " << "<solve time>" << "ms\n";

    if (moved_this_round.empty()) {
      std::cout << "\n✅ Converged after " << (round_num + 1)
                << " rounds (no more beneficial moves)\n";
      break;
    }
  }

  std::cout << "\nThrottled rebalancing complete!\n";
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  throttledRebalancing();
  return 0;
}
// solution_end
