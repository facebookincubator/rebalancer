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
 * GreedyGroupToScopeItem Move Type Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
 * */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

// quick_example_start
void quickExample() {
  // Move entire job to one datacenter, each task on different host
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  // Setup problem with job scattered across datacenters
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host1", {"task0"}},
          {"host2", {"task1"}},
          {"host3", {"task2"}},
      });

  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 1.0},
          {"task1", 1.0},
          {"task2", 1.0},
      });

  // Define partition with jobs
  std::unordered_map<std::string, std::vector<std::string>> jobs = {
      {"job0", {"task0", "task1", "task2"}},
  };
  solver.addPartition("job", jobs);

  // Define scope with datacenters
  std::map<std::string, std::string> dcScope = {
      {"host1", "dc1"},
      {"host2", "dc1"},
      {"host3", "dc1"},
  };
  solver.addScope("datacenter", dcScope);

  LocalSearchSolverSpec solverSpec;

  GreedyGroupToScopeItemMoveTypeSpec greedySpec;
  greedySpec.groupMovesPartition() = "job";
  greedySpec.scopeItemMovesScope() = "datacenter";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().greedyGroupToScopeItemMoveTypeSpec() =
      greedySpec;

  solver.addSolver(solverSpec);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(balanceSpec, 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial objective: " << std::fixed << std::setprecision(4)
            << *solution.initialObjective()->value() << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// quick_example_end

// replica_antiaffinity_start
void replicaAntiaffinity() {
  // Place all replicas of a shard in same rack, but on different machines
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("replica");
  solver.setContainerName("machine");

  // Setup with replicas scattered across racks
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"machine1", {"shard1_primary"}},
          {"machine2", {"shard2_primary"}},
          {"machine3", {}},
          {"machine4", {"shard1_replica1", "shard2_replica1"}},
          {"machine5", {"shard1_replica2"}},
          {"machine6", {"shard2_replica2"}},
      });

  solver.addObjectDimension(
      "size",
      std::map<std::string, double>{
          {"shard1_primary", 1.0},
          {"shard1_replica1", 1.0},
          {"shard1_replica2", 1.0},
          {"shard2_primary", 1.0},
          {"shard2_replica1", 1.0},
          {"shard2_replica2", 1.0},
      });

  // Define shard replicas
  std::unordered_map<std::string, std::vector<std::string>> shards = {
      {"shard1", {"shard1_primary", "shard1_replica1", "shard1_replica2"}},
      {"shard2", {"shard2_primary", "shard2_replica1", "shard2_replica2"}},
  };
  solver.addPartition("shard", std::move(shards));

  // Define rack scope
  std::map<std::string, std::string> rackScope = {
      {"machine1", "rack1"},
      {"machine2", "rack1"},
      {"machine3", "rack1"},
      {"machine4", "rack2"},
      {"machine5", "rack2"},
      {"machine6", "rack2"},
  };
  solver.addScope("rack", rackScope);

  LocalSearchSolverSpec solverSpec;

  GreedyGroupToScopeItemMoveTypeSpec greedySpec;
  greedySpec.groupMovesPartition() = "shard";
  greedySpec.scopeItemMovesScope() = "rack";
  greedySpec.nSampleSetsToExplore() = 2;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().greedyGroupToScopeItemMoveTypeSpec() =
      greedySpec;

  solver.addSolver(solverSpec);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-size";
  balanceSpec.scope() = "machine";
  balanceSpec.dimension() = "size";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial objective: " << std::fixed << std::setprecision(4)
            << *solution.initialObjective()->value() << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// replica_antiaffinity_end

// job_placement_start
void jobPlacement() {
  // Keep job tasks together in one datacenter, spread across hosts
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  // Define jobs
  std::unordered_map<std::string, std::vector<std::string>> jobs = {
      {"webserver", {"web_task1", "web_task2", "web_task3"}},
      {"database", {"db_task1", "db_task2"}},
  };
  solver.addPartition("job", std::move(jobs));

  // Define datacenter scope
  std::map<std::string, std::string> dcScope = {
      {"host1", "us-east"},
      {"host2", "us-east"},
      {"host3", "us-east"},
      {"host4", "us-west"},
      {"host5", "us-west"},
      {"host6", "us-west"},
  };
  solver.addScope("datacenter", dcScope);

  LocalSearchSolverSpec solverSpec;

  GreedyGroupToScopeItemMoveTypeSpec greedySpec;
  greedySpec.groupMovesPartition() = "job";
  greedySpec.scopeItemMovesScope() = "datacenter";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().greedyGroupToScopeItemMoveTypeSpec() =
      greedySpec;

  solver.addSolver(solverSpec);
}
// job_placement_end

// higher_sampling_start
void higherSampling() {
  // Explore more container sets for better placement quality
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> services = {
      {"service1", {"instance1", "instance2", "instance3"}},
  };
  solver.addPartition("service", std::move(services));

  std::map<std::string, std::string> zoneScope = {
      {"node1", "zone1"},
      {"node2", "zone1"},
      {"node3", "zone1"},
  };
  solver.addScope("zone", zoneScope);

  LocalSearchSolverSpec solverSpec;

  GreedyGroupToScopeItemMoveTypeSpec greedySpec;
  greedySpec.groupMovesPartition() = "service";
  greedySpec.scopeItemMovesScope() = "zone";
  greedySpec.nSampleSetsToExplore() = 10; // Higher sampling for better quality

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().greedyGroupToScopeItemMoveTypeSpec() =
      greedySpec;

  solver.addSolver(solverSpec);
}
// higher_sampling_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout
      << "=== Quick example showing basic GreedyGroupToScopeItem usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
