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

#include "algopt/rebalancer/benchmarks/utils/BenchmarkUtils.h"
#include "algopt/rebalancer/interface/tests/utils.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer::interface;
using SolverType = facebook::rebalancer::interface::benchmarks::SolverType;

static void runGroupLimit(
    int objectCount,
    int containerCount,
    int groupCount,
    SolverType solverType) {
  auto solver = initializeTestProblemSolver();
  solver->setObjectName("shard");
  solver->setContainerName("host");

  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(containerCount)) {
    assignment[fmt::format("h{}", i)] = {};
  }
  std::map<std::string, std::string> groupAssignment;
  for (const auto i : folly::irange(objectCount)) {
    auto shard = fmt::format("s{}", i);
    assignment["h0"].push_back(shard);
    groupAssignment[shard] = fmt::format("g{}", i % groupCount);
  }
  solver->setAssignment(assignment);
  solver->addPartition("replicas", std::move(groupAssignment));

  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "host";
  groupCountSpec.partitionName() = "replicas";

  solver->addGoal(std::move(groupCountSpec));

  addSolver(*solver, solverType);
  auto solution = solver->solve();
}

BENCHMARK(GroupLimit_1000_100_50_lp_subset) {
  runGroupLimit(1000, 100, 50, SolverType::LP_SUBSET_BUILD_ONLY);
}
BENCHMARK(GroupLimit_100_100_50_lp_subset) {
  runGroupLimit(100, 100, 50, SolverType::LP_SUBSET_BUILD_ONLY);
}
BENCHMARK(GroupLimit_100_1000_50_lp_subset) {
  runGroupLimit(100, 1000, 50, SolverType::LP_SUBSET_BUILD_ONLY);
}
BENCHMARK(GroupLimit_10000_1000_500_lp_subset) {
  runGroupLimit(10000, 1000, 500, SolverType::LP_SUBSET_BUILD_ONLY);
}
BENCHMARK(GroupLimit_1000_1000_500_lp_subset) {
  runGroupLimit(1000, 1000, 500, SolverType::LP_SUBSET_BUILD_ONLY);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
