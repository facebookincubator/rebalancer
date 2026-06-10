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
 * SingleRandomStratified Move Type Reference Examples
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
  // Stratified sampling across regions
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  // Setup problem data before configuring solver spec
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {"task2"}},
          {"host3", {}},
      });

  solver.addScope(
      "region",
      std::map<std::string, std::string>{
          {"host0", "region0"},
          {"host1", "region0"},
          {"host2", "region1"},
          {"host3", "region1"},
      });

  const std::map<std::string, double> cpu = {
      {"task0", 1.0},
      {"task1", 1.0},
      {"task2", 1.0},
  };
  solver.addObjectDimension("cpu", cpu);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(balanceSpec, 1.0);

  LocalSearchSolverSpec solverSpec;

  SingleRandomStratifiedMoveTypeSpec stratifiedSpec;

  MoveToCurrentScopeItemSpec moveSpec;
  moveSpec.scopeNameForExploringMovesToCurrentScopeItem() = "region";

  DestinationsToExploreOptions destOptions;
  destOptions.moveToCurrentScopeItem() = moveSpec;
  stratifiedSpec.destinationsToExplore() = destOptions;

  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 100; // 100 samples per region
  stratifiedSpec.stratifiedSampleSize() = sampleSize;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomStratifiedMoveTypeSpec() =
      stratifiedSpec;

  solver.addSolver(solverSpec);
  // quick_example_end

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

// region_stratified_start
void regionStratified() {
  // Ensure coverage across all regions
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SingleRandomStratifiedMoveTypeSpec stratifiedSpec;

  MoveToCurrentScopeItemSpec moveSpec;
  moveSpec.scopeNameForExploringMovesToCurrentScopeItem() = "region";

  DestinationsToExploreOptions destOptions;
  destOptions.moveToCurrentScopeItem() = moveSpec;
  stratifiedSpec.destinationsToExplore() = std::move(destOptions);

  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 50; // Sample 50 containers per region
  stratifiedSpec.stratifiedSampleSize() = std::move(sampleSize);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomStratifiedMoveTypeSpec() =
      stratifiedSpec;

  solver.addSolver(solverSpec);
  // region_stratified_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {}},
          {"host2", {"task3"}},
          {"host3", {}},
      });

  solver.addScope(
      "region",
      std::map<std::string, std::string>{
          {"host0", "region0"},
          {"host1", "region0"},
          {"host2", "region1"},
          {"host3", "region1"},
      });

  std::map<std::string, double> cpu = {
      {"task0", 2.0},
      {"task1", 1.5},
      {"task2", 1.5},
      {"task3", 1.0},
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
  // Stratify by container size class
  // Assumes containers are organized into size-based scope items
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SingleRandomStratifiedMoveTypeSpec stratifiedSpec;

  MoveToCurrentScopeItemSpec moveSpec;
  moveSpec.scopeNameForExploringMovesToCurrentScopeItem() = "size_class";

  DestinationsToExploreOptions destOptions;
  destOptions.moveToCurrentScopeItem() = moveSpec;
  stratifiedSpec.destinationsToExplore() = destOptions;

  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 100; // 100 samples per size class
  stratifiedSpec.stratifiedSampleSize() = sampleSize;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomStratifiedMoveTypeSpec() =
      stratifiedSpec;

  solver.addSolver(solverSpec);
  // size_stratified_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {"task2"}},
          {"host3", {}},
      });

  solver.addScope(
      "size_class",
      std::map<std::string, std::string>{
          {"host0", "small"},
          {"host1", "small"},
          {"host2", "large"},
          {"host3", "large"},
      });

  std::map<std::string, double> cpu = {
      {"task0", 1.0},
      {"task1", 1.0},
      {"task2", 1.0},
  };
  solver.addObjectDimension("cpu", cpu);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(balanceSpec, 1.0);

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
  // Large-scale: 100K containers across 10 regions
  // Sample 50 per region = 500 total evaluations
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SingleRandomStratifiedMoveTypeSpec stratifiedSpec;

  MoveToCurrentScopeItemSpec moveSpec;
  moveSpec.scopeNameForExploringMovesToCurrentScopeItem() = "region";

  DestinationsToExploreOptions destOptions;
  destOptions.moveToCurrentScopeItem() = moveSpec;
  stratifiedSpec.destinationsToExplore() = destOptions;

  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 50; // Small sample, many strata
  stratifiedSpec.stratifiedSampleSize() = sampleSize;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomStratifiedMoveTypeSpec() =
      stratifiedSpec;

  solver.addSolver(solverSpec);
  // large_scale_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3"}},
          {"host1", {}},
          {"host2", {"task4"}},
          {"host3", {}},
      });

  solver.addScope(
      "region",
      std::map<std::string, std::string>{
          {"host0", "region0"},
          {"host1", "region0"},
          {"host2", "region1"},
          {"host3", "region1"},
      });

  std::map<std::string, double> cpu = {
      {"task0", 2.0},
      {"task1", 1.5},
      {"task2", 2.5},
      {"task3", 1.0},
      {"task4", 2.0},
  };
  solver.addObjectDimension("cpu", cpu);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(balanceSpec, 1.0);

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

// adaptive_sample_start
void adaptiveSample() {
  // Start with small samples, increase later
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // Stage 1: Quick exploration
  LocalSearchSolverSpec stage1;

  SingleRandomStratifiedMoveTypeSpec stratifiedSpec1;

  MoveToCurrentScopeItemSpec moveSpec1;
  moveSpec1.scopeNameForExploringMovesToCurrentScopeItem() = "region";

  DestinationsToExploreOptions destOptions1;
  destOptions1.moveToCurrentScopeItem() = moveSpec1;
  stratifiedSpec1.destinationsToExplore() = std::move(destOptions1);

  SampleSize sampleSize1;
  sampleSize1.defaultSampleSize() = 25; // Quick initial pass
  stratifiedSpec1.stratifiedSampleSize() = std::move(sampleSize1);

  stage1.moveTypeList()->emplace_back();
  stage1.moveTypeList()->back().singleRandomStratifiedMoveTypeSpec() =
      stratifiedSpec1;

  solver.addSolver(stage1);

  // Stage 2: Refined search
  LocalSearchSolverSpec stage2;

  SingleRandomStratifiedMoveTypeSpec stratifiedSpec2;

  MoveToCurrentScopeItemSpec moveSpec2;
  moveSpec2.scopeNameForExploringMovesToCurrentScopeItem() = "region";

  DestinationsToExploreOptions destOptions2;
  destOptions2.moveToCurrentScopeItem() = moveSpec2;
  stratifiedSpec2.destinationsToExplore() = std::move(destOptions2);

  SampleSize sampleSize2;
  sampleSize2.defaultSampleSize() = 100; // More thorough
  stratifiedSpec2.stratifiedSampleSize() = std::move(sampleSize2);

  stage2.moveTypeList()->emplace_back();
  stage2.moveTypeList()->back().singleRandomStratifiedMoveTypeSpec() =
      stratifiedSpec2;

  // Add Single for final exhaustive pass
  stage2.moveTypeList()->emplace_back();
  stage2.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solver.addSolver(stage2);
  // adaptive_sample_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
          {"host2", {"task2"}},
          {"host3", {}},
      });

  solver.addScope(
      "region",
      std::map<std::string, std::string>{
          {"host0", "region0"},
          {"host1", "region0"},
          {"host2", "region1"},
          {"host3", "region1"},
      });

  std::map<std::string, double> cpu = {
      {"task0", 1.0},
      {"task1", 1.0},
      {"task2", 1.0},
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
      << "=== Quick example showing basic SingleRandomStratified usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
