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
 * SingleFast Move Type Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
 */

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
  // Use SingleFast for faster convergence (stops after finding improvement)
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFastMoveTypeSpec() =
      SingleFastMoveTypeSpec{};

  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3"}},
          {"host1", {}},
          {"host2", {}},
      });

  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 1.5},
          {"task2", 2.5},
          {"task3", 1.0},
      });

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
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// quick_example_end

// basic_configuration_start
void basicConfiguration() {
  // SingleFast is good default for faster convergence
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFastMoveTypeSpec() =
      SingleFastMoveTypeSpec{};

  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
      });

  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 1.0},
          {"task1", 1.0},
      });

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
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// basic_configuration_end

// combined_with_single_start
void combinedWithSingle() {
  // Try fast moves first, then thorough search if needed
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  // Add SingleFast
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFastMoveTypeSpec() =
      SingleFastMoveTypeSpec{};

  // Add Single as fallback
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {}},
          {"host2", {}},
      });

  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 1.5},
          {"task2", 1.5},
      });

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Solve and print results
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
// combined_with_single_end

// more_objects_start
void moreObjects() {
  // Explore 5 objects before stopping (better quality)
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SingleFastMoveTypeSpec fastSpec;
  fastSpec.minHotObjects() = 5; // Explore 5 objects before early exit

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFastMoveTypeSpec() = fastSpec;

  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3", "task4"}},
          {"host1", {}},
          {"host2", {}},
      });

  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 1.0},
          {"task1", 2.0},
          {"task2", 1.5},
          {"task3", 2.5},
          {"task4", 1.0},
      });

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Solve and print results
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
// more_objects_end

// restrict_destinations_start
void restrictDestinations() {
  // Only explore moves within the same rack
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // Define rack scope
  solver.addScope(
      "rack",
      std::map<std::string, std::string>{
          {"host0", "rack0"},
          {"host1", "rack0"},
          {"host2", "rack1"},
          {"host3", "rack1"},
      });

  LocalSearchSolverSpec solverSpec;

  SingleFastMoveTypeSpec fastSpec;
  MoveToCurrentScopeItemSpec destSpec;
  destSpec.scopeNameForExploringMovesToCurrentScopeItem() = "rack";
  DestinationsToExploreOptions destOptions;
  destOptions.moveToCurrentScopeItem() = destSpec;
  fastSpec.destinationsToExplore() = std::move(destOptions);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFastMoveTypeSpec() = fastSpec;

  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {}},
          {"host2", {"task3"}},
          {"host3", {}},
      });

  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 1.0},
          {"task1", 1.0},
          {"task2", 1.0},
          {"task3", 1.0},
      });

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Solve and print results
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
// restrict_destinations_end

// interactive_start
void interactive() {
  // Fast solver for UI/dashboard (1 second limit)
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3", "task4"}},
          {"host1", {}},
          {"host2", {}},
          {"host3", {}},
      });

  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 1.5},
          {"task2", 2.5},
          {"task3", 1.0},
          {"task4", 2.0},
      });

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 1.0);

  LocalSearchSolverSpec solverSpec;
  solverSpec.solveTime() = 1; // 1 second limit for interactive response

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFastMoveTypeSpec() =
      SingleFastMoveTypeSpec{};

  solver.addSolver(solverSpec);

  // Solve and print results
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
// interactive_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);

  std::cout << "=== SingleFast Move Type Examples ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  std::cout << "Running interactive...\n";
  interactive();
  std::cout << "✓ interactive completed\n\n";

  return 0;
}
