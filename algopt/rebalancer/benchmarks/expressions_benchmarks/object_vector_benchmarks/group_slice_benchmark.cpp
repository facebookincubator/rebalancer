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

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/ObjectValueTypes.h"
#include "algopt/rebalancer/entities/Partition.h"
#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/solver/expressions/ObjectVector.h"
#include "algopt/rebalancer/treeprof/ThreadMemoryMonitor.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>
#include <folly/memory/Malloc.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace facebook::rebalancer::benchmarks {
namespace {

using entities::GroupId;
using entities::GroupIdToDoubleMap;
using entities::ObjectId;
using entities::ObjectIdToDoubleMap;
using entities::ObjectValues;
using entities::Partition;
using entities::PartitionId;
using entities::Universe;

constexpr entities::EntityIdType kNumObjects = 100'000;
constexpr entities::EntityIdType kNumGroups = 200;
constexpr entities::EntityIdType kNumScopeItems = 20;

struct Fixture {
  std::shared_ptr<const Partition> partition;
  std::shared_ptr<const Partition> fallbackPartition;
  std::vector<ObjectValues> sources;
  std::shared_ptr<const Universe> universe;
};

std::shared_ptr<const Partition> makePartition() {
  entities::Map<GroupId, std::vector<ObjectId>> groups;
  groups.reserve(kNumGroups);
  for (const auto groupIndex : folly::irange(kNumGroups)) {
    groups[GroupId(groupIndex)] = {};
  }
  for (const auto objectIndex : folly::irange(kNumObjects)) {
    groups[GroupId(objectIndex % kNumGroups)].emplace_back(objectIndex);
  }
  return std::make_shared<const Partition>(std::move(groups));
}

ObjectValues makeGroupBackedSource(
    const std::shared_ptr<const Partition>& partition,
    double baseValue) {
  auto groupValues = std::make_shared<GroupIdToDoubleMap>();
  groupValues->reserve(kNumGroups);
  for (const auto groupIndex : folly::irange(kNumGroups)) {
    groupValues->emplace(
        GroupId(groupIndex), baseValue + static_cast<double>(groupIndex) + 1.0);
  }
  return ObjectValues(
      std::move(groupValues),
      partition,
      /*defaultValue=*/0.0,
      kNumObjects,
      PartitionId(0));
}

Fixture makeFixture() {
  Fixture fixture;
  fixture.partition = makePartition();
  fixture.fallbackPartition = makePartition();
  fixture.sources.reserve(kNumScopeItems);
  for (const auto scopeItemIndex : folly::irange(kNumScopeItems)) {
    fixture.sources.push_back(makeGroupBackedSource(
        fixture.partition, /*baseValue=*/static_cast<double>(scopeItemIndex)));
  }
  fixture.universe = std::make_shared<const Universe>();
  return fixture;
}

const Fixture& fixture() {
  static const Fixture data = makeFixture();
  return data;
}

using ObjectVectors = std::vector<std::shared_ptr<ObjectVector>>;

ObjectVectors materializeCompactSlices(const Fixture& data) {
  ObjectVectors slices;
  slices.reserve(kNumScopeItems * kNumGroups);
  for (const auto& source : data.sources) {
    for (const auto groupIndex : folly::irange(kNumGroups)) {
      slices.push_back(
          std::make_shared<ObjectVector>(
              source.sliceGroup(*data.partition, GroupId(groupIndex)),
              data.universe));
    }
  }
  return slices;
}

ObjectVectors materializeCrossPartitionFallbackSlices(const Fixture& data) {
  ObjectVectors slices;
  slices.reserve(kNumScopeItems * kNumGroups);
  for (const auto& source : data.sources) {
    for (const auto groupIndex : folly::irange(kNumGroups)) {
      slices.push_back(
          std::make_shared<ObjectVector>(
              source.sliceGroup(*data.fallbackPartition, GroupId(groupIndex)),
              data.universe));
    }
  }
  return slices;
}

ObjectVectors materializeExpandedSlices(const Fixture& data) {
  ObjectVectors slices;
  slices.reserve(kNumScopeItems * kNumGroups);
  for (const auto& source : data.sources) {
    for (const auto groupIndex : folly::irange(kNumGroups)) {
      const auto& objectIds = data.partition->getObjectIds(GroupId(groupIndex));
      auto objectValues = std::make_shared<ObjectIdToDoubleMap>(
          kNumObjects,
          /*defaultValue=*/0.0,
          /*expectedNonDefaultSize=*/objectIds.size());
      for (const auto objectId : objectIds) {
        objectValues->emplace(objectId, source.getObjectValue(objectId));
      }
      slices.push_back(
          std::make_shared<ObjectVector>(
              ObjectValues(std::move(objectValues)), data.universe));
    }
  }
  return slices;
}

struct MeasuredSlices {
  ObjectVectors slices;
  int64_t liveBytes = 0;
  uint64_t peakBytes = 0;
};

template <typename BuildFn>
MeasuredSlices materializeAndMeasure(BuildFn&& build) {
  algopt::treeprof::ThreadMemoryMonitor::reset();
  auto slices = build();
  return MeasuredSlices{
      .slices = std::move(slices),
      .liveBytes = algopt::treeprof::ThreadMemoryMonitor::delta(),
      .peakBytes = algopt::treeprof::ThreadMemoryMonitor::peak()};
}

double toMiB(double bytes) {
  return bytes / (1024.0 * 1024.0);
}

enum class StoragePath {
  Expanded,
  Compact,
};

int64_t expectedValueEntries(StoragePath path) {
  switch (path) {
    case StoragePath::Expanded:
      return static_cast<int64_t>(kNumScopeItems * kNumObjects);
    case StoragePath::Compact:
      return static_cast<int64_t>(kNumScopeItems * kNumGroups);
  }
  throw std::runtime_error("Unknown storage path");
}

void setCounters(
    folly::UserCounters& counters,
    const MeasuredSlices& measured,
    StoragePath path) {
  counters["objects"] = static_cast<int64_t>(kNumObjects);
  counters["groups"] = static_cast<int64_t>(kNumGroups);
  counters["scopeItems"] = static_cast<int64_t>(kNumScopeItems);
  counters["slices"] = static_cast<int64_t>(kNumScopeItems * kNumGroups);
  counters["valueEntries"] = expectedValueEntries(path);
  counters["liveMiB"] = toMiB(static_cast<double>(measured.liveBytes));
  counters["peakMiB"] = toMiB(static_cast<double>(measured.peakBytes));
  counters["jemalloc"] = folly::usingJEMalloc() ? int64_t{1} : int64_t{0};
}

void releaseOutsideTiming(MeasuredSlices& measured) {
  measured.slices.clear();
  measured.slices.shrink_to_fit();
}

} // namespace

