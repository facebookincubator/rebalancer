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
 * Tuning Goal Weights for Conflicting Goals
 *
 * This example shows how to tune the weight ratio between colocation and
 * diversity goals to achieve the desired balance.
 *
 * Use this when: You need to prioritize either performance (colocation) or
 * fault tolerance (diversity) based on your specific requirements.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/init/Init.h>

#include <iostream>

using namespace facebook::rebalancer::interface;

void tuneConflictingGoals(ProblemSolver& solver) {
  // variation_start
  // Colocation goal - keep related services together
  ColocateGroupsSpec colocation;
  colocation.name() = "colocate-services";
  colocation.scope() = "host";
  colocation.partitionName() = "service_pairs";
  colocation.bound() = ColocateGroupsSpecBound::MAX;
  Limit colocLimit;
  colocLimit.type() = LimitType::ABSOLUTE;
  colocLimit.globalLimit() = 1; // All instances on same host
  colocation.limits() = std::move(colocLimit);

  // Diversity goal - spread replicas across failure domains
  ColocateGroupsSpec diversity;
  diversity.name() = "spread-replicas";
  diversity.scope() = "rack";
  diversity.partitionName() = "replica_sets";
  diversity.bound() = ColocateGroupsSpecBound::MIN; // MIN = spread across
  Limit divLimit;
  divLimit.type() = LimitType::ABSOLUTE;
  divLimit.globalLimit() = 3; // Spread across at least 3 racks
  diversity.limits() = std::move(divLimit);

  // Prioritize colocation (accept less diversity)
  solver.addGoal(std::move(colocation), 10.0); // High weight
  solver.addGoal(std::move(diversity), 1.0); // Low weight

  // Or prioritize diversity (accept less colocation):
  // solver.addGoal(colocation, 2.0);  // Low weight
  // solver.addGoal(diversity, 8.0);   // High weight
  // variation_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Conflicting goals tuning variation\n";
  std::cout << "Adjust weights to prioritize colocation vs diversity\n";
  return 0;
}
