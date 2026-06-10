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

#include "algopt/rebalancer/benchmarks/interface_benchmarks/BenchmarkUtils.h"
#include "algopt/rebalancer/interface/ProblemSolver.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

#include <string_view>

namespace facebook::rebalancer::interface::benchmarks {
namespace {
constexpr std::string_view DIMENSION_NAME = "test_dimension";
constexpr std::string_view SCOPE_NAME = "test_scope";
constexpr std::string_view PARTITION_NAME = "test_partition";

struct ProblemData {
  std::unique_ptr<ProblemSolver> solver;
  folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
      dimension;
};

struct ProblemDataWithPartition {
  std::unique_ptr<ProblemSolver> solver;
  folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
      scopeItemToGroupToValue;
  folly::F14FastMap<std::string, std::vector<std::string>> groupToObjects;
};

void addScopeToSolver(ProblemSolver& solver, int nScopeItems) {
  folly::F14FastMap<std::string, std::string> containerToScopeItem;
  for (const auto scopeIdx : folly::irange(nScopeItems)) {
    containerToScopeItem[fmt::format("host_{}", scopeIdx)] =
        fmt::format("scope_{}", scopeIdx);
  }
  solver.addScope(std::string(SCOPE_NAME), containerToScopeItem);
}

} // namespace

std::unique_ptr<ProblemData> setupProblemAndGetDynamicDimension(
    int nScopeItems,
    int nObjects,
    bool canExecuteAsync = false) {
  auto solver = createBaseProblemSolver(nScopeItems, nObjects, canExecuteAsync);
  addScopeToSolver(*solver, nScopeItems);

  auto dimension = createDynamicDimensionData(nScopeItems, nObjects);

  return std::make_unique<ProblemData>(ProblemData{
      .solver = std::move(solver), .dimension = std::move(dimension)});
}

std::unique_ptr<ProblemDataWithPartition>
setupProblemAndGetDynamicDimensionByPartition(
    int nScopeItems,
    int nGroups,
    int nObjects,
    bool canExecuteAsync = false) {
  auto solver = createBaseProblemSolver(nScopeItems, nObjects, canExecuteAsync);
  addScopeToSolver(*solver, nScopeItems);

  auto groupToObjects = createPartitionData(nGroups, nObjects);
  solver->addPartition(std::string(PARTITION_NAME), groupToObjects);

  auto scopeItemToGroupToValue = createDynamicDimensionData(
      nScopeItems, nGroups, /*dimensionBasedOnGroups=*/true);

  return std::make_unique<ProblemDataWithPartition>(ProblemDataWithPartition{
      .solver = std::move(solver),
      .scopeItemToGroupToValue = std::move(scopeItemToGroupToValue),
      .groupToObjects = std::move(groupToObjects)});
}

BENCHMARK(AddDynamicDimension_100x90k) {
  folly::BenchmarkSuspender suspend;
  auto data = setupProblemAndGetDynamicDimension(/*nScopeItems=*/100,
                                                 /*nObjects=*/90000);
  suspend.dismiss();

  data->solver->addDynamicObjectDimension(
      std::string(DIMENSION_NAME),
      std::string(SCOPE_NAME),
      data->dimension,
      0.0);

  BENCHMARK_SUSPEND {
    data.reset();
  }
}

BENCHMARK(AddDynamicDimension_250x2m) {
  folly::BenchmarkSuspender suspend;
  auto data = setupProblemAndGetDynamicDimension(/*nScopeItems=*/250,
                                                 /*nObjects=*/2000000);
  suspend.dismiss();

  data->solver->addDynamicObjectDimension(
      std::string(DIMENSION_NAME),
      std::string(SCOPE_NAME),
      data->dimension,
      0.0);

  BENCHMARK_SUSPEND {
    data.reset();
  }
}

BENCHMARK(AddDynamicDimensionByPartition) {
  folly::BenchmarkSuspender suspend;
  constexpr int nScopeItems = 500;
  constexpr int nGroups = 2000;
  constexpr int nObjects = nGroups * 500;
  auto data = setupProblemAndGetDynamicDimensionByPartition(
      nScopeItems, nGroups, nObjects);
  suspend.dismiss();

  data->solver->addDynamicObjectDimension(
      std::string(DIMENSION_NAME),
      std::string(SCOPE_NAME),
      std::string(PARTITION_NAME),
      data->scopeItemToGroupToValue,
      0.0);

  BENCHMARK_SUSPEND {
    data.reset();
  }
}

BENCHMARK(AddDynamicDimensionByPartition_250x2m_with_1_partition) {
  folly::BenchmarkSuspender suspend;
  constexpr int nScopeItems = 250;
  constexpr int nGroups = 1;
  constexpr int nObjects = 2000000;
  auto data = setupProblemAndGetDynamicDimensionByPartition(
      nScopeItems, nGroups, nObjects);
  suspend.dismiss();

  data->solver->addDynamicObjectDimension(
      std::string(DIMENSION_NAME),
      std::string(SCOPE_NAME),
      std::string(PARTITION_NAME),
      data->scopeItemToGroupToValue,
      0.0);

  BENCHMARK_SUSPEND {
    data.reset();
  }
}

BENCHMARK(AddDynamicDimensionByPartitionSparse) {
  folly::BenchmarkSuspender suspend;
  constexpr int nScopeItems = 500;
  constexpr int nGroups = 2000;
  constexpr int nObjects = nGroups * 500;
  constexpr int groupsToInclude = 10;

  auto data = setupProblemAndGetDynamicDimensionByPartition(
      nScopeItems, nGroups, nObjects);

  folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
      sparseData;
  sparseData.reserve(nScopeItems);
  for (const auto& [scopeItem, groupToValue] : data->scopeItemToGroupToValue) {
    folly::F14FastMap<std::string, double> sparseGroupToValue;
    sparseGroupToValue.reserve(groupsToInclude);
    int count = 0;
    for (const auto& [group, value] : groupToValue) {
      if (count++ >= groupsToInclude) {
        break;
      }
      sparseGroupToValue[group] = value;
    }
    sparseData.emplace(scopeItem, std::move(sparseGroupToValue));
  }
  suspend.dismiss();

  data->solver->addDynamicObjectDimension(
      std::string(DIMENSION_NAME),
      std::string(SCOPE_NAME),
      std::string(PARTITION_NAME),
      sparseData,
      0.0);

  BENCHMARK_SUSPEND {
    data.reset();
  }
}

void runSeveralAddDynamicDimension_400x360kAsync(
    bool canExecuteAsync,
    bool includeTimeToDestruct) {
  folly::BenchmarkSuspender suspend;
  constexpr int kNumDimensions = 3;
  constexpr int nScopeItems = 400;
  constexpr int nObjects = 360000;
  auto solver = createBaseProblemSolver(nScopeItems, nObjects, canExecuteAsync);
  addScopeToSolver(*solver, nScopeItems);
  suspend.dismiss();

  for (const auto dimensionIdx : folly::irange(kNumDimensions)) {
    folly::BenchmarkSuspender innerSuspend;
    auto dimension = createDynamicDimensionData(nScopeItems, nObjects);
    innerSuspend.dismiss();

    solver->addDynamicObjectDimension(
        fmt::format("{}_{}", DIMENSION_NAME, dimensionIdx),
        std::string(SCOPE_NAME),
        std::move(dimension),
        0.0);
  }
  if (includeTimeToDestruct) {
    solver.reset();
  } else {
    BENCHMARK_SUSPEND {
      solver.reset();
    }
  }
}

BENCHMARK(Several400x360k_NotAsync) {
  runSeveralAddDynamicDimension_400x360kAsync(
      /*canExecuteAsync=*/false,
      /*includeTimeToDestruct=*/false);
}

BENCHMARK_RELATIVE(Several400x360k_Async_waitForDestructor) {
  runSeveralAddDynamicDimension_400x360kAsync(/*canExecuteAsync=*/true,
                                              /*includeTimeToDestruct=*/true);
}

BENCHMARK_RELATIVE(Several400x360k_Async) {
  runSeveralAddDynamicDimension_400x360kAsync(
      /*canExecuteAsync=*/true,
      /*includeTimeToDestruct=*/false);
}

} // namespace facebook::rebalancer::interface::benchmarks

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
