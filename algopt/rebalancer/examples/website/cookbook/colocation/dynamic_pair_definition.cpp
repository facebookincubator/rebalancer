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
 * Dynamic Pair Definition Based on Traffic
 *
 * This variation demonstrates how to dynamically create service pairs based on
 * observed runtime communication patterns rather than static definitions.
 *
 * Use this when: Service communication patterns change over time and you want
 * to optimize based on actual traffic data.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

using namespace facebook::rebalancer::interface;

// variation_start
std::map<std::string, std::vector<std::string>> createPairsFromTraffic(
    const std::map<std::pair<std::string, std::string>, double>& traffic_matrix,
    double traffic_threshold) {
  // Create service pairs based on observed traffic patterns
  std::map<std::string, std::vector<std::string>> pairs;
  int pair_id = 0;

  // Find instance pairs with high traffic between them
  for (const auto& [instance_pair, traffic] : traffic_matrix) {
    if (traffic > traffic_threshold) { // High traffic
      std::string pair_name = "dynamic_pair_" + std::to_string(pair_id);
      pairs[pair_name] = {instance_pair.first, instance_pair.second};
      pair_id++;
    }
  }

  return pairs;
}

void setupDynamicColocation(ProblemSolver& solver) {
  // Example traffic data
  const double TRAFFIC_THRESHOLD = 1000.0;
  const std::map<std::pair<std::string, std::string>, double> traffic_data = {
      {{"serviceA_1", "serviceB_1"}, 5000.0},
      {{"serviceA_2", "serviceB_2"}, 2000.0},
  };

  // Use observed traffic patterns
  auto dynamic_pairs = createPairsFromTraffic(traffic_data, TRAFFIC_THRESHOLD);
  solver.addPartition("dynamic_pairs", std::move(dynamic_pairs));

  ColocateGroupsSpec dynamic_coloc;
  dynamic_coloc.name() = "colocate-high-traffic";
  dynamic_coloc.scope() = "host";
  dynamic_coloc.partitionName() = "dynamic_pairs";
  dynamic_coloc.bound() = ColocateGroupsSpecBound::MAX;
  Limit dynamic_limit;
  dynamic_limit.type() = LimitType::ABSOLUTE;
  dynamic_limit.globalLimit() = 1;
  dynamic_coloc.limits() = std::move(dynamic_limit);
  solver.addGoal(std::move(dynamic_coloc), 6.0);
}
// variation_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Dynamic pair definition variation\n";
  std::cout << "Create service pairs based on observed traffic patterns\n";
  return 0;
}
