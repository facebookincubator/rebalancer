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
 * Weighted Colocation Priority
 *
 * This variation demonstrates how to assign different importance levels to
 * different colocation pairs based on their business impact.
 *
 * Use this when: Some service pairs have higher performance requirements than
 * others (e.g., user-facing vs internal services).
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/init/Init.h>

#include <iostream>

using namespace facebook::rebalancer::interface;

void addWeightedColocationPriority(ProblemSolver& solver) {
  // variation_start
  // High-priority pairs (frontend-API) - stronger colocation
  ColocateGroupsSpec frontend_api_coloc;
  frontend_api_coloc.name() = "colocate-frontend-api";
  frontend_api_coloc.scope() = "host";
  frontend_api_coloc.partitionName() = "frontend_api_pairs";
  frontend_api_coloc.bound() = ColocateGroupsSpecBound::MAX;
  Limit fa_limit;
  fa_limit.type() = LimitType::ABSOLUTE;
  fa_limit.globalLimit() = 1;
  frontend_api_coloc.limits() = std::move(fa_limit);
  solver.addGoal(
      std::move(frontend_api_coloc), 8.0); // Critical for user-facing latency

  // Medium-priority pairs (API-cache) - weaker colocation
  ColocateGroupsSpec api_cache_coloc;
  api_cache_coloc.name() = "colocate-api-cache";
  api_cache_coloc.scope() = "host";
  api_cache_coloc.partitionName() = "api_cache_pairs";
  api_cache_coloc.bound() = ColocateGroupsSpecBound::MAX;
  Limit ac_limit;
  ac_limit.type() = LimitType::ABSOLUTE;
  ac_limit.globalLimit() = 1;
  api_cache_coloc.limits() = std::move(ac_limit);
  solver.addGoal(
      std::move(api_cache_coloc), 3.0); // Nice to have but not critical
  // variation_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Weighted colocation priority variation\n";
  std::cout << "Assign different weights based on business importance\n";
  return 0;
}
