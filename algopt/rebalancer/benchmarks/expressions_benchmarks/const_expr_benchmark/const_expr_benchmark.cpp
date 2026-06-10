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
#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer::interface;
using SolverType = facebook::rebalancer::interface::benchmarks::SolverType;

static void run() {
  const std::shared_ptr<facebook::rebalancer::entities::Universe> universe =
      std::make_shared<facebook::rebalancer::entities::Universe>();

  for ([[maybe_unused]] const auto _ : folly::irange(1'000'000'000)) {
    facebook::rebalancer::const_expr(0, universe);
  }
}

BENCHMARK(ConstExprBenchmark) {
  run();
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
