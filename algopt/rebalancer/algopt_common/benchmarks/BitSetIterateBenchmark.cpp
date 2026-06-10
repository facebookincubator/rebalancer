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
// Compares DynamicBitSet against boost::dynamic_bitset on iteration over set
// bits and per-call set / isSet.

#include "algopt/rebalancer/algopt_common/DynamicBitSet.h"

#include <boost/dynamic_bitset.hpp>
#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

DEFINE_uint64(total_bits, 10'000'000, "Total number of bits to iterate over.");

namespace facebook::algopt::benchmarks {

namespace {

using BoostBitSet = boost::dynamic_bitset<std::uint64_t>;

std::vector<std::size_t> sampledIndices(std::size_t totalBits, double pctSet) {
  const auto count =
      static_cast<std::size_t>(static_cast<double>(totalBits) * pctSet / 100.0);
  std::vector<std::size_t> result(count);
  std::mt19937_64 rng(42);
  const auto range = folly::irange(totalBits);
  std::sample(range.begin(), range.end(), result.begin(), count, rng);
  return result;
}

template <typename BitSet>
BitSet buildBitSet(const std::vector<std::size_t>& setIndices) {
  BitSet bs(FLAGS_total_bits);
  for (const auto i : setIndices) {
    bs.set(i);
  }
  return bs;
}

inline bool query(const DynamicBitSet& bs, std::size_t i) {
  return bs.isSet(i);
}
inline bool query(
    const boost::dynamic_bitset<std::uint64_t>& bs,
    std::size_t i) {
  return bs.test(i);
}

template <typename Fn>
void forEachSet(const DynamicBitSet& bs, Fn&& fn) {
  bs.forEachSetBit(std::forward<Fn>(fn));
}
template <typename Fn>
void forEachSet(const boost::dynamic_bitset<std::uint64_t>& bs, Fn&& fn) {
  for (auto i = bs.find_first();
       i != boost::dynamic_bitset<std::uint64_t>::npos;
       i = bs.find_next(i)) {
    fn(i);
  }
}

template <typename BitSet>
void runSet(std::size_t n, double pctSet) {
  folly::BenchmarkSuspender suspend;
  const auto indices = sampledIndices(FLAGS_total_bits, pctSet);
  BitSet bs(FLAGS_total_bits);
  suspend.dismiss();
  for ([[maybe_unused]] const auto k : folly::irange(n)) {
    bs.set(indices[k % indices.size()]);
  }
  folly::doNotOptimizeAway(bs);
}

template <typename BitSet>
void runIsSet(std::size_t n, double pctSet) {
  folly::BenchmarkSuspender suspend;
  const auto indices = sampledIndices(FLAGS_total_bits, pctSet);
  const auto bs = buildBitSet<BitSet>(indices);
  suspend.dismiss();
  std::size_t hits = 0;
  for ([[maybe_unused]] const auto k : folly::irange(n)) {
    hits += query(bs, indices[k % indices.size()]) ? 1 : 0;
  }
  folly::doNotOptimizeAway(hits);
}

template <typename BitSet>
void runIterate(std::size_t n, double pctSet) {
  folly::BenchmarkSuspender suspend;
  const auto bs = buildBitSet<BitSet>(sampledIndices(FLAGS_total_bits, pctSet));
  suspend.dismiss();
  std::size_t sum = 0;
  for ([[maybe_unused]] const auto _ : folly::irange(n)) {
    forEachSet(bs, [&](std::size_t i) { sum += i; });
  }
  folly::doNotOptimizeAway(sum);
}

} // namespace

#define ITERATE_BENCHMARKS(NAME, PCT)             \
  BENCHMARK(Dynamic_##NAME##_Iterate, n) {        \
    runIterate<DynamicBitSet>(n, PCT);            \
  }                                               \
  BENCHMARK_RELATIVE(Boost_##NAME##_Iterate, n) { \
    runIterate<BoostBitSet>(n, PCT);              \
  }

ITERATE_BENCHMARKS(100pct, 100.0)
ITERATE_BENCHMARKS(50pct, 50.0)
ITERATE_BENCHMARKS(30pct, 30.0)
ITERATE_BENCHMARKS(10pct, 10.0)
ITERATE_BENCHMARKS(1pct, 1.0)
ITERATE_BENCHMARKS(01pct, 0.1)

BENCHMARK(Dynamic_Set, n) {
  runSet<DynamicBitSet>(n, 0.1);
}
BENCHMARK_RELATIVE(Boost_Set, n) {
  runSet<BoostBitSet>(n, 0.1);
}

BENCHMARK(Dynamic_IsSet, n) {
  runIsSet<DynamicBitSet>(n, 0.1);
}
BENCHMARK_RELATIVE(Boost_IsSet, n) {
  runIsSet<BoostBitSet>(n, 0.1);
}

} // namespace facebook::algopt::benchmarks

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
