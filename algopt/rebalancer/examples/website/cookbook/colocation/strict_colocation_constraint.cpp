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
 * Strict Colocation Using Constraint
 *
 * This variation demonstrates how to make colocation mandatory using a
 * constraint instead of a soft goal. This ensures ALL service pairs are
 * colocated on exactly one host.
 *
 * Use this when: Colocation is absolutely required for correctness, not just
 * performance optimization.
 */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/init/Init.h>

#include <iostream>

using namespace facebook::rebalancer::interface;

void addStrictColocationConstraint(ProblemSolver& solver) {
  // variation_start
  // Use GroupCountSpec to enforce colocation
  // Each service pair must be on exactly 1 host
  GroupCountSpec enforce_coloc;
  enforce_coloc.name() = "enforce-colocation";
  enforce_coloc.scope() = "host";
  enforce_coloc.partitionName() = "service_pair";
  enforce_coloc.bound() = GroupCountSpecBound::EXACT;
  Limit exact_limit;
  exact_limit.type() = LimitType::ABSOLUTE;
  exact_limit.globalLimit() = 1;
  enforce_coloc.limit() = std::move(exact_limit);
  solver.addConstraint(std::move(enforce_coloc));
  // variation_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  // This is a snippet - would be integrated into a full solution
  // See basic_colocation.cpp for the complete example
  std::cout << "Strict colocation constraint variation\n";
  std::cout << "Use GroupCountSpec with EXACT bound to enforce colocation\n";
  return 0;
}
