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
 * MinimizeNthLargestSpec Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void quick_example() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 3.0},
          {"task2", 1.0},
          {"task3", 4.0},
          {"task4", 2.0},
          {"task5", 3.0},
      });

  // quick_example_start
  // Minimize the 2nd largest host utilization
  MinimizeNthLargestUtilizationSpec spec;
  spec.name() = "minimize-2nd-highest";
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.n() = 1; // 0-based: 0=max, 1=2nd largest, 2=3rd largest

  solver.addGoal(spec, 1.0);
  // quick_example_end
}

void percentile_95() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 4.0},
          {"task1", 8.0},
          {"task2", 2.0},
          {"task3", 6.0},
          {"task4", 4.0},
          {"task5", 8.0},
      });

  // percentile_95_start
  // For 100 hosts, 95th percentile is the 5th largest value
  // n = ceil(100 * 0.05) - 1 = 4
  MinimizeNthLargestUtilizationSpec spec;
  spec.name() = "minimize-p95";
  spec.scope() = "host";
  spec.dimension() = "memory";
  spec.n() = 4; // 5th largest (0-based index)

  solver.addGoal(std::move(spec), 1.0);
  // percentile_95_end
}

void cap_top_3() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server0", {"shard0", "shard1", "shard2"}},
          {"server1", {"shard3"}},
          {"server2", {}},
          {"server3", {"shard4", "shard5"}},
      });
  solver.addObjectDimension(
      "disk",
      std::map<std::string, double>{
          {"shard0", 100.0},
          {"shard1", 200.0},
          {"shard2", 50.0},
          {"shard3", 150.0},
          {"shard4", 100.0},
          {"shard5", 200.0},
      });

  // cap_top_3_start
  // Minimize the 3rd highest server load
  MinimizeNthLargestUtilizationSpec spec;
  spec.name() = "cap-top-3-servers";
  spec.scope() = "server";
  spec.dimension() = "disk";
  spec.n() = 2; // 3rd largest (0-based)

  solver.addGoal(std::move(spec), 1.0);
  // cap_top_3_end
}

void with_target() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
          {"host2", {}},
          {"host3", {"task4", "task5"}},
      });
  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 2.0},
          {"task1", 3.0},
          {"task2", 1.0},
          {"task3", 4.0},
          {"task4", 2.0},
          {"task5", 3.0},
      });

  // with_target_start
  // Minimize 2nd largest, but don't go below 70% utilization
  MinimizeNthLargestUtilizationSpec spec;
  spec.name() = "minimize-with-target";
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.n() = 1;
  spec.targetUtilization() = 0.7; // Don't minimize below 70%

  solver.addGoal(std::move(spec), 1.0);
  // with_target_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all MinimizeNthLargest examples...\n";

  std::cout << "\n1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Percentile 95...\n";
  percentile_95();

  std::cout << "\n3. Cap Top 3...\n";
  cap_top_3();

  std::cout << "\n4. With Target...\n";
  with_target();

  std::cout << "\nAll MinimizeNthLargest examples completed successfully!\n";
  return 0;
}
