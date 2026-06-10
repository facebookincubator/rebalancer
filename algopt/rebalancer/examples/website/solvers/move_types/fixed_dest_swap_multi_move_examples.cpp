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
 * FixedDestSwapMultiMove Move Type Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
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
  // Swap object bundles between hot container and specific destination
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;

  FixedDestSwapMultiMoveTypeSpec swapSpec;
  swapSpec.specialContainer() = "swap_partner"; // Fixed destination
  // RasLocalSearchMetadata is required for multi-move types
  auto rasMetadata = std::make_shared<const RasLocalSearchMetadata>();
  swapSpec.rasLocalSearchMetadata() = std::move(rasMetadata);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedDestSwapMultiMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);

  // Setup swap scenario
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"task0", "task1", "task2"}},
          {"swap_partner", {"task3", "task4", "task5"}},
          {"server2", {}},
      });

  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 1.0},
          {"task2", 1.0},
          {"task3", 1.5},
          {"task4", 1.5},
          {"task5", 1.0},
      });

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(balanceSpec, 1.0);

  // Note: solve() is not called here because multi-move types require
  // additional bundle/partition setup (e.g., search space partitioning)
  // that is beyond the scope of this basic example.
}
// quick_example_end

// traditional_swap_start
void traditionalSwap() {
  // Traditional 1:1 bundle swaps
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;

  FixedDestSwapMultiMoveTypeSpec swapSpec;
  swapSpec.specialContainer() = "destination_server";
  swapSpec.greedyOnSrc() = false; // Evaluate all combinations
  auto rasMetadata = std::make_shared<const RasLocalSearchMetadata>();
  swapSpec.rasLocalSearchMetadata() = std::move(rasMetadata);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedDestSwapMultiMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);

  // Setup balanced swap problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"task0", "task1"}},
          {"destination_server", {"task2", "task3"}},
          {"server2", {"task4"}},
      });

  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 3.0},
          {"task1", 2.0},
          {"task2", 1.5},
          {"task3", 1.5},
          {"task4", 2.0},
      });

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "memory";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Note: solve() is not called here because multi-move types require
  // additional bundle/partition setup (e.g., search space partitioning)
  // that is beyond the scope of this basic example.
}
// traditional_swap_end

// adaptive_swap_start
void adaptiveSwap() {
  // Adaptive 1:k swaps based on capacity dimension
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  auto rasMetadata = std::make_shared<RasLocalSearchMetadata>();
  StringKeyValueMap swapRatioDim;
  swapRatioDim.defaultValue() = "capacity";
  rasMetadata->swapRatioDimension() =
      std::move(swapRatioDim); // Dimension for ratios
  rasMetadata->useAdaptiveAllotments() = true; // Enable adaptive mode

  FixedDestSwapMultiMoveTypeSpec swapSpec;
  swapSpec.specialContainer() = "new_server_type";
  swapSpec.greedyOnSrc() = true; // Required for adaptive
  swapSpec.rasLocalSearchMetadata() = std::move(rasMetadata);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedDestSwapMultiMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// adaptive_swap_end

// greedy_start
void greedy() {
  // Greedy source selection for faster search
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  FixedDestSwapMultiMoveTypeSpec swapSpec;
  swapSpec.specialContainer() = "swap_destination";
  swapSpec.greedyOnSrc() = true; // Exit early on first improvement

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedDestSwapMultiMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// greedy_end

// sampling_start
void sampling() {
  // Limit evaluations with source and destination sampling
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  FixedDestSwapMultiMoveTypeSpec swapSpec;
  swapSpec.specialContainer() = "swap_partner";
  swapSpec.maxSamplesPerEquivSet() = 10; // Sample equiv sets
  swapSpec.maxSampleSizeOnSrc() = 20; // Max 20 source bundles
  swapSpec.maxSampleSizeOnDst() = 30; // Max 30 dest bundles

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedDestSwapMultiMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// sampling_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout
      << "=== Quick example showing basic FixedDestSwapMultiMove usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
