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
 * Verify Colocation Results
 *
 * This example demonstrates how to verify that the solution achieved the target
 * colocation percentage.
 *
 * Use this when: You want to validate that the optimization met your colocation
 * requirements before deploying changes.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"

#include <folly/init/Init.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

// variation_start
bool verifyColocation(
    const AssignmentSolution& solution,
    const std::map<std::string, std::vector<std::string>>&
        service_pair_partition,
    double target_pct = 80.0) {
  // Verify colocation percentage meets target
  int colocated_pairs = 0;

  // Build container -> objects map from object -> container map
  std::map<std::string, std::vector<std::string>> container_to_objects;
  for (const auto& [object, container] : *solution.assignment()) {
    container_to_objects[container].push_back(object);
  }

  for (const auto& [pair_name, instances] : service_pair_partition) {
    // Find hosts with these instances
    std::set<std::string> hosts_with_instances;
    for (const auto& [host, host_instances] : container_to_objects) {
      for (const auto& instance : instances) {
        if (std::find(host_instances.begin(), host_instances.end(), instance) !=
            host_instances.end()) {
          hosts_with_instances.insert(host);
          break;
        }
      }
    }

    // Colocated if all instances on 1 host
    if (hosts_with_instances.size() == 1) {
      colocated_pairs++;
    }
  }

  int total_pairs = service_pair_partition.size();
  double colocation_pct =
      (static_cast<double>(colocated_pairs) / total_pairs) * 100.0;

  if (colocation_pct >= target_pct) {
    std::cout << "Colocation target met: " << colocation_pct
              << "% (target: " << target_pct << "%)\n";
    return true;
  } else {
    std::cout << "Colocation below target: " << colocation_pct
              << "% (target: " << target_pct << "%)\n";
    return false;
  }
}
// variation_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Verification example\n";
  std::cout << "Check if colocation percentage meets target requirements\n";
  return 0;
}
