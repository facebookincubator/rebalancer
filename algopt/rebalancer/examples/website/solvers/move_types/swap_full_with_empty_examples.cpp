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
 * SwapFullWithEmpty Move Type Reference Examples
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
  // Move all objects from hot container to empty containers
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullWithEmptyContainersMoveTypeSpec() =
      SwapFullWithEmptyContainersMoveTypeSpec{};

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
  solverSpec.moveTypeList()->back().swapFullWithEmptyContainersMoveTypeSpec() =
      SwapFullWithEmptyContainersMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem with scattered workload
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0"}}, // 4GB
          {"host1", {"task1"}}, // 4GB
          {"host2", {"task2"}}, // 4GB
          {"host3", {}}, // Empty
          {"host4", {}}, // Empty
      });

  std::map<std::string, double> memory = {
      {"task0", 4.0},
      {"task1", 4.0},
      {"task2", 4.0},
  };
  solver.addObjectDimension("memory", memory);

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 8.0;
  capacitySpec.limit() = limit;
  solver.addConstraint(capacitySpec);

  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-hosts";
  minimizeSpec.scope() = "host";
  solver.addGoal(minimizeSpec, 1.0);

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

// consolidation_start
void consolidation() {
  // Consolidate workload to reduce active servers
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-servers";
  minimizeSpec.scope() = "server";
  solver.addGoal(std::move(minimizeSpec), 1.0);

  LocalSearchSolverSpec solverSpec;

  // Add SwapFullWithEmpty
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullWithEmptyContainersMoveTypeSpec() =
      SwapFullWithEmptyContainersMoveTypeSpec{};

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// consolidation_end

void consolidationComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("server");

  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-servers";
  minimizeSpec.scope() = "server";
  solver.addGoal(std::move(minimizeSpec), 1.0);

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullWithEmptyContainersMoveTypeSpec() =
      SwapFullWithEmptyContainersMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"task0", "task1"}}, // 8GB
          {"server1", {"task2"}}, // 4GB
          {"server2", {"task3"}}, // 4GB
          {"server3", {}}, // Empty
      });

  std::map<std::string, double> memory = {
      {"task0", 4.0},
      {"task1", 4.0},
      {"task2", 4.0},
      {"task3", 4.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "server";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 16.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

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

// decommission_start
void decommission() {
  // Decommission specific servers by moving all shards to empties
  // Use nonAccepting or toFree constraints to mark servers for decommission
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullWithEmptyContainersMoveTypeSpec() =
      SwapFullWithEmptyContainersMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// decommission_end

void decommissionComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullWithEmptyContainersMoveTypeSpec() =
      SwapFullWithEmptyContainersMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"shard0", "shard1"}}, // 8GB - to decommission
          {"server1", {"shard2"}}, // 4GB
          {"server2", {}}, // Empty
          {"server3", {}}, // Empty
      });

  std::map<std::string, double> memory = {
      {"shard0", 4.0},
      {"shard1", 4.0},
      {"shard2", 4.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "server";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 12.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "memory";
  solver.addGoal(std::move(balanceSpec), 1.0);

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

// bin_packing_start
void binPacking() {
  // Bin-packing: minimize active containers
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("container");

  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-bins";
  minimizeSpec.scope() = "container";
  solver.addGoal(
      std::move(minimizeSpec), 10.0); // High weight for consolidation

  LocalSearchSolverSpec solverSpec;

  // Add SwapFullWithEmpty
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullWithEmptyContainersMoveTypeSpec() =
      SwapFullWithEmptyContainersMoveTypeSpec{};

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// bin_packing_end

void binPackingComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("container");

  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-bins";
  minimizeSpec.scope() = "container";
  solver.addGoal(std::move(minimizeSpec), 10.0);

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullWithEmptyContainersMoveTypeSpec() =
      SwapFullWithEmptyContainersMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"container0", {"task0"}}, // 3GB
          {"container1", {"task1"}}, // 3GB
          {"container2", {"task2"}}, // 3GB
          {"container3", {}}, // Empty
          {"container4", {}}, // Empty
      });

  std::map<std::string, double> memory = {
      {"task0", 3.0},
      {"task1", 3.0},
      {"task2", 3.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "container";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 10.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

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

// combined_start
void combined() {
  // Multi-stage consolidation strategy
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-containers";
  minimizeSpec.scope() = "container";
  solver.addGoal(std::move(minimizeSpec), 5.0);

  LocalSearchSolverSpec solverSpec;

  // Add SwapFullWithEmpty
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullWithEmptyContainersMoveTypeSpec() =
      SwapFullWithEmptyContainersMoveTypeSpec{};

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solver.addSolver(solverSpec);
}
// combined_end

void combinedComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  MinimizeContainersSpec minimizeSpec;
  minimizeSpec.name() = "minimize-containers";
  minimizeSpec.scope() = "container";
  solver.addGoal(minimizeSpec, 5.0);

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapFullWithEmptyContainersMoveTypeSpec() =
      SwapFullWithEmptyContainersMoveTypeSpec{};
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}}, // 6GB
          {"host1", {"task2"}}, // 3GB
          {"host2", {"task3"}}, // 3GB
          {"host3", {}}, // Empty
          {"host4", {}}, // Empty
      });

  std::map<std::string, double> memory = {
      {"task0", 3.0},
      {"task1", 3.0},
      {"task2", 3.0},
      {"task3", 3.0},
  };
  solver.addObjectDimension("memory", memory);

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 12.0;
  capacitySpec.limit() = limit;
  solver.addConstraint(capacitySpec);

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
  std::cout
      << "=== Quick example showing basic SwapFullWithEmpty usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
