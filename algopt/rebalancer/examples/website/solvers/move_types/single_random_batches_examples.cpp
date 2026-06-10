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
 * SingleRandomBatches Move Type Reference Examples
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
  // Batched random evaluation with parallelism
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  SingleRandomBatchesMoveTypeSpec randomBatchesSpec;
  randomBatchesSpec.randomContainerBatchSize() =
      10; // Evaluate 10 containers per batch

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomBatchesMoveTypeSpec() =
      randomBatchesSpec;

  solver.addSolver(solverSpec);
  // quick_example_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {}},
          {"host2", {}},
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
  // Very large problem: 100K objects, 10K containers
  // Small batches for fast early exit
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SingleRandomBatchesMoveTypeSpec randomBatchesSpec;
  randomBatchesSpec.randomContainerBatchSize() = 10; // Quick early exit

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomBatchesMoveTypeSpec() =
      randomBatchesSpec;

  solver.addSolver(solverSpec);
  // large_scale_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3"}},
          {"host1", {}},
          {"host2", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 2.0},
      {"task1", 1.5},
      {"task2", 2.5},
      {"task3", 1.0},
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

// batch_tuning_start
void batchTuning() {
  // Adjust batch size for your workload
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  // Small batch: faster early exit, less parallelism
  LocalSearchSolverSpec stage1;

  SingleRandomBatchesMoveTypeSpec smallBatchSpec;
  smallBatchSpec.randomContainerBatchSize() = 5; // Exit after 5 evaluations

  stage1.moveTypeList()->emplace_back();
  stage1.moveTypeList()->back().singleRandomBatchesMoveTypeSpec() =
      smallBatchSpec;

  solver.addSolver(stage1);

  // Large batch: more parallelism, better moves
  LocalSearchSolverSpec stage2;

  SingleRandomBatchesMoveTypeSpec largeBatchSpec;
  largeBatchSpec.randomContainerBatchSize() = 50; // Utilize more cores

  stage2.moveTypeList()->emplace_back();
  stage2.moveTypeList()->back().singleRandomBatchesMoveTypeSpec() =
      largeBatchSpec;

  solver.addSolver(stage2);
  // batch_tuning_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 1.0},
      {"task1", 1.0},
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

// multicore_start
void multicore() {
  // Large batch size to utilize 16 cores
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SingleRandomBatchesMoveTypeSpec randomBatchesSpec;
  randomBatchesSpec.randomContainerBatchSize() = 32; // 2x number of cores

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomBatchesMoveTypeSpec() =
      randomBatchesSpec;

  solver.addSolver(solverSpec);
  // multicore_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {}},
          {"host2", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 2.0},
      {"task1", 1.5},
      {"task2", 1.5},
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

// combined_start
void combined() {
  // Stage 1: Fast improvement with batches
  // Stage 2: Refinement with full Single
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  // Add SingleRandomBatches
  SingleRandomBatchesMoveTypeSpec randomBatchesSpec;
  randomBatchesSpec.randomContainerBatchSize() = 10; // Quick wins

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleRandomBatchesMoveTypeSpec() =
      randomBatchesSpec;

  // Add Single for refinement
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  solver.addSolver(solverSpec);
  // combined_end

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2", "task3", "task4"}},
          {"host1", {}},
          {"host2", {}},
      });

  std::map<std::string, double> cpu = {
      {"task0", 2.0},
      {"task1", 1.5},
      {"task2", 2.5},
      {"task3", 1.0},
      {"task4", 2.0},
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
      << "=== Quick example showing basic SingleRandomBatches usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
