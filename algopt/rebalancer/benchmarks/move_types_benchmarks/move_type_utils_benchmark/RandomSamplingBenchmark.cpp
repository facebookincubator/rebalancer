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

#include "algopt/rebalancer/solver/moves/MoveTypeUtils.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer;
using namespace facebook::rebalancer::interface;

static void
runGetRandomSamplesBenchmark(int numObjects, int sampleSize, int numCalls) {
  std::random_device rd;
  std::mt19937 gen(rd());
  gen.seed();
  std::vector<entities::ObjectId> ids(numObjects, entities::ObjectId(0));
  std::generate_n(
      ids.begin(), numObjects, [&gen] { return entities::ObjectId(gen()); });
  for (const auto _ : folly::irange(numCalls)) {
    getRandomSample(ids, sampleSize, gen);
  }
}

BENCHMARK(get_random_sample_10000_10_10) {
  runGetRandomSamplesBenchmark(10000, 10, 1);
}

BENCHMARK(get_random_sample_10000_10_100) {
  runGetRandomSamplesBenchmark(10000, 10, 100);
}

BENCHMARK(get_random_sample_10000_10_1000) {
  runGetRandomSamplesBenchmark(10000, 10, 1000);
}

BENCHMARK(get_random_sample_10000_10_10000) {
  runGetRandomSamplesBenchmark(10000, 10, 10000);
}

BENCHMARK(get_random_sample_10000_100_10000) {
  runGetRandomSamplesBenchmark(10000, 100, 10000);
}

BENCHMARK(get_random_sample_10000_1000_10000) {
  runGetRandomSamplesBenchmark(10000, 1000, 10000);
}

BENCHMARK(get_random_sample_10000_10000_10000) {
  runGetRandomSamplesBenchmark(10000, 10000, 10000);
}

BENCHMARK(get_random_sample_1000000_10_10) {
  runGetRandomSamplesBenchmark(1000000, 10, 1);
}

BENCHMARK(get_random_sample_1000000_10_100) {
  runGetRandomSamplesBenchmark(1000000, 10, 100);
}

BENCHMARK(get_random_sample_1000000_10_1000) {
  runGetRandomSamplesBenchmark(1000000, 10, 1000);
}

BENCHMARK(get_random_sample_1000000_10_10000) {
  runGetRandomSamplesBenchmark(1000000, 10, 10000);
}

BENCHMARK(get_random_sample_1000000_100_10) {
  runGetRandomSamplesBenchmark(1000000, 100, 1);
}

BENCHMARK(get_random_sample_1000000_100_100) {
  runGetRandomSamplesBenchmark(1000000, 100, 100);
}

BENCHMARK(get_random_sample_1000000_100_1000) {
  runGetRandomSamplesBenchmark(1000000, 100, 1000);
}

BENCHMARK(get_random_sample_1000000_100_10000) {
  runGetRandomSamplesBenchmark(1000000, 100, 10000);
}

BENCHMARK(get_random_sample_1000000_1000_10000) {
  runGetRandomSamplesBenchmark(1000000, 1000, 10000);
}

BENCHMARK(get_random_sample_1000000_10000_10000) {
  runGetRandomSamplesBenchmark(1000000, 10000, 10000);
}

BENCHMARK(get_random_sample_1000000_100000_10000) {
  runGetRandomSamplesBenchmark(1000000, 100000, 10000);
}

BENCHMARK(get_random_sample_1000000_1000000_10000) {
  runGetRandomSamplesBenchmark(1000000, 1000000, 10000);
}

BENCHMARK(real_life_example) {
  runGetRandomSamplesBenchmark(200000, 10, 7000);
}

BENCHMARK(real_life_example2) {
  runGetRandomSamplesBenchmark(200000, 100, 7000);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
