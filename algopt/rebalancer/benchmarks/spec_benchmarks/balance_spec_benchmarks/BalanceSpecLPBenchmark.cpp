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

static void
runBalance(int objectCount, int containerCount, SolverType solverType) {
  auto solver = initializeTestProblemSolver();
  solver->setObjectName("shard");
  solver->setContainerName("host");

  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(containerCount)) {
    assignment[fmt::format("h{}", i)] = {};
  }
  for (const auto i : folly::irange(objectCount)) {
    assignment["h0"].push_back(fmt::format("s{}", i));
  }
  solver->setAssignment(assignment);

  std::map<std::string, double> shard_cpu;
  for (const auto i : folly::irange(objectCount)) {
    shard_cpu[fmt::format("s{}", i)] = 1;
  }
  solver->addObjectDimension("cpu", std::move(shard_cpu));

  std::map<std::string, double> host_cpu;
  for (const auto i : folly::irange(containerCount)) {
    host_cpu[fmt::format("h{}", i)] = objectCount;
  }
  solver->addContainerDimension("cpu", host_cpu);

  BalanceSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  spec.formula() = BalanceSpecFormula::LEGACY;
  spec.fixAverageToInitial() = true;

  solver->addGoal(std::move(spec));

  addSolver(*solver, solverType);
  auto solution = solver->solve();
}

BENCHMARK(Balance_10000_10000_lp) {
  runBalance(10000, 10000, SolverType::LP_BUILD_ONLY);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
