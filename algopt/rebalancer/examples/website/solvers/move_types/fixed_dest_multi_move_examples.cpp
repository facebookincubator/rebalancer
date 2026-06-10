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
 * FixedDestMultiMove Move Type Reference Examples
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
  // Move object bundles from hot container to specific destination
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;

  FixedDestMultiMoveTypeSpec fixedDestMultiSpec;
  fixedDestMultiSpec.specialContainer() = "new_server"; // Fixed destination
  // RasLocalSearchMetadata is required for multi-move types
  auto rasMetadata = std::make_shared<const RasLocalSearchMetadata>();
  fixedDestMultiSpec.rasLocalSearchMetadata() = std::move(rasMetadata);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedDestMultiMoveTypeSpec() =
      fixedDestMultiSpec;

  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"task0", "task1", "task2"}},
          {"server1", {"task3"}},
          {"new_server", {}},
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

// ras_migration_start
void rasMigration() {
  // Migrate RAS task bundles to new server
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;

  FixedDestMultiMoveTypeSpec fixedDestMultiSpec;
  fixedDestMultiSpec.specialContainer() = "new_ras_server"; // New server
  auto rasMetadata = std::make_shared<const RasLocalSearchMetadata>();
  fixedDestMultiSpec.rasLocalSearchMetadata() = std::move(rasMetadata);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedDestMultiMoveTypeSpec() =
      fixedDestMultiSpec;

  solver.addSolver(solverSpec);

  // Setup RAS problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"task0", "task1", "task2", "task3"}},
          {"server1", {"task4", "task5"}},
          {"new_ras_server", {}},
      });

  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 2.0},
          {"task2", 1.5},
          {"task3", 1.5},
          {"task4", 3.0},
          {"task5", 2.0},
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
// ras_migration_end

// fill_container_start
void fillContainer() {
  // Fill specific underutilized container with object bundles
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  FixedDestMultiMoveTypeSpec fixedDestMultiSpec;
  fixedDestMultiSpec.specialContainer() = "underutilized_server";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedDestMultiMoveTypeSpec() =
      fixedDestMultiSpec;

  solver.addSolver(solverSpec);
}
// fill_container_end

// sampling_start
void sampling() {
  // Limit evaluations by sampling equivalent sets
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  FixedDestMultiMoveTypeSpec fixedDestMultiSpec;
  fixedDestMultiSpec.specialContainer() = "destination_server";
  fixedDestMultiSpec.maxSamplesPerEquivSet() =
      10; // Evaluate up to 10 per equiv set

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedDestMultiMoveTypeSpec() =
      fixedDestMultiSpec;

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

  FixedDestMultiMoveTypeSpec fixedDestMultiSpec;
  fixedDestMultiSpec.specialContainer() = "ras_destination";
  fixedDestMultiSpec.maxSamplesPerEquivSet() = 5;
  fixedDestMultiSpec.rasLocalSearchMetadata() = std::move(rasMetadata);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().fixedDestMultiMoveTypeSpec() =
      fixedDestMultiSpec;

  solver.addSolver(solverSpec);
}
// ras_metadata_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout
      << "=== Quick example showing basic FixedDestMultiMove usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
