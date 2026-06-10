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
 * SingleChain Move Type Reference Examples
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
  // 2-object chains: hot container gives one, receives another
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleChainMoveTypeSpec() =
      SingleChainMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// quick_example_end

void quickExampleComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleChainMoveTypeSpec() =
      SingleChainMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup capacity-constrained problem requiring chain moves
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0",
           {"task0", "task1", "task2", "task3"}}, // At capacity, imbalanced
          {"host1", {"task4"}}, // At capacity
          {"host2", {}}, // Empty
      });

  std::map<std::string, double> memory = {
      {"task0", 3.0},
      {"task1", 1.0},
      {"task2", 2.0},
      {"task3", 2.0},
      {"task4", 4.0},
  };
  solver.addObjectDimension("memory", memory);

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 8.0; // Tight capacity
  capacitySpec.limit() = limit;
  solver.addConstraint(capacitySpec);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "memory";
  solver.addGoal(balanceSpec, 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial objective: " << *solution.initialObjective()->value()
            << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << *solution.initialObjective()->value() -
          *solution.finalObjective()->value()
            << "\n";
}

// basic_start
void basic() {
  // Hot container must remain balanced (one out, one in)
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleChainMoveTypeSpec() =
      SingleChainMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// basic_end

// restricted_start
void restricted() {
  // Only replace hot object with same type
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // Define object type partition
  std::unordered_map<std::string, std::vector<std::string>> taskTypes = {
      {"batch", {"task1", "task2"}},
      {"realtime", {"task3", "task4"}},
      {"ml", {"task5", "task6"}},
  };
  solver.addPartition("task_type", taskTypes);

  LocalSearchSolverSpec solverSpec;

  SingleChainMoveTypeSpec chainSpec;
  chainSpec.partitionNameToExploreChainsWithinObjectGroup() = "task_type";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleChainMoveTypeSpec() = chainSpec;

  solver.addSolver(solverSpec);
}
// restricted_end

// fixed_dest_start
void fixedDest() {
  // Move hot objects to specific destination, find replacements
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SingleChainMoveTypeSpec chainSpec;
  chainSpec.specialColdContainer() = "new_server"; // Fixed destination

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleChainMoveTypeSpec() = chainSpec;

  solver.addSolver(solverSpec);
}
// fixed_dest_end

// combined_start
void combined() {
  // Multi-stage: simple moves first, then chains
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  // Add SingleEndChain
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleEndChainMoveTypeSpec() =
      SingleEndChainMoveTypeSpec{};

  // Add SingleChain
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleChainMoveTypeSpec() =
      SingleChainMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// combined_end

void combinedComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleEndChainMoveTypeSpec() =
      SingleEndChainMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleChainMoveTypeSpec() =
      SingleChainMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup tight capacity problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3", "task4"}},
          {"host2", {"task5"}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 2.0},
      {"task1", 3.0},
      {"task2", 1.0},
      {"task3", 2.5},
      {"task4", 1.5},
      {"task5", 4.0},
  };
  solver.addObjectDimension("cpu", cpu);

  CapacitySpec capacitySpec;
  capacitySpec.name() = "cpu-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "cpu";
  Limit limit;
  limit.globalLimit() = 6.0;
  capacitySpec.limit() = limit;
  solver.addConstraint(capacitySpec);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(balanceSpec, 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial objective: " << *solution.initialObjective()->value()
            << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << *solution.initialObjective()->value() -
          *solution.finalObjective()->value()
            << "\n";
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "=== Quick example showing basic SingleChain usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
