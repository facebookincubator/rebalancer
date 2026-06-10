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
 * SwapSampled Move Type Reference Examples
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
  // Use SwapMoveTypeSpec with sampling for large problems
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  // Add SwapSampled
  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 100;
  swapSpec.sampleSize() = sampleSize;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;

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
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 100;
  swapSpec.sampleSize() = std::move(sampleSize);
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;
  solver.addSolver(solverSpec);

  // Setup capacity-constrained problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}}, // 12GB
          {"host1", {"task3", "task4"}}, // 8GB
          {"host2", {"task5"}}, // 4GB
      });

  std::map<std::string, double> memory = {
      {"task0", 4.0},
      {"task1", 4.0},
      {"task2", 4.0},
      {"task3", 4.0},
      {"task4", 4.0},
      {"task5", 4.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 16.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
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

// basic_sampling_start
void basicSampling() {
  // Reasonable sample size for most large problems
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 100;
  swapSpec.sampleSize() = std::move(sampleSize);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// basic_sampling_end

void basicSamplingComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 100;
  swapSpec.sampleSize() = std::move(sampleSize);
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}}, // 8GB - full
          {"host1", {"task2", "task3"}}, // 8GB - full
          {"host2", {"task4"}}, // 4GB
      });

  std::map<std::string, double> memory = {
      {"task0", 4.0},
      {"task1", 4.0},
      {"task2", 4.0},
      {"task3", 4.0},
      {"task4", 4.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 8.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
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

// per_object_sampling_start
void perObjectSampling() {
  // Higher sample size for important objects
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 50;
  sampleSize.objectToSampleSize() = {
      {"critical_obj1", 200},
      {"critical_obj2", 200},
  };
  swapSpec.sampleSize() = std::move(sampleSize);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// per_object_sampling_end

void perObjectSamplingComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 50;
  sampleSize.objectToSampleSize() = {
      {"critical_obj1", 200},
      {"critical_obj2", 200},
  };
  swapSpec.sampleSize() = std::move(sampleSize);
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}}, // 6GB
          {"host1", {"task2"}}, // 3GB
          {"host2", {"task3"}}, // 3GB
      });

  std::map<std::string, double> memory = {
      {"task0", 3.0},
      {"task1", 3.0},
      {"task2", 3.0},
      {"task3", 3.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 8.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
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

// large_problem_start
void largeProblem() {
  // Small sample for problems with 100K+ objects
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 50;
  swapSpec.sampleSize() = std::move(sampleSize);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// large_problem_end

void largeProblemComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 50;
  swapSpec.sampleSize() = std::move(sampleSize);
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}}, // 12GB
          {"host1", {"task3", "task4"}}, // 8GB
          {"host2", {"task5", "task6"}}, // 8GB
      });

  std::map<std::string, double> memory = {
      {"task0", 4.0},
      {"task1", 4.0},
      {"task2", 4.0},
      {"task3", 4.0},
      {"task4", 4.0},
      {"task5", 4.0},
      {"task6", 4.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 12.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
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

// greedy_sampling_start
void greedySampling() {
  // Sample + early exit = maximum speed
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 50;
  swapSpec.sampleSize() = std::move(sampleSize);
  swapSpec.greedyOnSrc() = true;
  swapSpec.greedyOnDst() = true;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;

  solver.addSolver(solverSpec);
}
// greedy_sampling_end

void greedySamplingComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  SwapMoveTypeSpec swapSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 50;
  swapSpec.sampleSize() = std::move(sampleSize);
  swapSpec.greedyOnSrc() = true;
  swapSpec.greedyOnDst() = true;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec;
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}}, // 8GB - full
          {"host1", {"task2", "task3"}}, // 8GB - full
          {"host2", {"task4", "task5"}}, // 8GB - full
      });

  std::map<std::string, double> memory = {
      {"task0", 4.0},
      {"task1", 4.0},
      {"task2", 4.0},
      {"task3", 4.0},
      {"task4", 4.0},
      {"task5", 4.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 8.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
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

// adaptive_sampling_start
void adaptiveSampling() {
  // Start with small sample, increase in later stages
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;

  // Add Single
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};

  // Add SwapSampled with moderate sample
  SwapMoveTypeSpec swapSpec1;
  SampleSize sampleSize1;
  sampleSize1.defaultSampleSize() = 100;
  swapSpec1.sampleSize() = std::move(sampleSize1);
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec1;

  // Add SwapSampled with larger sample for refinement
  SwapMoveTypeSpec swapSpec2;
  SampleSize sampleSize2;
  sampleSize2.defaultSampleSize() = 500;
  swapSpec2.sampleSize() = std::move(sampleSize2);
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec2;

  solver.addSolver(solverSpec);
}
// adaptive_sampling_end

void adaptiveSamplingComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().singleMoveTypeSpec() = SingleMoveTypeSpec{};
  SwapMoveTypeSpec swapSpec1;
  SampleSize sampleSize1;
  sampleSize1.defaultSampleSize() = 100;
  swapSpec1.sampleSize() = std::move(sampleSize1);
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec1;
  SwapMoveTypeSpec swapSpec2;
  SampleSize sampleSize2;
  sampleSize2.defaultSampleSize() = 500;
  swapSpec2.sampleSize() = std::move(sampleSize2);
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapMoveTypeSpec() = swapSpec2;
  solver.addSolver(solverSpec);

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}}, // 9GB
          {"host1", {"task3", "task4"}}, // 6GB
          {"host2", {"task5"}}, // 3GB
      });

  std::map<std::string, double> memory = {
      {"task0", 3.0},
      {"task1", 3.0},
      {"task2", 3.0},
      {"task3", 3.0},
      {"task4", 3.0},
      {"task5", 3.0},
  };
  solver.addObjectDimension("memory", std::move(memory));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "memory-capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  Limit limit;
  limit.globalLimit() = 12.0;
  capacitySpec.limit() = std::move(limit);
  solver.addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "host";
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

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "=== Quick example showing basic SwapSampled usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
