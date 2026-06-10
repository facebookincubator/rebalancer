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
 * SwapN Move Type Reference Examples
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
  // Swap 3 objects simultaneously to satisfy exact-limit constraints
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SwapNMoveTypeSpec swapNSpec;
  swapNSpec.swapNConcurrentObjects() = 3;
  swapNSpec.swapNSourceObjects() = {
      "req0", "req1", "req2", "req3", "req4", "req5"};
  swapNSpec.swapNDestinationScope() = {
      {"rack0", "rack1", "rack2"}, // Pod 0 racks
      {"rack3", "rack4", "rack5"}, // Pod 1 racks
  };
  swapNSpec.swapNIterations() = 1000;

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapNMoveTypeSpec() = swapNSpec;

  solver.addSolver(solverSpec);
}
// quick_example_end

void quickExampleComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("request");
  solver.setContainerName("rack");

  // Define pod scope
  std::map<std::string, std::string> podScope = {
      {"rack0", "pod0"},
      {"rack1", "pod0"},
      {"rack2", "pod0"},
      {"rack3", "pod1"},
      {"rack4", "pod1"},
      {"rack5", "pod1"},
  };
  solver.addScope("pod", podScope);

  // Setup problem with unassigned requests
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack0", {}},
          {"rack1", {}},
          {"rack2", {}},
          {"rack3", {}},
          {"rack4", {}},
          {"rack5", {}},
          {"unassigned", {"req0", "req1", "req2", "req3", "req4", "req5"}},
      });

  std::map<std::string, double> capacityDim = {
      {"req0", 1.0},
      {"req1", 1.0},
      {"req2", 1.0},
      {"req3", 1.0},
      {"req4", 1.0},
      {"req5", 1.0},
  };
  solver.addObjectDimension("capacity", capacityDim);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-capacity";
  balanceSpec.scope() = "rack";
  balanceSpec.dimension() = "capacity";
  solver.addGoal(balanceSpec, 1.0);

  LocalSearchSolverSpec solverSpec;
  SwapNMoveTypeSpec swapNSpec;
  swapNSpec.swapNConcurrentObjects() = 3;
  swapNSpec.swapNSourceObjects() = {
      "req0", "req1", "req2", "req3", "req4", "req5"};
  swapNSpec.swapNDestinationScope() = {
      {"rack0", "rack1", "rack2"},
      {"rack3", "rack4", "rack5"},
  };
  swapNSpec.swapNIterations() = 1000;
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapNMoveTypeSpec() = swapNSpec;
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

// exact_limit_start
void exactLimit() {
  // Exactly 3 requests per pod using SwapN
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("request");
  solver.setContainerName("rack");

  // Define pod scope
  std::map<std::string, std::string> podScope = {
      {"rack0", "pod0"},
      {"rack1", "pod0"},
      {"rack2", "pod0"},
      {"rack3", "pod1"},
      {"rack4", "pod1"},
      {"rack5", "pod1"},
  };
  solver.addScope("pod", podScope);

  LocalSearchSolverSpec solverSpec;

  SwapNMoveTypeSpec swapNSpec;
  swapNSpec.swapNConcurrentObjects() = 3;
  swapNSpec.swapNSourceObjects() = {
      "request0",
      "request1",
      "request2",
      "request3",
      "request4",
      "request5",
  };
  swapNSpec.swapNDestinationScope() = {
      {"rack0", "rack1", "rack2"}, // Pod 0
      {"rack3", "rack4", "rack5"}, // Pod 1
  };

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapNMoveTypeSpec() = swapNSpec;

  solver.addSolver(solverSpec);
}
// exact_limit_end

void exactLimitComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("request");
  solver.setContainerName("rack");

  // Define pod scope
  std::map<std::string, std::string> podScope = {
      {"rack0", "pod0"},
      {"rack1", "pod0"},
      {"rack2", "pod0"},
      {"rack3", "pod1"},
      {"rack4", "pod1"},
      {"rack5", "pod1"},
  };
  solver.addScope("pod", podScope);

  // Define request partition
  std::unordered_map<std::string, std::vector<std::string>> requestPartition = {
      {"group1",
       {"request0",
        "request1",
        "request2",
        "request3",
        "request4",
        "request5"}},
  };
  solver.addPartition("request_group", std::move(requestPartition));

  // Setup with unassigned requests
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack0", {}},
          {"rack1", {}},
          {"rack2", {}},
          {"rack3", {}},
          {"rack4", {}},
          {"rack5", {}},
          {"unassigned",
           {"request0",
            "request1",
            "request2",
            "request3",
            "request4",
            "request5"}},
      });

  std::map<std::string, double> sizeDim = {
      {"request0", 1.0},
      {"request1", 1.0},
      {"request2", 1.0},
      {"request3", 1.0},
      {"request4", 1.0},
      {"request5", 1.0},
  };
  solver.addObjectDimension("size", std::move(sizeDim));

  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "pod";
  groupCountSpec.dimension() = "request_count";
  groupCountSpec.partitionName() = "request_group";
  groupCountSpec.bound() = GroupCountSpecBound::EXACT;
  groupCountSpec.limit()->globalLimit() = 3;
  solver.addConstraint(std::move(groupCountSpec));

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-size";
  balanceSpec.scope() = "rack";
  balanceSpec.dimension() = "size";
  solver.addGoal(std::move(balanceSpec), 1.0);

  LocalSearchSolverSpec solverSpec;
  SwapNMoveTypeSpec swapNSpec;
  swapNSpec.swapNConcurrentObjects() = 3;
  swapNSpec.swapNSourceObjects() = {
      "request0",
      "request1",
      "request2",
      "request3",
      "request4",
      "request5",
  };
  swapNSpec.swapNDestinationScope() = {
      {"rack0", "rack1", "rack2"},
      {"rack3", "rack4", "rack5"},
  };
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapNMoveTypeSpec() = swapNSpec;
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

// crp_start
void crp() {
  // Capacity Request Portal: allocate hosts with exact pod constraints
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("host");
  solver.setContainerName("rack");

  // Define pod scope
  std::map<std::string, std::string> podScope = {
      {"rack0", "pod0"},
      {"rack1", "pod0"},
      {"rack2", "pod1"},
      {"rack3", "pod1"},
  };
  solver.addScope("pod", podScope);

  LocalSearchSolverSpec solverSpec;

  SwapNMoveTypeSpec swapNSpec;
  swapNSpec.swapNConcurrentObjects() = 3; // 3 hosts per allocation
  swapNSpec.swapNSourceObjects() = {
      "host0", "host1", "host2", "host3", "host4", "host5"};
  swapNSpec.swapNDestinationScope() = {
      {"rack0", "rack1"}, // Pod 0 racks
      {"rack2", "rack3"}, // Pod 1 racks
  };
  swapNSpec.swapNIterations() = 1000000; // 1M iterations

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapNMoveTypeSpec() = swapNSpec;

  solver.addSolver(solverSpec);
}
// crp_end

// limited_iterations_start
void limitedIterations() {
  // Use fewer iterations for faster (but lower quality) results
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  LocalSearchSolverSpec solverSpec;

  SwapNMoveTypeSpec swapNSpec;
  swapNSpec.swapNConcurrentObjects() = 2;
  swapNSpec.swapNSourceObjects() = {"obj0", "obj1", "obj2", "obj3"};
  swapNSpec.swapNDestinationScope() = {
      {"container0", "container1"},
      {"container2", "container3"},
  };
  swapNSpec.swapNIterations() = 100; // Reduced from default 1M

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().swapNMoveTypeSpec() = swapNSpec;

  solver.addSolver(solverSpec);
}
// limited_iterations_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "=== Quick example showing basic SwapN usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
