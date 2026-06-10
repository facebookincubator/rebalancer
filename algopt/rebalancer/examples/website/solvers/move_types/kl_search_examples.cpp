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
 * KLSearch Move Type Reference Examples
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
  // Enable KL Search for advanced optimization
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  KLSearchMoveTypeSpec klSearchSpec; // No parameters needed

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().klSearchMoveTypeSpec() = klSearchSpec;

  solver.addSolver(solverSpec);
}
// quick_example_end

void quickExampleComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  // Setup imbalanced problem that may have local optima
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"task0", "task1", "task2", "task3"}},
          {"server1", {"task4"}},
          {"server2", {}},
      });

  std::map<std::string, double> cpuDim = {
      {"task0", 1.5},
      {"task1", 2.0},
      {"task2", 1.5},
      {"task3", 1.0},
      {"task4", 4.0},
  };
  solver.addObjectDimension("cpu", std::move(cpuDim));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 1.0);

  LocalSearchSolverSpec solverSpec;
  KLSearchMoveTypeSpec klSearchSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().klSearchMoveTypeSpec() = klSearchSpec;
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

// basic_usage_start
void basicUsage() {
  // Use KL Search to escape local optima
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;

  KLSearchMoveTypeSpec klSearchSpec;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().klSearchMoveTypeSpec() = klSearchSpec;

  solver.addSolver(solverSpec);
}
// basic_usage_end

// combined_start
void combined() {
  // Start with fast moves, then use KL Search for refinement
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  // Fast initial optimization
  SingleMoveTypeSpec singleSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = singleSpec;

  // Refine and escape local optima
  KLSearchMoveTypeSpec klSearchSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().klSearchMoveTypeSpec() = klSearchSpec;

  solver.addSolver(solverSpec);
}
// combined_end

// balancing_start
void balancing() {
  // Use KL Search when simple moves plateau during load balancing
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("node");

  LocalSearchSolverSpec solverSpec;

  // Initial balancing
  SingleMoveTypeSpec singleSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = singleSpec;

  // Escape local optima for better balance
  KLSearchMoveTypeSpec klSearchSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().klSearchMoveTypeSpec() = klSearchSpec;

  solver.addSolver(solverSpec);
}
// balancing_end

void balancingComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("node");

  // Setup problem with potential local optima
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"node0", {"shard0", "shard1", "shard2"}},
          {"node1", {"shard3", "shard4"}},
          {"node2", {"shard5"}},
          {"node3", {}},
      });

  std::map<std::string, double> loadDim = {
      {"shard0", 2.0},
      {"shard1", 1.5},
      {"shard2", 2.5},
      {"shard3", 3.0},
      {"shard4", 2.0},
      {"shard5", 4.0},
  };
  solver.addObjectDimension("load", std::move(loadDim));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-load";
  balanceSpec.scope() = "node";
  balanceSpec.dimension() = "load";
  solver.addGoal(std::move(balanceSpec), 1.0);

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().klSearchMoveTypeSpec() =
      KLSearchMoveTypeSpec{};
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

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "=== Quick example showing basic KLSearch usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
