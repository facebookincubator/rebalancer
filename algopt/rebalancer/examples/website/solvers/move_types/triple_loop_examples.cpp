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
 * TripleLoop Move Type Reference Examples
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
  // Only use TripleLoop for small problems or as last resort
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  // Add Swap
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  // Add TripleLoop
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().tripleLoopMoveTypeSpec() =
      TripleLoopMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// quick_example_end

void quickExampleComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  // Setup small capacity-constrained problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2"}},
          {"host2", {"task3"}},
      });

  std::map<std::string, double> memoryDim = {
      {"task0", 2.5},
      {"task1", 1.5},
      {"task2", 3.5},
      {"task3", 1.5},
  };
  solver.addObjectDimension("memory", memoryDim);

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  capacitySpec.limit()->globalLimit() = 4.0; // Tight capacity
  solver.addConstraint(capacitySpec);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "memory";
  solver.addGoal(balanceSpec, 1.0);

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().tripleLoopMoveTypeSpec() =
      TripleLoopMoveTypeSpec{};
  solver.addSolver(solverSpec);

  auto solution = solver.solve();
  std::cout << "  Initial objective: " << *solution.initialObjective()->value()
            << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}

// last_resort_start
void lastResort() {
  // Try progressively more powerful moves
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  // Add Swap
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  // Add SingleEndChain
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleEndChainMoveTypeSpec() =
      SingleEndChainMoveTypeSpec{};

  // Add TripleLoop - last resort
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().tripleLoopMoveTypeSpec() =
      TripleLoopMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// last_resort_end

// small_problem_start
void smallProblem() {
  // For problems with <100 objects per container
  // WARNING: Do not use with large problems!
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  // Add TripleLoop - safe for small problems only
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().tripleLoopMoveTypeSpec() =
      TripleLoopMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// small_problem_end

void smallProblemComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  // Small problem with few objects
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"shard0", "shard1", "shard2"}},
          {"server1", {"shard3"}},
          {"server2", {}},
      });

  std::map<std::string, double> cpuDim = {
      {"shard0", 2.0},
      {"shard1", 1.5},
      {"shard2", 1.5},
      {"shard3", 4.0},
  };
  solver.addObjectDimension("cpu", std::move(cpuDim));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "cpu-capacity";
  capacitySpec.scope() = "server";
  capacitySpec.dimension() = "cpu";
  capacitySpec.limit()->globalLimit() = 5.0;
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 1.0);

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().tripleLoopMoveTypeSpec() =
      TripleLoopMoveTypeSpec{};
  solver.addSolver(solverSpec);

  auto solution = solver.solve();
  std::cout << "  Initial objective: " << *solution.initialObjective()->value()
            << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}

// time_limits_start
void timeLimits() {
  // Strict time limit to prevent expensive TripleLoop from taking too long
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;
  solverSpec.solveTime() = 60; // Maximum 60 seconds total

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  // Add Swap
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  // Add TripleLoop - will stop after 60s
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().tripleLoopMoveTypeSpec() =
      TripleLoopMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// time_limits_end

// offline_start
void offline() {
  // Offline optimization - take time to find best solution
  // Use when solution quality is critical and time is available
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;
  solverSpec.solveTime() = 3600; // Allow 1 hour

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  // Add Swap
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = SwapMoveTypeSpec{};

  // Add SingleEndChain
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleEndChainMoveTypeSpec() =
      SingleEndChainMoveTypeSpec{};

  // Add TripleLoop - thorough search for best solution
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().tripleLoopMoveTypeSpec() =
      TripleLoopMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// offline_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "=== Quick example showing basic TripleLoop usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
