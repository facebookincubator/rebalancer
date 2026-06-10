// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Stricter DC Diversity Variation
 *
 * Demonstrates enforcing stricter datacenter diversity by requiring all
 * replicas of a shard to be in different datacenters (MAX 1 replica per DC per
 * shard).
 *
 * This is useful when you have 3+ datacenters and want maximum geographic
 * diversity. For example, with 3 DCs and 3 replicas, each replica goes to a
 * different DC.
 */

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
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "replica-placer", "production");

  solver.setObjectName("replica");
  solver.setContainerName("server");

  // Create initial assignment
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(30)) {
    assignment["server" + std::to_string(i)] = {};
  }

  int replica_idx = 0;
  for (const auto shard : folly::irange(100)) {
    for (const auto replica : folly::irange(3)) {
      int server_idx = replica_idx % 10;
      std::string replica_name = "shard" + std::to_string(shard) + "_replica" +
          std::to_string(replica);
      assignment["server" + std::to_string(server_idx)].push_back(replica_name);
      replica_idx++;
    }
  }
  solver.setAssignment(assignment);

  // Define rack topology
  std::map<std::string, std::string> server_to_rack;
  for (const auto rack : folly::irange(6)) {
    for (const auto server_in_rack : folly::irange(5)) {
      int server_idx = rack * 5 + server_in_rack;
      server_to_rack["server" + std::to_string(server_idx)] =
          "rack" + std::to_string(rack);
    }
  }
  solver.addScope("rack", server_to_rack);

  // Define datacenter topology (maps servers to DCs)
  std::map<std::string, std::string> server_to_dc;
  for (const auto i : folly::irange(15)) {
    server_to_dc["server" + std::to_string(i)] = "dc1";
  }
  for (const auto i : folly::irange(15, 30)) {
    server_to_dc["server" + std::to_string(i)] = "dc2";
  }
  solver.addScope("datacenter", server_to_dc);

  // Define shard partition
  std::map<std::string, std::vector<std::string>> shard_partition;
  for (const auto shard : folly::irange(100)) {
    std::vector<std::string> replicas;
    replicas.reserve(3);
    for (const auto r : folly::irange(3)) {
      replicas.push_back(
          "shard" + std::to_string(shard) + "_replica" + std::to_string(r));
    }
    shard_partition["shard" + std::to_string(shard)] = replicas;
  }
  solver.addPartition("shard", shard_partition);

  // Replica storage sizes
  std::map<std::string, double> replica_storage;
  for (const auto shard : folly::irange(100)) {
    for (const auto r : folly::irange(3)) {
      std::string replica_name =
          "shard" + std::to_string(shard) + "_replica" + std::to_string(r);
      replica_storage[replica_name] = 50.0;
    }
  }
  solver.addObjectDimension("storage_gb", replica_storage);

  // Server capacities
  std::map<std::string, double> server_capacity;
  for (const auto i : folly::irange(30)) {
    server_capacity["server" + std::to_string(i)] = 600.0;
  }
  solver.addContainerDimension("storage_capacity_gb", server_capacity);

  // Rack diversity constraint
  GroupCountSpec rackDiversity;
  rackDiversity.name() = "rack-diversity";
  rackDiversity.scope() = "rack";
  rackDiversity.partitionName() = "shard";
  rackDiversity.bound() = GroupCountSpecBound::MAX;
  rackDiversity.limit() = Limit();
  rackDiversity.limit()->type() = LimitType::ABSOLUTE;
  rackDiversity.limit()->globalLimit() = 1;
  rackDiversity.definition() = GroupCountSpecDefinition::DURING;
  solver.addConstraint(rackDiversity);

  // variation_start
  // For 3 replicas, need 3 DCs (if available)
  GroupCountSpec strictDcDiversity;
  strictDcDiversity.name() = "strict-dc-diversity";
  strictDcDiversity.scope() = "datacenter";
  strictDcDiversity.partitionName() = "shard";
  strictDcDiversity.bound() = GroupCountSpecBound::MAX;
  strictDcDiversity.limit() = Limit();
  strictDcDiversity.limit()->type() = LimitType::ABSOLUTE;
  strictDcDiversity.limit()->globalLimit() = 1;
  solver.addConstraint(strictDcDiversity);
  // variation_end

  // Capacity constraint
  CapacitySpec capacity;
  capacity.name() = "storage-capacity";
  capacity.scope() = "server";
  capacity.dimension() = "storage_gb";
  solver.addConstraint(capacity);

  // Balance goals
  std::map<std::string, double> replica_count;
  for (const auto shard : folly::irange(100)) {
    for (const auto r : folly::irange(3)) {
      std::string replica_name =
          "shard" + std::to_string(shard) + "_replica" + std::to_string(r);
      replica_count[replica_name] = 1.0;
    }
  }
  solver.addObjectDimension("count", replica_count);

  BalanceSpec balanceCount;
  balanceCount.name() = "balance-replica-count";
  balanceCount.scope() = "server";
  balanceCount.dimension() = "count";
  balanceCount.formula() = BalanceSpecFormula::LEGACY;
  balanceCount.fixAverageToInitial() = true;
  solver.addGoal(balanceCount, 1.0);

  BalanceSpec balanceStorage;
  balanceStorage.name() = "balance-storage";
  balanceStorage.scope() = "server";
  balanceStorage.dimension() = "storage_gb";
  balanceStorage.formula() = BalanceSpecFormula::LEGACY;
  balanceStorage.fixAverageToInitial() = true;
  solver.addGoal(balanceStorage, 1.0);

  // Use Local Search solver
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 60000;
  solver.addSolver(localSearch);

  // Solve
  auto solution = solver.solve();

  std::cout << "AssignmentSolution found in " << "<solve time>" << "ms\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "Replicas moved: " << "<move count>" << "\n";

  return 0;
}
