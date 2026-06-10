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
 * Database Shard Placement
 *
 * This example demonstrates how to place database shards across servers with
 * capacity constraints and balance goals.
 *
 * Problem: 50 database shards of varying sizes need to be distributed across 10
 * servers with different storage capacities. Shards have different storage and
 * IOPS requirements. The current placement is highly imbalanced.
 *
 * Goal: Place shards to balance both storage and IOPS load while respecting
 * capacity limits and minimizing data movement.
 */

// solution_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "database-rebalancer", "production");

  // Define objects and containers
  solver.setObjectName("shard");
  solver.setContainerName("server");

  // Current imbalanced assignment
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(30)) {
    assignment["server0"].push_back("shard" + std::to_string(i));
  }
  for (const auto i : folly::irange(30, 45)) {
    assignment["server1"].push_back("shard" + std::to_string(i));
  }
  for (const auto i : folly::irange(45, 50)) {
    assignment["server2"].push_back("shard" + std::to_string(i));
  }
  for (const auto i : folly::irange(3, 10)) {
    assignment["server" + std::to_string(i)] = {};
  }
  solver.setAssignment(assignment);

  // Shard storage sizes (GB)
  std::map<std::string, double> shard_storage;
  for (const auto i : folly::irange(50)) {
    shard_storage["shard" + std::to_string(i)] = 50 + (i * 10) % 450;
  }
  solver.addObjectDimension("storage_gb", shard_storage);

  // Shard IOPS requirements
  std::map<std::string, double> shard_iops;
  for (const auto i : folly::irange(50)) {
    shard_iops["shard" + std::to_string(i)] = 500 + (i * 100) % 4500;
  }
  solver.addObjectDimension("iops", shard_iops);

  // Server storage capacities
  std::map<std::string, double> server_storage_capacity = {
      {"server0", 3000},
      {"server1", 3000},
      {"server2", 5000},
      {"server3", 5000},
      {"server4", 8000},
      {"server5", 3000},
      {"server6", 3000},
      {"server7", 5000},
      {"server8", 5000},
      {"server9", 8000}};
  solver.addContainerDimension("storage_capacity_gb", server_storage_capacity);

  // Server IOPS capacities
  std::map<std::string, double> server_iops_capacity;
  for (const auto i : folly::irange(10)) {
    server_iops_capacity["server" + std::to_string(i)] = 50000;
  }
  solver.addContainerDimension("iops_capacity", server_iops_capacity);

  // CONSTRAINT: Storage capacity
  CapacitySpec storageCapacity;
  storageCapacity.name() = "storage-capacity";
  storageCapacity.scope() = "server";
  storageCapacity.dimension() = "storage_gb";
  solver.addConstraint(storageCapacity);

  // CONSTRAINT: IOPS capacity
  CapacitySpec iopsCapacity;
  iopsCapacity.name() = "iops-capacity";
  iopsCapacity.scope() = "server";
  iopsCapacity.dimension() = "iops";
  solver.addConstraint(iopsCapacity);

  // GOAL: Balance storage
  BalanceSpec balanceStorage;
  balanceStorage.name() = "balance-storage";
  balanceStorage.scope() = "server";
  balanceStorage.dimension() = "storage_gb";
  balanceStorage.formula() = BalanceSpecFormula::LEGACY;
  balanceStorage.fixAverageToInitial() = true;
  solver.addGoal(balanceStorage, 1.0);

  // GOAL: Balance IOPS
  BalanceSpec balanceIops;
  balanceIops.name() = "balance-iops";
  balanceIops.scope() = "server";
  balanceIops.dimension() = "iops";
  balanceIops.formula() = BalanceSpecFormula::LEGACY;
  balanceIops.fixAverageToInitial() = true;
  solver.addGoal(balanceIops, 1.0);

  // GOAL: Minimize movement
  MinimizeMovementSpec minMovement;
  minMovement.name() = "minimize-movement";
  minMovement.scope() = "server";
  minMovement.dimension() = "storage_gb";
  solver.addGoal(minMovement, 0.2);

  // Use Local Search solver
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 30000;
  solver.addSolver(localSearch);

  // Solve
  auto solution = solver.solve();

  // Print results
  std::cout << "AssignmentSolution found in " << "<solve time>" << "ms\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "Shards moved: " << "<move count>" << "\n";

  return 0;
}
// solution_end
