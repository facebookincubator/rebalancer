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
 * SingleRandomObjectStratified Move Type Reference Examples
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
  // Stratified object sampling across size classes
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
      });

  // Define object groups
  std::unordered_map<std::string, std::vector<std::string>> sizeGroups = {
      {"small", {"task0", "task1", "task2"}},
      {"medium", {"task3"}},
      {"large", {"task4"}},
  };
  solver.addPartition("size_class", std::move(sizeGroups));

  LocalSearchSolverSpec solverSpec;

  SingleRandomObjectStratifiedMoveTypeSpec stratifiedSpec;

  GroupList groupList;
  groupList.partitionName() = "size_class";

  ObjectsFromGroupsSpec objectsFromGroupsSpec;
  objectsFromGroupsSpec.groupList() = std::move(groupList);

  ObjectsToExploreOptions objectsToExploreOptions;
  objectsToExploreOptions.objectsFromGroupsSpec() = objectsFromGroupsSpec;
  stratifiedSpec.objectsToExploreOptions() = std::move(objectsToExploreOptions);

  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 300; // 300 objects per size class
  stratifiedSpec.stratifiedSampleSize() = std::move(sampleSize);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomObjectStratifiedMoveTypeSpec() =
      stratifiedSpec;

  solver.addSolver(solverSpec);
  // quick_example_end

  std::map<std::string, double> cpu = {
      {"task0", 1.0},
      {"task1", 1.0},
      {"task2", 2.0},
      {"task3", 3.0},
      {"task4", 3.0},
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

// size_stratified_start
void sizeStratified() {
  // Ensure coverage across object size classes
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> sizeGroups = {
      {"very_small", {}}, // Objects populated elsewhere
      {"small", {}},
      {"medium", {}},
      {"large", {}},
      {"very_large", {}},
  };
  solver.addPartition("size_class", std::move(sizeGroups));

  LocalSearchSolverSpec solverSpec;

  SingleRandomObjectStratifiedMoveTypeSpec stratifiedSpec;

  GroupList groupList;
  groupList.partitionName() = "size_class";

  ObjectsFromGroupsSpec objectsFromGroupsSpec;
  objectsFromGroupsSpec.groupList() = std::move(groupList);

  ObjectsToExploreOptions objectsToExploreOptions;
  objectsToExploreOptions.objectsFromGroupsSpec() = objectsFromGroupsSpec;
  stratifiedSpec.objectsToExploreOptions() = std::move(objectsToExploreOptions);

  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 300; // Sample 300 per size class
  stratifiedSpec.stratifiedSampleSize() = std::move(sampleSize);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomObjectStratifiedMoveTypeSpec() =
      stratifiedSpec;

  solver.addSolver(solverSpec);
  // size_stratified_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3", "task4"}},
          {"host1", {}},
          {"host2", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 0.5},
      {"task1", 1.0},
      {"task2", 2.0},
      {"task3", 3.0},
      {"task4", 4.0},
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

// type_stratified_start
void typeStratified() {
  // Stratify by object type/workload
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> workloadGroups = {
      {"batch", {}}, // Batch processing objects
      {"realtime", {}}, // Real-time objects
      {"ml_training", {}}, // ML training objects
      {"serving", {}}, // Serving objects
  };
  solver.addPartition("workload_type", std::move(workloadGroups));

  LocalSearchSolverSpec solverSpec;

  SingleRandomObjectStratifiedMoveTypeSpec stratifiedSpec;

  GroupList groupList;
  groupList.partitionName() = "workload_type";

  ObjectsFromGroupsSpec objectsFromGroupsSpec;
  objectsFromGroupsSpec.groupList() = std::move(groupList);

  ObjectsToExploreOptions objectsToExploreOptions;
  objectsToExploreOptions.objectsFromGroupsSpec() = objectsFromGroupsSpec;
  stratifiedSpec.objectsToExploreOptions() = std::move(objectsToExploreOptions);

  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 200; // Sample 200 per workload type
  stratifiedSpec.stratifiedSampleSize() = std::move(sampleSize);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomObjectStratifiedMoveTypeSpec() =
      stratifiedSpec;

  solver.addSolver(solverSpec);
  // type_stratified_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3"}},
          {"host1", {}},
          {"host2", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 1.0},
      {"task1", 2.0},
      {"task2", 3.0},
      {"task3", 1.5},
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

// large_scale_start
void largeScale() {
  // Very large: 3M objects across 5 size groups
  // Sample 300 per group = 1500 total (2000x reduction)
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> sizeGroups = {
      {"xs", {}}, // Very small objects
      {"s", {}}, // Small objects
      {"m", {}}, // Medium objects
      {"l", {}}, // Large objects
      {"xl", {}}, // Very large objects
  };
  solver.addPartition("size_class", std::move(sizeGroups));

  LocalSearchSolverSpec solverSpec;

  SingleRandomObjectStratifiedMoveTypeSpec stratifiedSpec;

  GroupList groupList;
  groupList.partitionName() = "size_class";

  ObjectsFromGroupsSpec objectsFromGroupsSpec;
  objectsFromGroupsSpec.groupList() = std::move(groupList);

  ObjectsToExploreOptions objectsToExploreOptions;
  objectsToExploreOptions.objectsFromGroupsSpec() = objectsFromGroupsSpec;
  stratifiedSpec.objectsToExploreOptions() = std::move(objectsToExploreOptions);

  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 300; // Small sample, many strata
  stratifiedSpec.stratifiedSampleSize() = std::move(sampleSize);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomObjectStratifiedMoveTypeSpec() =
      stratifiedSpec;

  solver.addSolver(solverSpec);
  // large_scale_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3", "task4"}},
          {"host1", {}},
          {"host2", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 0.5},
      {"task1", 1.0},
      {"task2", 2.0},
      {"task3", 3.0},
      {"task4", 4.0},
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

// priority_stratified_start
void priorityStratified() {
  // Stratify by priority level
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> priorityGroups = {
      {"critical", {}}, // P0 critical objects
      {"high", {}}, // P1 high priority
      {"medium", {}}, // P2 medium priority
      {"low", {}}, // P3 low priority
  };
  solver.addPartition("priority", std::move(priorityGroups));

  LocalSearchSolverSpec solverSpec;

  SingleRandomObjectStratifiedMoveTypeSpec stratifiedSpec;

  GroupList groupList;
  groupList.partitionName() = "priority";

  ObjectsFromGroupsSpec objectsFromGroupsSpec;
  objectsFromGroupsSpec.groupList() = std::move(groupList);

  ObjectsToExploreOptions objectsToExploreOptions;
  objectsToExploreOptions.objectsFromGroupsSpec() = objectsFromGroupsSpec;
  stratifiedSpec.objectsToExploreOptions() = std::move(objectsToExploreOptions);

  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 100; // Default sample
  stratifiedSpec.stratifiedSampleSize() = std::move(sampleSize);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomObjectStratifiedMoveTypeSpec() =
      stratifiedSpec;

  // Add Single for final exhaustive pass
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solver.addSolver(solverSpec);
  // priority_stratified_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3"}},
          {"host1", {}},
          {"host2", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 1.0},
      {"task1", 1.5},
      {"task2", 2.0},
      {"task3", 0.5},
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

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout
      << "=== Quick example showing basic SingleRandomObjectStratified usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
