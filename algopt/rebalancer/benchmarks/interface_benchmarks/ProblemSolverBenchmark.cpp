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

#include "algopt/rebalancer/interface/tests/utils.h"

#include <fmt/core.h>
#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

namespace facebook::rebalancer::interface::benchmarks {

struct Config {
  size_t numShards;
  bool canExecuteAsync;
  size_t numDimensions;
};

void addObjectDimensions(const Config& config) {
  folly::BenchmarkSuspender suspender;

  folly::F14FastMap<std::string, std::vector<std::string>> initialAssignment;
  folly::F14FastMap<std::string, double> shardDimension;
  for (const auto shardId : folly::irange(config.numShards)) {
    const std::string shardName = fmt::format("a:0,{0}-0,{0}", shardId);
    initialAssignment["h1"].push_back(shardName);
    shardDimension[shardName] = 1.0;
  }

  auto solverParams = TestProblemSolverParams();
  solverParams.canExecuteAsync = config.canExecuteAsync;
  auto solver = initializeTestProblemSolver(solverParams);
  solver->setObjectName("shard");
  solver->setContainerName("host");
  solver->setAssignment(initialAssignment);

  suspender.dismiss();

  for (const auto i : folly::irange(config.numDimensions)) {
    solver->addObjectDimension(fmt::format("dim_{}", i), shardDimension);
  }

  suspender.rehire();
}

BENCHMARK(AddObjectDimension) {
  addObjectDimensions(
      Config{
          .numShards = 240'000, .canExecuteAsync = false, .numDimensions = 1});
}

BENCHMARK(AddSeveralObjectDimension_notAsync) {
  addObjectDimensions(
      Config{
          .numShards = 10'000'000,
          .canExecuteAsync = false,
          .numDimensions = 10});
}

BENCHMARK(AddSeveralObjectDimension_Async) {
  addObjectDimensions(
      Config{
          .numShards = 10'000'000,
          .canExecuteAsync = true,
          .numDimensions = 10});
}

} // namespace facebook::rebalancer::interface::benchmarks

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
