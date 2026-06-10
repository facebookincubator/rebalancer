// Copyright (c) Meta Platforms, Inc. and affiliates.
#include "algopt/rebalancer/entities/Identifiers.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

#include <vector>

namespace facebook::rebalancer::entities::benchmarks {

constexpr std::size_t kCount = 1e9;

// Helper to create IDs from std::size_t values
template <typename T>
T makeId(std::size_t value) {
  if constexpr (std::is_same_v<T, ObjectId>) {
    return ObjectId(value);
  } else {
    return value;
  }
}

// Generic comparison benchmark
template <typename T, typename Op>
void benchmarkComparison(Op op) {
  folly::BenchmarkSuspender suspend;
  std::vector<T> ids1, ids2;
  ids1.reserve(kCount);
  ids2.reserve(kCount);
  for (const auto i : folly::irange(kCount)) {
    ids1.push_back(makeId<T>(i));
    ids2.push_back(makeId<T>(i + (i % 2)));
  }
  suspend.dismiss();

  std::size_t result = 0;
  for (const auto i : folly::irange(kCount)) {
    if (op(ids1.at(i), ids2.at(i))) {
      result++;
    }
  }
  folly::doNotOptimizeAway(result);
}

// Comparison benchmarks
BENCHMARK(SizeT_Equality) {
  benchmarkComparison<std::size_t>(std::equal_to<>{});
}
BENCHMARK_RELATIVE(ObjectId_Equality) {
  benchmarkComparison<ObjectId>(std::equal_to<>{});
}

BENCHMARK_DRAW_LINE();

BENCHMARK(SizeT_LessThan) {
  benchmarkComparison<std::size_t>(std::less<>{});
}

BENCHMARK_RELATIVE(ObjectId_LessThan) {
  benchmarkComparison<ObjectId>(std::less<>{});
}

BENCHMARK_DRAW_LINE();

BENCHMARK(SizeT_GreaterThan) {
  benchmarkComparison<std::size_t>(std::greater<>{});
}

BENCHMARK_RELATIVE(ObjectId_GreaterThan) {
  benchmarkComparison<ObjectId>(std::greater<>{});
}

BENCHMARK_DRAW_LINE();

// Constructor benchmark
BENCHMARK(ObjectId_Constructor) {
  std::vector<ObjectId> ids;
  ids.reserve(kCount);
  for (const auto i : folly::irange(kCount)) {
    ids.emplace_back(i);
  }
}

BENCHMARK_DRAW_LINE();

// Hash function benchmark
BENCHMARK(ObjectId_Hash) {
  folly::BenchmarkSuspender suspend;
  std::vector<ObjectId> ids;
  ids.reserve(kCount);
  for (const auto i : folly::irange(kCount)) {
    ids.push_back(makeId<ObjectId>(i));
  }

  std::size_t result = 0;
  const std::hash<ObjectId> hasher;
  suspend.dismiss();
  for (const auto i : folly::irange(kCount)) {
    result += hasher(ids.at(i));
  }
  folly::doNotOptimizeAway(result);
}

BENCHMARK_DRAW_LINE();

// IdStore getter benchmark
BENCHMARK(IdStore_Getter) {
  folly::BenchmarkSuspender suspend;
  constexpr std::size_t nObjects = 10e6;

  IdStoreConfig config;
  config.objectNames.reserve(nObjects);
  for (const auto i : folly::irange(nObjects)) {
    const auto objectName = fmt::format("object_{}", i);
    const auto objectId = ObjectId(config.objectNames.size());
    config.objectNames.push_back(objectName);
    config.objectNameToId.emplace(objectName, objectId);
  }
  const IdStore idStore(std::move(config));
  suspend.dismiss();

  std::size_t result = 0;
  constexpr std::size_t nLookups = 10e6;
  for (const auto i : folly::irange(nLookups)) {
    const ObjectId id =
        idStore.getObjectId(fmt::format("object_{}", i % nObjects));
    result += static_cast<std::size_t>(id);
  }
  folly::doNotOptimizeAway(result);
}

BENCHMARK_DRAW_LINE();

} // namespace facebook::rebalancer::entities::benchmarks

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
