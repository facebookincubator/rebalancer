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
#include "algopt/rebalancer/interface/ProblemSolverFactory.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer::interface;
using SolverType = facebook::rebalancer::interface::benchmarks::SolverType;

static void
runAvoidMoves(int objectCount, int containerCount, SolverType solverType) {
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("shard");
  solver->setContainerName("host");

  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(containerCount)) {
    assignment[fmt::format("h{}", i)] = {};
  }

  // initially, each object i is in container j, where j = i % containerCount
  std::vector<std::string> allObjects;
  for (const auto i : folly::irange(objectCount)) {
    auto hostName = fmt::format("h{}", i % containerCount);
    auto objectName = fmt::format("s{}", i);
    assignment[hostName].push_back(objectName);

    allObjects.push_back(objectName);
  }
  solver->setAssignment(assignment);

  // add an avoid moving spec, preventing all objects from moving
  AvoidMovingSpec avoidMovingSpec;
  avoidMovingSpec.objects() = std::move(allObjects);

  solver->addConstraint(std::move(avoidMovingSpec));

  addSolver(*solver, solverType);

  solver->solve();
}

BENCHMARK(AvoidMoves_1000000_1000000) {
  runAvoidMoves(1000000, 1000000, SolverType::MATERIALIZE_ONLY);
}

BENCHMARK(AvoidMoves_1000000_100000) {
  runAvoidMoves(1000000, 100000, SolverType::MATERIALIZE_ONLY);
}

BENCHMARK(AvoidMoves_1000000_10000) {
  runAvoidMoves(1000000, 10000, SolverType::MATERIALIZE_ONLY);
}

BENCHMARK(AvoidMoves_1000000_100) {
  runAvoidMoves(1000000, 100, SolverType::MATERIALIZE_ONLY);
}

BENCHMARK(AvoidMoves_1000000_1) {
  runAvoidMoves(1000000, 1, SolverType::MATERIALIZE_ONLY);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
