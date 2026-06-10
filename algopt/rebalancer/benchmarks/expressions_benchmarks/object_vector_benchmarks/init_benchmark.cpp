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

#include "algopt/rebalancer/algopt_common/CompressedIdMap.h"
#include "algopt/rebalancer/solver/expressions/ObjectVector.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

namespace facebook::rebalancer::benchmarks {

void objectVectorInit(size_t numObjects, double value) {
  auto objectValues = std::make_shared<entities::ObjectIdToDoubleMap>(
      numObjects,
      /*defaultValue=*/0.0,
      /*expectedNonDefaultSize=*/numObjects);
  for (const auto i : folly::irange(numObjects)) {
    objectValues->emplace(entities::ObjectId(i), value);
  }
  auto universe = std::make_shared<entities::Universe>();
  folly::doNotOptimizeAway(
      ObjectVector(entities::ObjectValues(std::move(objectValues)), universe));
}

BENCHMARK(ObjectVectorInit_10K) {
  const int nVectors = 100e3;
  for (const auto i : folly::irange(nVectors)) {
    objectVectorInit(/*numObjects=*/10000, /*value=*/i);
  }
}

} // namespace facebook::rebalancer::benchmarks

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
