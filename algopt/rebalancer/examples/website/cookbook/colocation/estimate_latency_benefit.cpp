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
 * Estimate Latency Benefit from Colocation
 *
 * This example demonstrates how to estimate the latency improvement achieved
 * through service colocation.
 *
 * Use this when: You want to quantify the performance benefit of colocation
 * to justify the optimization effort.
 */

#include <folly/init/Init.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

// variation_start
struct LatencyMetrics {
  double initial_latency_ms;
  double final_latency_ms;
  double improvement_pct;
};

LatencyMetrics estimateLatencyBenefit(
    const std::map<std::string, std::vector<std::string>>& initial_assignment,
    const std::map<std::string, std::vector<std::string>>& final_assignment,
    const std::vector<std::pair<std::string, std::string>>& service_pairs,
    double intra_host_latency_ms = 0.1,
    double inter_host_latency_ms = 2.0) {
  // Estimate latency improvement from colocation
  [[maybe_unused]] auto calculate_avg_latency = [&](const auto& assignment) {
    double total_latency = 0.0;
    for (const auto& [instance_a, instance_b] : service_pairs) {
      // Find hosts
      std::string host_a, host_b;
      for (const auto& [host, instances] : assignment) {
        if (std::find(instances.begin(), instances.end(), instance_a) !=
            instances.end()) {
          host_a = host;
        }
        if (std::find(instances.begin(), instances.end(), instance_b) !=
            instances.end()) {
          host_b = host;
        }
      }

      if (host_a == host_b) {
        total_latency += intra_host_latency_ms;
      } else {
        total_latency += inter_host_latency_ms;
      }
    }
    return total_latency / service_pairs.size();
  };

  double initial_latency = calculate_avg_latency(initial_assignment);
  double final_latency = calculate_avg_latency(final_assignment);
  double improvement_pct =
      ((initial_latency - final_latency) / initial_latency) * 100.0;

  std::cout << "Latency analysis:\n";
  std::cout << "  Initial avg latency: " << initial_latency << "ms\n";
  std::cout << "  Final avg latency: " << final_latency << "ms\n";
  std::cout << "  Improvement: " << improvement_pct << "%\n";

  return {
      .initial_latency_ms = initial_latency,
      .final_latency_ms = final_latency,
      .improvement_pct = improvement_pct};
}
// variation_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Latency benefit estimation\n";
  std::cout << "Calculate performance improvement from colocation\n";
  return 0;
}
