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
 * GroupCapacitySpec Reference Examples

This file demonstrates all the usage patterns for GroupCapacitySpec shown in the
reference documentation. Each function is a complete, runnable example.
 *
 * This file demonstrates all the usage patterns for GroupCapacitySpec shown in
the
 * reference documentation. Each function is a complete, runnable example.
 */

// example_start
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
  /**
   * Quick example showing basic GroupCapacitySpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3", "task4", "task5"}},
      });
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"web_service", {"task0", "task1"}},
          {"api_service", {"task2", "task3"}},
          {"db_service", {"task4", "task5"}},
      });

  // quick_example_start
  // Limit each service to max 100 GB total across all hosts
  GroupCapacitySpec spec;
  spec.name() = "service-total-capacity";
  spec.scope() = "host";
  spec.partitionName() = "service";
  spec.bound() = GroupCapacitySpecBound::MAX;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.groupLimits() = {
      {"web_service", 100.0}, // Max 100 GB total
      {"api_service", 200.0}, // Max 200 GB total
      {"db_service", 500.0}, // Max 500 GB total
  };
  spec.limit() = limit;

  Limit contribution;
  contribution.type() = LimitType::ABSOLUTE;
  contribution.globalLimit() = 1.0; // Each object counts as 1 unit
  spec.contribution() = contribution;

  solver.addConstraint(spec);
  // quick_example_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "\n✓ All examples completed successfully!\n";
  return 0;
}
// example_end
