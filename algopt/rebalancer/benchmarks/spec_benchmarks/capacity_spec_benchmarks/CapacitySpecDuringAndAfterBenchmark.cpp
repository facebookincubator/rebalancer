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
#include "algopt/rebalancer/interface/tests/utils.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer::interface;

static void runDuringAndAfterCapacity(
    int objectCount,
    int containerCount,
    int dimensionCount,
    bool useParallelizedNewMaterializer = false,
    CapacitySpecDefinition specDefinition =
        CapacitySpecDefinition::DURING_AND_AFTER) {
  auto solver = initializeTestProblemSolver(
      {.useParallelMaterializer = useParallelizedNewMaterializer});
  solver->setObjectName("shard");
  solver->setContainerName("server");

  std::map<std::string, std::vector<std::string>> serverToShards;
  for (const auto serverId : folly::irange(containerCount)) {
    serverToShards[fmt::format("server-{}", serverId)] = {};
  }
  for (const auto shardId : folly::irange(objectCount)) {
    int serverId = shardId % containerCount;
    serverToShards[fmt::format("server-{}", serverId)].push_back(
        fmt::format("shard-{}", shardId));
  }
  solver->setAssignment(serverToShards);

  for (const auto dimensionId : folly::irange(dimensionCount)) {
    auto dimensionName = fmt::format("dimension-{}", dimensionId);
    solver->addObjectDimension(dimensionName, std::map<std::string, double>());

    CapacitySpec spec;
    spec.scope() = "server";
    spec.dimension() = dimensionName;
    spec.definition() = specDefinition;
    solver->addConstraint(spec);
  }

  {
    // Local search with a limit of zero moves, since we are interested in
    // measuring materialization time for this benchmark.
    LocalSearchSolverSpec spec;
    spec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
    spec.stopAfterMoves() = 0;
    solver->addSolver(spec);
  }

  solver->solve();
}

BENCHMARK(DuringAndAfterCapacity_10000_10000_10) {
  runDuringAndAfterCapacity(10000, 10000, 10);
}
BENCHMARK(DoubleDuringAndAfterCapacity_10000_10000_10) {
  runDuringAndAfterCapacity(
      10000, 10000, 10, false, CapacitySpecDefinition::DOUBLE_DURING_AND_AFTER);
}

BENCHMARK(DuringAndAfterCapacityMoreDimensions_10000_10000_100) {
  runDuringAndAfterCapacity(10000, 10000, 100);
}

BENCHMARK(DuringAndAfterCapacity_10000_10000_10_parallel) {
  runDuringAndAfterCapacity(10000, 10000, 10, true);
}
BENCHMARK(DoubleDuringAndAfterCapacity_10000_10000_10_parallel) {
  runDuringAndAfterCapacity(
      10000, 10000, 10, true, CapacitySpecDefinition::DOUBLE_DURING_AND_AFTER);
}

BENCHMARK(DuringAndAfterCapacityMoreDimensions_10000_10000_100_parallel) {
  runDuringAndAfterCapacity(10000, 10000, 100, true);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
