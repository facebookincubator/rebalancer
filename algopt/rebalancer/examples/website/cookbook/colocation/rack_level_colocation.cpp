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
 * Rack-Level Colocation
 *
 * This variation demonstrates how to colocate services at the rack level
 * instead of the host level, providing a balance between performance and fault
 * tolerance.
 *
 * Use this when: Host-level colocation is too strict, but you still want
 * services in the same rack for reduced network latency.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/init/Init.h>

#include <iostream>

using namespace facebook::rebalancer::interface;

void addRackLevelColocation(ProblemSolver& solver) {
  // variation_start
  // Colocate service pairs at rack level (lower latency within rack)
  ColocateGroupsSpec rack_coloc;
  rack_coloc.name() = "colocate-pairs-rack-level";
  rack_coloc.scope() = "rack";
  rack_coloc.partitionName() = "service_pair";
  rack_coloc.bound() = ColocateGroupsSpecBound::MAX;
  Limit rack_limit;
  rack_limit.type() = LimitType::ABSOLUTE;
  rack_limit.globalLimit() = 1;
  rack_coloc.limits() = std::move(rack_limit);
  solver.addGoal(std::move(rack_coloc), 5.0);

  // But spread each service across racks (fault tolerance)
  ColocateGroupsSpec rack_spread;
  rack_spread.name() = "spread-services-rack-level";
  rack_spread.scope() = "rack";
  rack_spread.partitionName() = "service_type";
  rack_spread.bound() = ColocateGroupsSpecBound::MIN;
  Limit spread_rack_limit;
  spread_rack_limit.type() = LimitType::ABSOLUTE;
  spread_rack_limit.globalLimit() = 3;
  rack_spread.limits() = std::move(spread_rack_limit);
  solver.addGoal(std::move(rack_spread), 3.0);
  // variation_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Rack-level colocation variation\n";
  std::cout
      << "Colocate at rack level for balance of performance and fault tolerance\n";
  return 0;
}
