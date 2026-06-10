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

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/ProblemSolverFactory.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer::interface;

static void runBalanceWithPruning(
    int objectCount,
    int containerCount,
    int extraObjectsPerContainer = 4,
    double overUtilizedContainersFraction = 0.2) {
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("shard");
  solver->setContainerName("host");

  std::set<std::string> hosts_at_average_util;
  std::map<std::string, std::vector<std::string>> assignment;
  std::map<std::string, double> host_cpu;
  for (const auto i : folly::irange(containerCount)) {
    auto host = fmt::format("h{}", i);
    host_cpu[host] =
        objectCount / containerCount + extraObjectsPerContainer + 1;
    assignment[host] = {};
    hosts_at_average_util.insert(host);
  }

  std::map<std::string, double> shard_cpu;
  for (const auto i : folly::irange(objectCount)) {
    auto shard = fmt::format("s{}", i);
    shard_cpu[shard] = 1;
    assignment[fmt::format("h{}", i % containerCount)].push_back(shard);
  }

  // over_utilized_hosts_fraction of hosts get extra_shards more shards each
  for (const auto i : folly::irange(
           static_cast<int64_t>(
               overUtilizedContainersFraction * containerCount))) {
    auto host = fmt::format("h{}", i);
    hosts_at_average_util.erase(host);
    for (const auto j : folly::irange(1, extraObjectsPerContainer + 1)) {
      auto shard =
          fmt::format("s{}", objectCount + extraObjectsPerContainer * i + j);
      shard_cpu[shard] = 1;
      assignment[host].push_back(shard);
    }
  }

  solver->setAssignment(assignment);
  solver->addObjectDimension("cpu", std::move(shard_cpu));
  solver->addContainerDimension("cpu", host_cpu);

  // since average is fixed to its initial value, though filter is set (and
  // thus utilization could potentially be non-preserving) we expect no
  // regression
  {
    BalanceSpec spec;
    spec.scope() = "host";
    spec.dimension() = "cpu";
    spec.fixAverageToInitial() = true;

    Filter filter;
    filter.itemsBlacklist() = {fmt::format("h{}", containerCount - 1)};
    spec.filter() = std::move(filter);

    solver->addGoal(std::move(spec));
  }

  solver->addGoalBoundary();

  {
    BalanceSpec spec;
    spec.scope() = "host";
    spec.dimension() = "cpu";
    spec.fixAverageToInitial() = true;

    // ensure balance between all over utilized hosts
    Filter filter;
    filter.itemsBlacklist() = std::vector<std::string>(
        hosts_at_average_util.begin(), hosts_at_average_util.end());
    spec.filter() = std::move(filter);

    solver->addGoal(std::move(spec));
  }

  {
    LocalSearchSolverSpec spec;
    spec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
    spec.enableConstrainedBoundsOptimization() = true;
    spec.constrainedBoundsCheckMs() = 25;

    solver->addSolver(spec);
  }

  auto solution = solver->solve();
}

BENCHMARK(BalancePruning_1000_10) {
  runBalanceWithPruning(1000, 10);
}
BENCHMARK(BalancePruning_1000_100) {
  runBalanceWithPruning(1000, 100);
}
BENCHMARK(BalancePruning_10000_10) {
  runBalanceWithPruning(10000, 10);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
