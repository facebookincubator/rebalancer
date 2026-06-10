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
 * SingleColdestStratified Move Type Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
 *
 * NOTE: SingleColdestStratified uses legacy string-based move type
 * specification.
 * */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

// quick_example_start
void quickExample() {
  // Move to coldest containers per stratum (legacy string-based)
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec spec;
  spec.stratifiedSampleSize() = 10; // Sample 10 coldest per stratum

  spec.moveTypeList()->emplace_back();
  spec.moveTypeList()->back().moveTypeName() = "SINGLE_COLDEST_STRATIFIED";

  solver.addSolver(spec);
  // quick_example_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {}},
          {"host2", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 1.0},
      {"task1", 1.0},
      {"task2", 1.0},
  };
  solver.addObjectDimension("cpu", cpu);

  // SimilarContainers is required for SINGLE_COLDEST_STRATIFIED
  solver.addSimilarContainers({{"host0", "host1", "host2"}});

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(balanceSpec, 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial: " << *solution.initialObjective()->value() << "\n";
  std::cout << "  Final: " << *solution.finalObjective()->value() << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// pattern_end

// bin_packing_start
void binPacking() {
  // Bin-packing: fill coldest (most empty) containers
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-containers";
  minimizeSpec.scope() = "host";
  solver.addGoal(std::move(minimizeSpec), 10.0);

  LocalSearchSolverSpec spec;
  spec.stratifiedSampleSize() = 10; // Try 10 coldest per stratum

  spec.moveTypeList()->emplace_back();
  spec.moveTypeList()->back().moveTypeName() = "SINGLE_COLDEST_STRATIFIED";

  solver.addSolver(spec);
  // bin_packing_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {"task2"}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 1.0},
      {"task1", 1.0},
      {"task2", 1.0},
  };
  solver.addObjectDimension("cpu", std::move(cpu));

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial: " << *solution.initialObjective()->value() << "\n";
  std::cout << "  Final: " << *solution.finalObjective()->value() << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// pattern_end

// capacity_balancing_start
void capacityBalancing() {
  // Move to coldest servers in each region for capacity balancing
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  LocalSearchSolverSpec spec;
  spec.stratifiedSampleSize() = 20; // Sample 20 coldest per region

  spec.moveTypeList()->emplace_back();
  spec.moveTypeList()->back().moveTypeName() = "SINGLE_COLDEST_STRATIFIED";

  solver.addSolver(spec);
  // capacity_balancing_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"task0", "task1", "task2"}},
          {"server1", {}},
          {"server2", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 2.0},
      {"task1", 1.5},
      {"task2", 1.5},
  };
  solver.addObjectDimension("cpu", std::move(cpu));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial: " << *solution.initialObjective()->value() << "\n";
  std::cout << "  Final: " << *solution.finalObjective()->value() << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// pattern_end

// random_sampling_start
void randomSampling() {
  // Include random sampling for diversity: k coldest + k random
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec spec;
  spec.stratifiedSampleSize() = 10; // 10 coldest
  spec.includeEqualSizeRandomSampleForSingleColdestMoveType() =
      true; // + 10 random = 20 total

  spec.moveTypeList()->emplace_back();
  spec.moveTypeList()->back().moveTypeName() = "SINGLE_COLDEST_STRATIFIED";

  solver.addSolver(spec);
  // random_sampling_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 1.0},
      {"task1", 1.0},
  };
  solver.addObjectDimension("cpu", std::move(cpu));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial: " << *solution.initialObjective()->value() << "\n";
  std::cout << "  Final: " << *solution.finalObjective()->value() << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// pattern_end

// decommission_start
void decommission() {
  // Decommission servers: move shards to coldest available servers
  // Use with toFree or nonAccepting constraints to mark servers for
  // decommission
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  LocalSearchSolverSpec spec;
  spec.stratifiedSampleSize() = 15; // Try 15 coldest alternatives per region

  spec.moveTypeList()->emplace_back();
  spec.moveTypeList()->back().moveTypeName() = "SINGLE_COLDEST_STRATIFIED";

  solver.addSolver(spec);
  // decommission_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"shard0", "shard1", "shard2"}},
          {"server1", {}},
          {"server2", {}},
      });

  std::map<std::string, double> memory = {
      {"shard0", 2.0},
      {"shard1", 1.5},
      {"shard2", 1.5},
  };
  solver.addObjectDimension("memory", std::move(memory));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "memory";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial: " << *solution.initialObjective()->value() << "\n";
  std::cout << "  Final: " << *solution.finalObjective()->value() << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// pattern_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout
      << "=== Quick example showing basic SingleColdestStratified usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