BENCHMARK_COUNTERS(MaterializeGroupSlices_ExpandedFallback, counters) {
  folly::BenchmarkSuspender suspend;
  const auto& data = fixture();
  suspend.dismiss();

  auto measured =
      materializeAndMeasure([&] { return materializeExpandedSlices(data); });
  folly::doNotOptimizeAway(measured.slices);

  BENCHMARK_SUSPEND {
    setCounters(counters, measured, StoragePath::Expanded);
    releaseOutsideTiming(measured);
  };
}

BENCHMARK_COUNTERS_RELATIVE(
    MaterializeGroupSlices_CrossPartitionFallback,
    counters) {
  folly::BenchmarkSuspender suspend;
  const auto& data = fixture();
  suspend.dismiss();

  auto measured = materializeAndMeasure(
      [&] { return materializeCrossPartitionFallbackSlices(data); });
  folly::doNotOptimizeAway(measured.slices);

  BENCHMARK_SUSPEND {
    setCounters(counters, measured, StoragePath::Expanded);
    releaseOutsideTiming(measured);
  };
}

BENCHMARK_COUNTERS_RELATIVE(MaterializeGroupSlices_Compact, counters) {
  folly::BenchmarkSuspender suspend;
  const auto& data = fixture();
  suspend.dismiss();

  auto measured =
      materializeAndMeasure([&] { return materializeCompactSlices(data); });
  folly::doNotOptimizeAway(measured.slices);

  BENCHMARK_SUSPEND {
    setCounters(counters, measured, StoragePath::Compact);
    releaseOutsideTiming(measured);
  };
}

} // namespace facebook::rebalancer::benchmarks

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
