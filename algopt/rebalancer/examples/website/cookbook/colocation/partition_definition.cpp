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
 * Correct Partition Definition for Colocation
 *
 * This example demonstrates the correct way to define partitions for service
 * pairs versus common mistakes.
 *
 * Use this when: You need to understand how to properly structure partitions
 * for colocation goals.
 */

#include <folly/container/irange.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

void definePartitionsCorrectly() {
  // Example data
  std::vector<std::string> all_instances = {
      "svc_a_1", "svc_b_1", "svc_a_2", "svc_b_2", "svc_a_3", "svc_b_3"};
  std::vector<std::pair<std::string, std::string>> pairs = {
      {"svc_a_1", "svc_b_1"},
      {"svc_a_2", "svc_b_2"},
      {"svc_a_3", "svc_b_3"},
  };

  // variation_start
  // BAD: All instances in one group
  std::map<std::string, std::vector<std::string>> bad_partition = {
      {"all_services", all_instances} // Won't colocate pairs!
  };

  // GOOD: Each pair is a separate group
  std::map<std::string, std::vector<std::string>> good_partition;
  for (const auto i : folly::irange(pairs.size())) {
    good_partition["pair_" + std::to_string(i)] = {
        pairs[i].first, pairs[i].second};
  }
  // variation_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Partition definition variation\n";
  std::cout << "Each service pair must be a separate partition group\n";
  return 0;
}
