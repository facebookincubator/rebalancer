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
 * Partial Colocation - Subset of Services
 *
 * This variation demonstrates how to colocate only a subset of service pairs
 * (e.g., high-traffic pairs) while leaving others unconstrained.
 *
 * Use this when: Only some service pairs have strong colocation requirements
 * based on traffic patterns or business logic.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/container/irange.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void addPartialColocation(ProblemSolver& solver) {
  // variation_start
  // Colocate only high-traffic pairs (instances 0-9)
  std::map<std::string, std::vector<std::string>> high_traffic_pairs;
  for (const auto i : folly::irange(10)) { // Only first 10 pairs
    std::string pair_name = "frontend_api_pair_" + std::to_string(i);
    high_traffic_pairs[pair_name] = {
        "frontend_instance_" + std::to_string(i),
        "api_instance_" + std::to_string(i),
    };
  }

  solver.addPartition("high_traffic_pairs", std::move(high_traffic_pairs));

  ColocateGroupsSpec high_traffic_coloc;
  high_traffic_coloc.name() = "colocate-high-traffic";
  high_traffic_coloc.scope() = "host";
  high_traffic_coloc.partitionName() = "high_traffic_pairs";
  high_traffic_coloc.bound() = ColocateGroupsSpecBound::MAX;
  Limit ht_limit;
  ht_limit.type() = LimitType::ABSOLUTE;
  ht_limit.globalLimit() = 1;
  high_traffic_coloc.limits() = std::move(ht_limit);
  solver.addGoal(
      std::move(high_traffic_coloc),
      10.0); // Very high weight - these MUST colocate
  // variation_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Partial colocation variation\n";
  std::cout << "Colocate only high-traffic service pairs (subset)\n";
  return 0;
}
