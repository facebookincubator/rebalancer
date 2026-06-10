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
 * NonAcceptingSpec Reference Examples

This file demonstrates all the usage patterns for NonAcceptingSpec shown in the
reference documentation. Each function is a complete, runnable example.
 *
 * This file demonstrates all the usage patterns for NonAcceptingSpec shown in
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
   * Quick example showing basic NonAcceptingSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host5", {"task0"}},
          {"host8", {"task1"}},
          {"host12", {"task2"}},
          {"host13", {"task3"}},
      });

  // quick_example_start
  // Prevent new objects on hosts under maintenance
  NonAcceptingSpec spec;
  spec.name() = "block-new-on-maintenance";
  spec.scope() = "host";
  spec.items() = {"host5", "host8", "host12"};

  solver.addConstraint(spec);
  // quick_example_end
}

void gradual_host_draining() {
  /**
   * Prevent new placements as first step in draining.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host1", {"task0"}},
          {"host2", {"task1"}},
          {"host3", {"task2"}},
      });

  // gradual_host_draining_start
  // Step 1: Block new placements
  std::vector<std::string> hosts_to_drain = {"host1", "host2"};
  NonAcceptingSpec spec;
  spec.name() = "block-new-placements";
  spec.scope() = "host";
  spec.items() = std::move(hosts_to_drain);

  solver.addConstraint(std::move(spec));

  // Step 2 (later): Actively drain existing objects
  // solver.addConstraint(ToFreeSpec(containers=hosts_to_drain))
  // gradual_host_draining_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "2. Gradual Host Draining...\n";
  gradual_host_draining();

  std::cout << "\n✓ All examples completed successfully!\n";
  return 0;
}
// example_end
