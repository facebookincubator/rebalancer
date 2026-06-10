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

#include "algopt/rebalancer/benchmarks/utils/BenchmarkReplayer.h"

#include <folly/Benchmark.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer::interface::benchmarks;

BENCHMARK(SMMultiRole) {
  replay("e693e7fa-d579-488d-863c-69ded4239b6f-v2");
}

BENCHMARK(SMSecondary) {
  replay("6ae2bfdd-c452-4072-bcc4-f488963ee7ec-corrected-v2");
}

BENCHMARK(SMPrimary) {
  replay("517a0984-6b71-400f-ad01-8b5ddf2fe7f6-v2");
}

BENCHMARK(SMAllocation) {
  replay("2e31d833-2b71-4376-b491-1d34f1b556f8-v2");
}

BENCHMARK(SMStickiness) {
  replay("491e95df-c47d-4181-863c-d2cbbaa08779-v2");
}

// SMAllocationTurbineCln
// SMAllocationAds
// SMAllocationZippy
// repo commands coming from T62320709
BENCHMARK(SMAllocationTurbineCln) {
  replay("14d2860f-cbc0-41af-b64e-c2063fc8d049-v2");
}

BENCHMARK(SMAllocationAds) {
  replay("0d6fff93-b9d8-4b12-a9d0-34efda47b425-v2");
}

BENCHMARK(SMAllocationZippy) {
  replay("e2f53c04-7b9d-4c7d-ba1f-ecf0d5fbf0a2-v2");
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
