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

namespace interface = facebook::rebalancer::interface;

static void run(
    int benchmarkIters,
    int primaryObjectCount,
    int containerCount,
    int dimensionLengthForObjectDimension,
    interface::benchmarks::SolverType solverType,
    bool useParallelMaterializer) {
  for (const auto _ : folly::irange(benchmarkIters)) {
    auto solver = interface::initializeTestProblemSolver(
        {.useParallelMaterializer = useParallelMaterializer});
    solver->setObjectName("task");
    solver->setContainerName("host");

    std::map<std::string, std::vector<std::string>> assignment;
    for (const auto i : folly::irange(containerCount)) {
      assignment[fmt::format("host{}", i)] = {};
    }

    // Each primaryObject has a secondaryObject associated with it. Each
    // primaryObject i is in container j, where j = i % containerCount, and the
    // corresponding secondaryObject is in (i+1)%containerCount
    for (const auto i : folly::irange(primaryObjectCount)) {
      auto primaryObjectName = fmt::format("task{}_p", i);
      auto secondaryObjectName = fmt::format("task{}_s", i);

      auto primaryObjectHostName = fmt::format("host{}", i % containerCount);
      auto secondaryObjectHostName =
          fmt::format("host{}", (i + 1) % containerCount);

      assignment[primaryObjectHostName].push_back(primaryObjectName);
      assignment[secondaryObjectHostName].push_back(secondaryObjectName);
    }
    solver->setAssignment(assignment);

    std::map<std::string, std::vector<std::string>>
        primaryToSetOfSecondaryObjects;
    for (const auto i : folly::irange(primaryObjectCount)) {
      auto primaryObjectName = fmt::format("task{}_p", i);
      auto secondaryObjectName = fmt::format("task{}_s", i);
      primaryToSetOfSecondaryObjects.emplace(
          primaryObjectName, std::vector<std::string>{secondaryObjectName});
    }

    // create object and container dimensions
    std::map<std::string, double> hostToLoad;
    for (const auto i : folly::irange(containerCount)) {
      hostToLoad.emplace(fmt::format("host{}", i), 100 + i);
    }

    solver->addContainerDimension("load", hostToLoad);

    std::map<std::string, std::vector<double>> objectToLoadValues;
    for (const auto i : folly::irange(primaryObjectCount)) {
      objectToLoadValues.emplace(
          fmt::format("task{}_p", i),
          std::vector<double>(dimensionLengthForObjectDimension, 10 + i));
      objectToLoadValues.emplace(
          fmt::format("task{}_s", i),
          std::vector<double>(dimensionLengthForObjectDimension, 10 + i));
    }

    solver->addObjectDimension("load", objectToLoadValues);

    interface::DisasterRecoveryCapacitySpec disasterSpec;
    disasterSpec.scope() = "host";
    disasterSpec.dimension() = "load";
    disasterSpec.primaryToSetOfSecondaryObjects() =
        std::move(primaryToSetOfSecondaryObjects);

    solver->addConstraint(disasterSpec);

    interface::benchmarks::addSolver(
        *solver, solverType, useParallelMaterializer);

    solver->solve();
  }
}

/* The following two benchmarks are to test the time taken when there are a
large number of dimensions in the input objectDimension, with and without
parallelization*/
constexpr int multiDimObjectCount = 500;
constexpr int multiDimContainerCount = 20;
constexpr int objectDimLength = 50;

BENCHMARK_NAMED_PARAM(
    run,
    serial_materialization,
    multiDimObjectCount,
    multiDimContainerCount,
    objectDimLength,
    interface::benchmarks::SolverType::MATERIALIZE_ONLY,
    false /* useParallelMaterializer*/)

BENCHMARK_NAMED_PARAM(
    run,
    parallel_materialization,
    multiDimObjectCount,
    multiDimContainerCount,
    objectDimLength,
    interface::benchmarks::SolverType::MATERIALIZE_ONLY,
    true /* useParallelMaterializer*/)

BENCHMARK_NAMED_PARAM(
    run,
    lp_building,
    multiDimObjectCount,
    multiDimContainerCount,
    objectDimLength,
    interface::benchmarks::SolverType::LP_BUILD_ONLY,
    true /* useParallelMaterializer*/)

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
