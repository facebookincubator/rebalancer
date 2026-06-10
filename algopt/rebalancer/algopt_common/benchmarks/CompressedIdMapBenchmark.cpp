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
//
// Benchmarks CompressedIdMap construction, lookup, and iteration against the
// bare entities::Map<entities::ObjectId, double> baseline at multiple density
// points. The Populate benchmarks emit a `MemMB` counter sourced from
// jemalloc's `stats.allocated` so memory footprint is comparable across layouts
// without any CompressedIdMap introspection API.

#include "algopt/rebalancer/algopt_common/CompressedIdMap.h"
#include "algopt/rebalancer/algopt_common/Utils.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"

#include <folly/Benchmark.h>
#include <folly/container/MapUtil.h>
#include <folly/Conv.h>
#include <folly/init/Init.h>
#include <folly/memory/MallctlHelper.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

DEFINE_uint64(
    total_objects,
    5'000'000,
    "Total number of object slots used by the benchmarks.");

namespace facebook::algopt::benchmarks {

namespace entities = ::facebook::rebalancer::entities;

namespace {

constexpr double kBytesPerMB = 1024.0 * 1024.0;

using BareMap = entities::Map<entities::ObjectId, double>;
using CompressedIdObjectMap = CompressedIdMap<entities::ObjectId, double>;

std::size_t jemallocAllocatedBytes() {
  folly::mallctlWrite<std::uint64_t>("epoch", 1);
  std::size_t allocated = 0;
  folly::mallctlRead("stats.allocated", &allocated);
  return allocated;
}

std::size_t nonDefaultCount(double pctNonDefault) {
  return static_cast<std::size_t>(
      static_cast<double>(FLAGS_total_objects) * pctNonDefault / 100.0);
}

// Builds a map of the chosen type pre-populated with `pct` non-default entries.
template <typename MapT>
MapT build(double pctNonDefault) {
  const auto count = nonDefaultCount(pctNonDefault);
  MapT map = [count] {
    if constexpr (std::is_same_v<MapT, BareMap>) {
      BareMap m;
      m.reserve(folly::to<typename BareMap::size_type>(count));
      return m;
    } else {
      return CompressedIdObjectMap(
          /*totalSize=*/FLAGS_total_objects,
          /*defaultValue=*/0.0,
          /*expectedNonDefaultSize=*/count);
    }
  }();
  for (const auto i : folly::irange(count)) {
    map.emplace(entities::ObjectId(i), static_cast<double>(i + 1));
  }
  return map;
}

double lookup(const BareMap& m, entities::ObjectId k) {
  return folly::get_default(m, k, 0.0);
}
double lookup(const CompressedIdObjectMap& m, entities::ObjectId k) {
  return m.getValue(k);
}

std::vector<std::size_t> shuffledIndices(std::size_t n) {
  std::vector<std::size_t> indices(n);
  for (const auto i : folly::irange(n)) {
    indices[i] = i % FLAGS_total_objects;
  }
  std::shuffle(indices.begin(), indices.end(), std::mt19937_64(42));
  return indices;
}

template <typename MapT>
void runPopulate(folly::UserCounters& counters, double pct) {
  const auto before = jemallocAllocatedBytes();
  auto built = build<MapT>(pct);
  folly::doNotOptimizeAway(built);
  const auto after = jemallocAllocatedBytes();
  counters["MemMB"] =
      folly::UserMetric((after > before ? after - before : 0) / kBytesPerMB);
}

template <typename MapT>
void runLookup(std::size_t n, double pct) {
  folly::BenchmarkSuspender suspend;
  const auto map = build<MapT>(pct);
  const auto indices = shuffledIndices(n);
  suspend.dismiss();

  double sum = 0;
  for (const auto i : folly::irange(n)) {
    sum += lookup(map, entities::ObjectId(indices[i]));
  }
  folly::doNotOptimizeAway(sum);
}

template <typename MapT>
void runIterate(std::size_t n, double pct) {
  folly::BenchmarkSuspender suspend;
  const auto map = build<MapT>(pct);
  suspend.dismiss();
  double sum = 0;
  for ([[maybe_unused]] const auto _ : folly::irange(n)) {
    utils::forEachNonDefault(
        map, [&](entities::ObjectId, const double& value) { sum += value; });
  }
  folly::doNotOptimizeAway(sum);
}
} // namespace

#define POPULATE_BENCHMARKS(NAME, PCT)                                    \
  BENCHMARK_COUNTERS(Map_##NAME##_Populate, counters) {                   \
    runPopulate<BareMap>(counters, PCT);                                  \
  }                                                                       \
  BENCHMARK_COUNTERS_RELATIVE(CompressedId_##NAME##_Populate, counters) { \
    runPopulate<CompressedIdObjectMap>(counters, PCT);                    \
  }

#define LOOKUP_BENCHMARKS(NAME, PCT)                    \
  BENCHMARK(Map_##NAME##_Lookup, n) {                   \
    runLookup<BareMap>(n, PCT);                         \
  }                                                     \
  BENCHMARK_RELATIVE(CompressedId_##NAME##_Lookup, n) { \
    runLookup<CompressedIdObjectMap>(n, PCT);           \
  }

#define ITERATE_BENCHMARKS(NAME, PCT)                           \
  BENCHMARK(Map_##NAME##_Iterate, n) {                          \
    runIterate<BareMap>(n, PCT);                                \
  }                                                             \
  BENCHMARK_RELATIVE(CompressedIdForEach_##NAME##_Iterate, n) { \
    runIterate<CompressedIdObjectMap>(n, PCT);                  \
  }

#define FOR_EACH_DENSITY(MACRO) \
  MACRO(100pct, 100.0)          \
  MACRO(50pct, 50.0)            \
  MACRO(30pct, 30.0)            \
  MACRO(20pct, 20.0)            \
  MACRO(10pct, 10.0)            \
  MACRO(1pct, 1.0)              \
  MACRO(01pct, 0.1)

FOR_EACH_DENSITY(POPULATE_BENCHMARKS)
FOR_EACH_DENSITY(LOOKUP_BENCHMARKS)
FOR_EACH_DENSITY(ITERATE_BENCHMARKS)

} // namespace facebook::algopt::benchmarks

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
