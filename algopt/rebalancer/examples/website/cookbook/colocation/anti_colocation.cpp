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
 * Anti-Colocation - Keeping Services Apart
 *
 * This variation demonstrates how to prevent conflicting services from sharing
 * hosts to avoid resource contention.
 *
 * Use this when: Services compete for the same resources (CPU, I/O, network)
 * and should be separated to prevent performance degradation.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void addAntiColocation(ProblemSolver& solver) {
  // variation_start
  // Define conflicting service pairs (resource contention)
  std::map<std::string, std::vector<std::string>> conflict_partition = {
      {"conflict_0", {"heavy_cpu_service_0", "heavy_cpu_service_1"}},
      {"conflict_1", {"heavy_io_service_0", "heavy_io_service_1"}},
  };
  solver.addPartition("conflicts", std::move(conflict_partition));

  // Spread conflicting services (MIN = keep apart)
  ColocateGroupsSpec separate_conflicts;
  separate_conflicts.name() = "separate-conflicts";
  separate_conflicts.scope() = "host";
  separate_conflicts.partitionName() = "conflicts";
  separate_conflicts.bound() = ColocateGroupsSpecBound::MIN;
  Limit conflict_limit;
  conflict_limit.type() = LimitType::ABSOLUTE;
  conflict_limit.globalLimit() = 2;
  separate_conflicts.limits() = std::move(conflict_limit);
  solver.addGoal(
      std::move(separate_conflicts),
      7.0); // High weight - avoid resource contention
  // variation_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Anti-colocation variation\n";
  std::cout << "Prevent conflicting services from sharing hosts\n";
  return 0;
}
