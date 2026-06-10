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
 * FixedSourceMultiMove Move Type Reference Examples
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
  // Move object bundles from specific source to hot container
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;

  FixedSrcMultiMoveTypeSpec fixedSrcMultiSpec;
  fixedSrcMultiSpec.specialContainer() = "old_server"; // Fixed source
  // RasLocalSearchMetadata is required for multi-move types
  auto rasMetadata = std::make_shared<const RasLocalSearchMetadata>();
  fixedSrcMultiSpec.rasLocalSearchMetadata() = std::move(rasMetadata);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedSrcMultiMoveTypeSpec() =
      fixedSrcMultiSpec;

  solver.addSolver(solverSpec);

  // Setup drain scenario
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_server", {"task0", "task1", "task2"}},
          {"server1", {"task3"}},
          {"server2", {}},
      });

  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 1.0},
          {"task1", 1.0},
          {"task2", 1.0},
          {"task3", 3.0},
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

// drain_server_start
void drainServer() {
  // Drain RAS task bundles from server being decommissioned
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;

  FixedSrcMultiMoveTypeSpec fixedSrcMultiSpec;
  fixedSrcMultiSpec.specialContainer() = "old_ras_server"; // Server to drain
  auto rasMetadata = std::make_shared<const RasLocalSearchMetadata>();
  fixedSrcMultiSpec.rasLocalSearchMetadata() = std::move(rasMetadata);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedSrcMultiMoveTypeSpec() =
      fixedSrcMultiSpec;

  solver.addSolver(solverSpec);

  // Setup RAS drain problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_ras_server", {"task0", "task1", "task2", "task3"}},
          {"server1", {"task4"}},
          {"server2", {"task5"}},
          {"server3", {}},
      });

  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 1.5},
          {"task1", 1.5},
          {"task2", 2.0},
          {"task3", 2.0},
          {"task4", 3.0},
          {"task5", 3.0},
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
// drain_server_end

// fill_container_start
void fillContainer() {
  // Fill underutilized container by pulling bundles from specific source
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  FixedSrcMultiMoveTypeSpec fixedSrcMultiSpec;
  fixedSrcMultiSpec.specialContainer() = "overloaded_server";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedSrcMultiMoveTypeSpec() =
      fixedSrcMultiSpec;

  solver.addSolver(solverSpec);
}
// fill_container_end

// sampling_start
void sampling() {
  // Limit evaluations by sampling equivalent sets
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  FixedSrcMultiMoveTypeSpec fixedSrcMultiSpec;
  fixedSrcMultiSpec.specialContainer() = "source_server";
  fixedSrcMultiSpec.maxSamplesPerEquivSet() =
      10; // Evaluate up to 10 per equiv set

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedSrcMultiMoveTypeSpec() =
      fixedSrcMultiSpec;

  solver.addSolver(solverSpec);
}
// sampling_end

// ras_metadata_start
void rasMetadata() {
  // RAS stackable solve with metadata
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  auto rasMetadata = std::make_shared<const RasLocalSearchMetadata>();
  // Configure RAS metadata as needed

  FixedSrcMultiMoveTypeSpec fixedSrcMultiSpec;
  fixedSrcMultiSpec.specialContainer() = "ras_source";
  fixedSrcMultiSpec.maxSamplesPerEquivSet() = 5;
  fixedSrcMultiSpec.rasLocalSearchMetadata() = std::move(rasMetadata);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedSrcMultiMoveTypeSpec() =
      fixedSrcMultiSpec;

  solver.addSolver(solverSpec);
}
// ras_metadata_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout
      << "=== Quick example showing basic FixedSourceMultiMove usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
