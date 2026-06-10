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
 * Adjusting Weight Balance
 *
 * This example shows how to increase colocation weight when services aren't
 * colocating as expected.
 *
 * Use this when: The solver isn't achieving enough colocation due to competing
 * goals with higher weights.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/init/Init.h>

#include <iostream>

using namespace facebook::rebalancer::interface;

void adjustWeightBalance(ProblemSolver& solver) {
  // variation_start
  // If colocation not happening, increase weight
  ColocateGroupsSpec colocate;
  colocate.name() = "service-colocation";
  colocate.scope() = "host";
  colocate.partitionName() = "service_pair";
  // limits defaults to globalLimit = 1 (all instances on same host)
  solver.addGoal(std::move(colocate), 10.0); // Increase from 5.0
  // variation_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Weight balance variation\n";
  std::cout << "Increase colocation weight to achieve better results\n";
  return 0;
}
