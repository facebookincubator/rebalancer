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
 * Load Balancing with Optimal Solver
 *
 * For smaller problems (<50 objects), use the optimal solver to get
 * provably optimal solutions.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "load-balancer", "production");

  solver.setObjectName("task");
  solver.setContainerName("host");

  // Smaller problem for optimal solver
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(15)) {
    assignment["host0"].push_back(fmt::format("task{}", i));
  }
  for (const auto i : folly::irange(15, 25)) {
    assignment["host1"].push_back(fmt::format("task{}", i));
  }
  for (const auto i : folly::irange(25, 40)) {
    assignment["host2"].push_back(fmt::format("task{}", i));
  }
  assignment["host3"] = {};
  assignment["host4"] = {};
  solver.setAssignment(assignment);

  std::map<std::string, double> cpu_usage;
  for (const auto i : folly::irange(40)) {
    cpu_usage[fmt::format("task{}", i)] = 1.0 + (i % 5) * 0.5;
  }
  solver.addObjectDimension("cpu_usage", cpu_usage);

  std::map<std::string, double> memory_usage;
  for (const auto i : folly::irange(40)) {
    memory_usage[fmt::format("task{}", i)] = 2.0 + (i % 3) * 1.0;
  }
  solver.addObjectDimension("memory_usage", memory_usage);

  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;
  solver.addGoal(balanceCpu, 1.0);

  BalanceSpec balanceMem;
  balanceMem.name() = "balance-memory";
  balanceMem.scope() = "host";
  balanceMem.dimension() = "memory_usage";
  balanceMem.formula() = BalanceSpecFormula::LEGACY;
  balanceMem.fixAverageToInitial() = true;
  solver.addGoal(balanceMem, 1.0);

  // variation_start
  OptimalSolverSpec optimal;
  optimal.solverPackage() = OptimalSolverPackage::HIGHS;
  optimal.solveTime() = 60000; // 1 minute
  solver.addSolver(optimal);
  // variation_end

  auto solution = solver.solve();

  std::cout << "AssignmentSolution found in " << "<solve time>" << "ms\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";

  return 0;
}
