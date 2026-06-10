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
 * GroupRouting Move Type Reference Examples
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
  // Route groups based on latency configuration
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server1", {"shard1", "shard3"}},
          {"server2", {"shard2"}},
          {"server3", {"shard4"}},
      });

  solver.addObjectDimension(
      "size",
      std::map<std::string, double>{
          {"shard1", 1.0},
          {"shard2", 1.0},
          {"shard3", 1.0},
          {"shard4", 1.0},
      });

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-size";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "size";
  solver.addGoal(balanceSpec, 1.0);

  // Define partitions
  solver.addPartition(
      "table",
      std::unordered_map<std::string, std::vector<std::string>>{
          {"embeddings", {"shard1", "shard2"}},
          {"features", {"shard3", "shard4"}},
      });

  LocalSearchSolverSpec solverSpec;

  GroupRoutingMoveTypeSpec routingSpec;
  routingSpec.routingConfigName() = "latency-aware";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().groupRoutingMoveTypeSpec() = routingSpec;

  solver.addSolver(solverSpec);
}
// quick_example_end

// basic_routing_start
void basicRouting() {
  // Place groups based on routing configuration to minimize latency
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  std::unordered_map<std::string, std::vector<std::string>> tables = {
      {"embeddings", {}},
      {"features", {}},
  };
  solver.addPartition("table", std::move(tables));

  LocalSearchSolverSpec solverSpec;

  GroupRoutingMoveTypeSpec routingSpec;
  routingSpec.routingConfigName() = "cdn-latency";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().groupRoutingMoveTypeSpec() = routingSpec;

  solver.addSolver(solverSpec);
}
// basic_routing_end

// unassigned_start
void unassigned() {
  // Use unassigned container to enable placement from unassigned state
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> services = {
      {"web", {}},
      {"api", {}},
  };
  solver.addPartition("service", services);

  LocalSearchSolverSpec solverSpec;

  GroupRoutingMoveTypeSpec routingSpec;
  routingSpec.routingConfigName() = "edge-latency";
  routingSpec.unassignedContainer() = "unassigned";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().groupRoutingMoveTypeSpec() = routingSpec;

  solver.addSolver(solverSpec);
}
// unassigned_end

// multi_region_start
void multiRegion() {
  // Optimize cross-region placement based on origin-to-destination latency
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("replica");
  solver.setContainerName("datacenter");

  std::unordered_map<std::string, std::vector<std::string>> services = {
      {"storage", {}},
      {"compute", {}},
  };
  solver.addPartition("service", std::move(services));

  LocalSearchSolverSpec solverSpec;

  GroupRoutingMoveTypeSpec routingSpec;
  routingSpec.routingConfigName() = "cross-region-latency";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().groupRoutingMoveTypeSpec() = routingSpec;

  solver.addSolver(solverSpec);

  // Setup multi-region problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"datacenter1", {"replica1", "replica3"}},
          {"datacenter2", {"replica2"}},
          {"datacenter3", {"replica4"}},
      });

  solver.addPartition(
      "service",
      std::unordered_map<std::string, std::vector<std::string>>{
          {"storage", {"replica1", "replica2"}},
          {"compute", {"replica3", "replica4"}},
      });

  solver.addObjectDimension(
      "load",
      std::map<std::string, double>{
          {"replica1", 2.0},
          {"replica2", 2.0},
          {"replica3", 3.0},
          {"replica4", 3.0},
      });

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-load";
  balanceSpec.scope() = "datacenter";
  balanceSpec.dimension() = "load";
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
// multi_region_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "=== Quick example showing basic GroupRouting usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
