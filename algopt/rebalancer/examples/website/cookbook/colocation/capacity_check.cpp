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
 * Capacity Check for Colocation Feasibility
 *
 * This example demonstrates how to verify that host capacity is sufficient for
 * colocating services before attempting optimization.
 *
 * Use this when: You want to ensure colocation is physically possible given
 * resource constraints.
 */

#include <folly/init/Init.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

void checkColocationCapacity(
    const std::map<std::string, double>& cpu_usage,
    const std::map<std::string, double>& host_cpu_capacity,
    const std::vector<std::pair<std::string, std::string>>& service_pairs) {
  // variation_start
  // Check if colocation is feasible
  double max_colocated_cpu = 0.0;
  for (const auto& pair : service_pairs) {
    double pair_cpu = cpu_usage.at(pair.first) + cpu_usage.at(pair.second);
    max_colocated_cpu = std::max(max_colocated_cpu, pair_cpu);
  }

  [[maybe_unused]] double min_capacity =
      std::min_element(
          host_cpu_capacity.begin(),
          host_cpu_capacity.end(),
          [](const auto& a, const auto& b) { return a.second < b.second; })
          ->second;

  assert(
      max_colocated_cpu <= min_capacity &&
      "Host capacity insufficient for colocation!");
  // variation_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Capacity check variation\n";
  std::cout << "Verify host capacity is sufficient for colocation\n";
  return 0;
}
