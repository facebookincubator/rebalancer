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
 * ScopeAffinitiesSpec Reference Examples

This file demonstrates all the usage patterns for ScopeAffinitiesSpec shown in
the reference documentation. Each function is a complete, runnable example.
 *
 * This file demonstrates all the usage patterns for ScopeAffinitiesSpec shown
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
   * Quick example showing basic ScopeAffinitiesSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("workload");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"premium_host_1", {"workload0"}},
          {"premium_host_2", {"workload1"}},
          {"standard_host_1", {"workload2"}},
          {"standard_host_2", {"workload3"}},
      });
  solver.addObjectDimension(
      "priority",
      std::map<std::string, double>{
          {"workload0", 10.0},
          {"workload1", 10.0},
          {"workload2", 1.0},
          {"workload3", 1.0},
      });

  // quick_example_start
  // High-priority workloads prefer premium hosts
  // Affinity scores for containers
  std::map<std::string, double> host_affinities = {
      {"premium_host_1", 10.0}, // High affinity
      {"premium_host_2", 10.0},
      {"standard_host_1", 1.0}, // Low affinity
      {"standard_host_2", 1.0}};

  ScopeAffinitiesSpec spec;
  spec.name() = "priority-affinity";
  spec.scope() = "host";
  spec.dimension() = "priority"; // Object dimension
  spec.affinities() = host_affinities;

  solver.addGoal(spec, 2.0);
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
