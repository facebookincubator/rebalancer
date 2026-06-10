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

static void runSumOfMax(
    int objectCount,
    int fixedObjectCount,
    int containerCount,
    int groupCount,
    SolverType solverType) {
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("server");
  solver->setContainerName("reservation");

  std::map<std::string, std::vector<std::string>> reservationToServers;
  for (const auto serverId : folly::irange(objectCount)) {
    int reservationId = serverId % containerCount;
    reservationToServers[fmt::format("r{}", reservationId)].push_back(
        fmt::format("s{}", serverId));
  }
  solver->setAssignment(reservationToServers);

  std::map<std::string, std::string> serverToMsb;
  for (const auto serverId : folly::irange(objectCount)) {
    int msbId = serverId % groupCount;
    serverToMsb[fmt::format("s{}", serverId)] = fmt::format("m{}", msbId);
  }
  solver->addPartition("msb", std::move(serverToMsb));

  SumOfMaxSpec sumOfMaxSpec;
  sumOfMaxSpec.scope() = "reservation";
  sumOfMaxSpec.partitionName() = "msb";
  sumOfMaxSpec.dimension() = "server_count";

  solver->addGoal(std::move(sumOfMaxSpec));

  std::vector<std::string> fixedObjects;
  fixedObjects.reserve(fixedObjectCount);
  for (const auto serverId : folly::irange(fixedObjectCount)) {
    fixedObjects.push_back(fmt::format("s{}", serverId));
  }

  AvoidMovingSpec avoidMovingSpec;
  avoidMovingSpec.objects() = std::move(fixedObjects);

  solver->addConstraint(std::move(avoidMovingSpec));

  addSolver(*solver, solverType);

  solver->solve();
}

BENCHMARK(SumOfMax_FewFixedObjects_lp) {
  runSumOfMax(100000, 100, 1000, 10, SolverType::LP_BUILD_ONLY);
}
BENCHMARK(SumOfMax_ManyFixedObjects_lp) {
  runSumOfMax(5000, 4000, 10, 500, SolverType::LP_BUILD_ONLY);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
