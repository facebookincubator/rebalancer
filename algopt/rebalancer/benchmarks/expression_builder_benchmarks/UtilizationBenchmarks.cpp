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

#include "algopt/rebalancer/entities/tests/UniverseBuilderTestUtils.h"
#include "algopt/rebalancer/materializer/utils/ExpressionBuilder.h"
#include "algopt/rebalancer/treeprof/ExecutorWrapper.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/coro/BlockingWait.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>
#include <folly/Random.h>

namespace facebook::rebalancer::benchmarks {

namespace {
struct BenchmarkData {
  std::shared_ptr<const entities::Universe> universe;
  std::shared_ptr<materializer::ExpressionBuilder> builder;
  entities::DimensionId dimensionId;
  entities::ScopeId scopeId;
  entities::ScopeItemId scopeItemId;
  entities::PartitionId partitionId;
};

struct BenchmarkUniverse : public entities::tests::UniverseBuilderTestUtils {
  BenchmarkUniverse() : UniverseBuilderTestUtils("shard", "host") {}

  folly::coro::Task<std::unique_ptr<BenchmarkData>> build(
      const int nShards,
      const int nHosts,
      const int nScopeItems,
      const int nGroups) {
    //  initial assignment: shard i -> host (i % nHosts)
    entities::Map<std::string, std::vector<std::string>> initialAssignment;
    for (const auto i : folly::irange(nHosts)) {
      initialAssignment[fmt::format("host{}", i)] = {};
    }
    for (const auto i : folly::irange(nShards)) {
      initialAssignment[fmt::format("host{}", i % nHosts)].push_back(
          fmt::format("shard{}", i));
    }
    setInitialAssignment(initialAssignment);

    // Set up rack scope: host i -> rack (i % nScopeItems)
    entities::Map<std::string, std::vector<std::string>> rackToHosts;
    for (const auto i : folly::irange(nHosts)) {
      rackToHosts[fmt::format("rack{}", i % nScopeItems)].push_back(
          fmt::format("host{}", i));
    }
    co_await addScope("rack", rackToHosts);

    // Set up partition: shard i -> group (i % nGroups)
    entities::Map<std::string, std::vector<std::string>> groupToShards;
    for (const auto i : folly::irange(nShards)) {
      groupToShards[fmt::format("group{}", i % nGroups)].push_back(
          fmt::format("shard{}", i));
    }
    co_await addPartition("partition", groupToShards);

    const auto hostScope = scopeId("host");

    // Set up dynamic object dimension: random shard values per host
    const auto shardsPerScopeItem = nShards / nHosts;
    entities::Map<std::string, entities::Map<std::string, double>>
        hostToShardToValue;
    for (const auto i : folly::irange(nHosts)) {
      entities::Map<std::string, double> loadVals;

      // create a vector of random shard ids for this host
      std::vector<int> relevantShardIds;
      relevantShardIds.reserve(shardsPerScopeItem);
      for (const auto _ : folly::irange(shardsPerScopeItem)) {
        relevantShardIds.push_back(folly::Random::rand32(nShards));
      }

      for (const auto j : relevantShardIds) {
        loadVals[fmt::format("shard{}", j)] = i + j;
      }
      hostToShardToValue[fmt::format("host{}", i)] = std::move(loadVals);
    }

    co_await addDynamicObjectDimension(
        "load", hostScope, hostToShardToValue, 0.0);

    const auto universe = buildUniverse();

    const auto executor = std::make_shared<algopt::treeprof::ExecutorWrapper>(
        std::make_shared<folly::CPUThreadPoolExecutor>(20));
    auto builder = std::make_shared<materializer::ExpressionBuilder>(
        universe, universe->getContainers().getInitialAssignment(), executor);

    const auto rackScope = scopeId("rack");
    co_return std::make_unique<BenchmarkData>(BenchmarkData{
        .universe = universe,
        .builder = std::move(builder),
        .dimensionId = dimensionId("load"),
        .scopeId = rackScope,
        .scopeItemId = scopeItemId(rackScope, "rack0"),
        .partitionId = partitionId("partition")});
  }
};
} // namespace

std::unique_ptr<BenchmarkData> setupDynamicDimension(
    const int nShards,
    const int nHosts,
    const int nScopeItems,
    const int nGroups) {
  return folly::coro::blockingWait(
      BenchmarkUniverse().build(nShards, nHosts, nScopeItems, nGroups));
}

void runBenchmark(int nShards, int nHosts, int nScopeItems, int nGroups) {
  folly::BenchmarkSuspender suspend;
  auto data = setupDynamicDimension(nShards, nHosts, nScopeItems, nGroups);
  suspend.dismiss();

  for (const auto i : folly::irange(nGroups)) {
    folly::doNotOptimizeAway(data->builder->getAbsoluteUtil(
        materializer::UtilMetric::AFTER,
        data->dimensionId,
        data->scopeId,
        data->scopeItemId,
        data->partitionId,
        data->universe->getGroupId(
            data->partitionId, fmt::format("group{}", i))));
  }

  BENCHMARK_SUSPEND {
    data.reset();
  };
}

BENCHMARK(DynamicDim1) {
  runBenchmark(
      /*nShards=*/10e6, /*nHosts=*/5e3, /*nScopeItems=*/1, /*nGroups=*/100);
}

} // namespace facebook::rebalancer::benchmarks

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
