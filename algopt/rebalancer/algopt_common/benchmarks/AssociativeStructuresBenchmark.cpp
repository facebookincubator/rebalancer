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

#include "algopt/rebalancer/algopt_common/AssociativeFixedMap.h"
#include "algopt/rebalancer/algopt_common/AssociativeMap.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <random>

using namespace folly;
using namespace facebook::algopt;

std::vector<int> make_keys(int n) {
  std::vector<int> keys(n);
  for (const auto i : folly::irange(n)) {
    keys[i] = i;
  }
  std::mt19937 generator;
  shuffle(keys.begin(), keys.end(), generator);
  return keys;
}

BENCHMARK(AssociativeMapInsert) {
  constexpr int N = 1000000;
  BenchmarkSuspender suspender;
  auto keys = make_keys(N);
  SumMap<int, double> sum;
  suspender.dismiss();
  for (const int key : keys) {
    sum.update(key, 1.0);
  }
  suspender.rehire();
  EXPECT_EQ(N, sum.query());
}

BENCHMARK(AssociativeMapUpdate) {
  BenchmarkSuspender suspender;
  auto keys = make_keys(1000000);
  SumMap<int, double> sum;
  for (const int key : keys) {
    sum.update(key, 1.0);
  }
  EXPECT_EQ(1000000.0, sum.query());
  suspender.dismiss();
  for (const int key : keys) {
    sum.update(key, 2.0);
  }
  suspender.rehire();
  EXPECT_EQ(2000000.0, sum.query());
}

BENCHMARK(AssociativeFixedMapInsert) {
  constexpr int N = 1000000;
  BenchmarkSuspender suspender;
  auto keys = make_keys(N);
  std::vector<std::pair<int, double>> items;
  items.reserve(N);
  for (const int key : keys) {
    items.emplace_back(key, 1.0);
  }
  SumFixedMap<int, double> sum;
  suspender.dismiss();
  sum.init(std::move(items));
  suspender.rehire();
  EXPECT_EQ(N, sum.query());
}

BENCHMARK(AssociativeFixedMapUpdate) {
  constexpr int N = 1000000;
  BenchmarkSuspender suspender;
  auto keys = make_keys(N);
  std::vector<std::pair<int, double>> items;
  items.reserve(N);
  for (const int key : keys) {
    items.emplace_back(key, 1.0);
  }
  SumFixedMap<int, double> sum;
  sum.init(std::move(items));
  EXPECT_EQ(N, sum.query());
  suspender.dismiss();
  for (const int key : keys) {
    sum.update(key, 2.0);
  }
  suspender.rehire();
  EXPECT_EQ(2000000.0, sum.query());
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  runBenchmarks();

  return 0;
}
