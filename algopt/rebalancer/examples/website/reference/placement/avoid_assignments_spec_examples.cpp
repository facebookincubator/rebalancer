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
 * AvoidAssignmentsSpec Reference Examples

This file demonstrates all the usage patterns for AvoidAssignmentsSpec shown in
the reference documentation. Each function is a complete, runnable example.
 *
 * This file demonstrates all the usage patterns for AvoidAssignmentsSpec shown
in the
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
   * Quick example showing basic AvoidAssignmentsSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("workload");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"cpu_only_host_1", {"gpu_workload_1"}},
          {"cpu_only_host_2", {"gpu_workload_2"}},
      });

  // quick_example_start
  // Prevent specific GPU workloads from CPU-only hosts
  std::vector<AvoidAssignment> assignments;

  AvoidAssignment assign1;
  assign1.object() = "gpu_workload_1";
  assign1.scopeItems() = {"cpu_only_host_1", "cpu_only_host_2"};
  assignments.push_back(assign1);

  AvoidAssignment assign2;
  assign2.object() = "gpu_workload_2";
  assign2.scopeItems() = {"cpu_only_host_1", "cpu_only_host_2"};
  assignments.push_back(assign2);

  AvoidAssignmentsSpec spec;
  spec.name() = "gpu-workload-restrictions";
  spec.scope() = "host";
  spec.assignments() = assignments;

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
