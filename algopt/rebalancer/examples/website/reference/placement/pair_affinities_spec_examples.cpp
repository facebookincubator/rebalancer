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
 * PairAffinitiesSpec Reference Examples

This file demonstrates all the usage patterns for PairAffinitiesSpec shown in
the reference documentation. Each function is a complete, runnable example.
 *
 * This file demonstrates all the usage patterns for PairAffinitiesSpec shown in
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
   * Quick example showing basic PairAffinitiesSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("service");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"frontend_0", "backend_0"}},
          {"host1", {"frontend_1", "backend_1"}},
      });

  // quick_example_start
  // Encourage frontend-backend pairs to colocate
  std::vector<PairAffinity> affinities;

  ObjectPair pair1;
  pair1.object1() = "frontend_0";
  pair1.object2() = "backend_0";
  PairAffinity aff1;
  aff1.pair() = pair1;
  aff1.affinity() = 10.0; // Strong affinity
  affinities.push_back(aff1);

  ObjectPair pair2;
  pair2.object1() = "frontend_1";
  pair2.object2() = "backend_1";
  PairAffinity aff2;
  aff2.pair() = pair2;
  aff2.affinity() = 10.0;
  affinities.push_back(aff2);

  PairAffinitiesSpec spec;
  spec.name() = "frontend-backend-affinity";
  spec.scope() = "host";
  spec.affinities() = affinities;
  spec.limit() = 1.0; // Default normalization

  solver.addGoal(spec, 3.0);
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
