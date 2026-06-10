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
 * FixedSource Move Type Reference Examples
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
  // Move objects from specific source to hot container
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;

  SingleFixedSourceMoveTypeSpec fixedSourceSpec;
  fixedSourceSpec.specialContainer() = "old_server"; // Fixed source

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFixedSourceMoveTypeSpec() =
      fixedSourceSpec;

  solver.addSolver(solverSpec);

  // Setup drain scenario
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_server", {"task0", "task1", "task2"}}, // Server to drain
          {"server1", {"task3"}},
          {"server2", {"task4"}},
          {"server3", {}},
      });

  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 1.5},
          {"task1", 2.0},
          {"task2", 1.5},
          {"task3", 2.0},
          {"task4", 2.0},
      });

  CapacitySpec capacitySpec;
  capacitySpec.name() = "cpu-capacity";
  capacitySpec.scope() = "server";
  capacitySpec.dimension() = "cpu";
  Limit limit;
  limit.globalLimit() = 5.0;
  capacitySpec.limit() = limit;
  solver.addConstraint(capacitySpec);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "server";
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

// drain_servers_start
void drainServers() {
  // Drain tasks from servers being decommissioned
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;

  SingleFixedSourceMoveTypeSpec fixedSourceSpec;
  ScopeItemList scopeItemList;
  scopeItemList.scopeItems() = {
      "old_server_01", "old_server_02", "old_server_03"};
  fixedSourceSpec.scopeItemList() = std::move(scopeItemList);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFixedSourceMoveTypeSpec() =
      fixedSourceSpec;

  solver.addSolver(solverSpec);

  // Setup multi-server drain
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_server_01", {"task0", "task1"}},
          {"old_server_02", {"task2"}},
          {"old_server_03", {"task3", "task4"}},
          {"server1", {}},
          {"server2", {}},
          {"server3", {}},
      });

  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 2.0},
          {"task2", 3.0},
          {"task3", 1.5},
          {"task4", 1.5},
      });

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "server";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 6.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "memory";
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
// drain_servers_end

// fill_container_start
void fillContainer() {
  // Fill underutilized container from specific sources
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SingleFixedSourceMoveTypeSpec fixedSourceSpec;
  ScopeItemList scopeItemList;
  scopeItemList.scopeItems() = {"overloaded_server_A", "overloaded_server_B"};
  fixedSourceSpec.scopeItemList() = std::move(scopeItemList);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFixedSourceMoveTypeSpec() =
      fixedSourceSpec;

  solver.addSolver(solverSpec);
}
// fill_container_end

// sampling_start
void sampling() {
  // Large source containers: sample 100 objects per source
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SingleFixedSourceMoveTypeSpec fixedSourceSpec;
  ScopeItemList scopeItemList;
  scopeItemList.scopeItems() = {"large_source_1", "large_source_2"};
  fixedSourceSpec.scopeItemList() = scopeItemList;

  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 100; // Sample ~100 per source
  fixedSourceSpec.sampleSize() = sampleSize;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFixedSourceMoveTypeSpec() =
      fixedSourceSpec;

  solver.addSolver(solverSpec);
}
// sampling_end

// early_exit_start
void earlyExit() {
  // Stop after first source with improvement
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SingleFixedSourceMoveTypeSpec fixedSourceSpec;
  ScopeItemList scopeItemList;
  scopeItemList.scopeItems() = {"source_A", "source_B", "source_C"};
  fixedSourceSpec.scopeItemList() = std::move(scopeItemList);
  fixedSourceSpec.stopEarlyAtScopeItemThatImprovesObjective() = true;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleFixedSourceMoveTypeSpec() =
      fixedSourceSpec;

  solver.addSolver(solverSpec);
}
// early_exit_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "=== Quick example showing basic FixedSource usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
