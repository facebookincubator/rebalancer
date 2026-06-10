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

#include "algopt/rebalancer/materializer/utils/Cache.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>
#include <folly/Random.h>
#include <folly/Synchronized.h>
#include <gtest/gtest.h>

using namespace facebook::rebalancer::materializer;

static int fibonacci(int n) {
  if (n == 0) {
    return 0;
  }
  if (n == 1) {
    return 1;
  }
  return fibonacci(n - 1) + fibonacci(n - 2);
}

static void run(
    int benchmarkIters,
    int keyCount,
    int jobCount,
    int threadCount,
    int workDifficulty) {
  for (const auto _ : folly::irange(benchmarkIters)) {
    folly::BenchmarkSuspender suspend;
    folly::CPUThreadPoolExecutor executor(threadCount);
    std::atomic<int> misses = 0;
    suspend.dismiss();

    auto onMiss = [&misses, &workDifficulty]() {
      const folly::BenchmarkSuspender suspendOnMiss;
      int uniqueResult = 0;
      const int uniqueId = misses++;
      uniqueResult = fibonacci(workDifficulty) + uniqueId;
      return uniqueResult;
    };

    Cache<int, int> cache;

    auto doJob = [&](int jobIndex) {
      folly::BenchmarkSuspender suspendJob;
      std::vector<int> allKeys(keyCount);
      for (const auto i : folly::irange(keyCount)) {
        allKeys.push_back(i);
      }
      std::default_random_engine rng(jobIndex);
      std::shuffle(allKeys.begin(), allKeys.end(), rng);
      std::vector<int> values(allKeys.size(), 0);
      suspendJob.dismiss();

      for (auto key : allKeys) {
        auto value = cache.getSavedOrCompute(key, onMiss);
        values[key] = value;
      }
      return values;
    };

    folly::Synchronized<std::vector<std::vector<int>>> results;
    for (const auto i : folly::irange(jobCount)) {
      executor.add([&results, &doJob, i]() {
        auto values = doJob(i);
        results.wlock()->push_back(std::move(values));
      });
    }

    executor.join();

    BENCHMARK_SUSPEND {
      ASSERT_EQ(keyCount, misses);
      results.withRLock([&](const auto& lockedResults) {
        ASSERT_EQ(jobCount, lockedResults.size());
        for (const auto i : folly::irange(1, jobCount)) {
          ASSERT_EQ(lockedResults.at(0), lockedResults.at(i));
        }
      });
    }
  }
}

/*
The following benchmarks try to write each of the `keyCount` number of keys once
into a cache and read each of them potentially several times. The main
parameters that changes across benchmarks are `jobCount` and `threadCount`. The
parameter `threadCount` controls the number of threads, whereas `jobCount`
refers to the number of jobs, where each job j tries to access every key.
Depending on whether job j is the first to access the key, it either writes or
reads the value corresponding to the key. Therefore, when jobCount = J and
keyCount = K, we have K writes and K*(J-1) reads.
*/

int keyCount = 5000000;
/*
Single threaded scenario: although thread-safe, it is possible that the cache
can be used with just one thread
*/
BENCHMARK_NAMED_PARAM(run, writeOnly_1Thread, keyCount, 1, 1, 4)
BENCHMARK_NAMED_PARAM(run, 1xRead_1xWrite_1Thread, keyCount, 2, 1, 4)
BENCHMARK_NAMED_PARAM(run, 2xRead_1xWrite_1Thread, keyCount, 3, 1, 4)
BENCHMARK_NAMED_PARAM(run, 3xRead_1xWrite_1Thread, keyCount, 4, 1, 4)
BENCHMARK_NAMED_PARAM(run, 5xRead_1xWrite_1Thread, keyCount, 6, 1, 4)
BENCHMARK_NAMED_PARAM(run, 10xRead_1xWrite_1Thread, keyCount, 11, 1, 4)
BENCHMARK_NAMED_PARAM(run, 20xRead_1xWrite_1Thread, keyCount, 21, 1, 4)

BENCHMARK_DRAW_LINE();
/*
Benchmarks with 5 threads
*/
BENCHMARK_NAMED_PARAM(run, writeOnly_5Threads, keyCount, 1, 5, 4)
BENCHMARK_NAMED_PARAM(run, 1xRead_1xWrite_5Threads, keyCount, 2, 5, 4)
BENCHMARK_NAMED_PARAM(run, 2xRead_1xWrite_5Threads, keyCount, 3, 1, 4)
BENCHMARK_NAMED_PARAM(run, 3xRead_1xWrite_5Threads, keyCount, 4, 5, 4)
BENCHMARK_NAMED_PARAM(run, 5xRead_1xWrite_5Threads, keyCount, 6, 5, 4)
BENCHMARK_NAMED_PARAM(run, 10xRead_1xWrite_5Threads, keyCount, 11, 5, 4)
BENCHMARK_NAMED_PARAM(run, 20xRead_1xWrite_5Threads, keyCount, 21, 5, 4)

BENCHMARK_DRAW_LINE();
/*
Benchmarks with 20 threads
*/
BENCHMARK_NAMED_PARAM(run, writeOnly_20Threads, keyCount, 1, 20, 4)
BENCHMARK_NAMED_PARAM(run, 1xRead_1xWrite_20Threads, keyCount, 2, 20, 4)
BENCHMARK_NAMED_PARAM(run, 2xRead_1xWrite_20Threads, keyCount, 3, 1, 4)
BENCHMARK_NAMED_PARAM(run, 3xRead_1xWrite_20Threads, keyCount, 4, 20, 4)
BENCHMARK_NAMED_PARAM(run, 5xRead_1xWrite_20Threads, keyCount, 6, 20, 4)
BENCHMARK_NAMED_PARAM(run, 10xRead_1xWrite_20Threads, keyCount, 11, 20, 4)
BENCHMARK_NAMED_PARAM(run, 20xRead_1xWrite_20Threads, keyCount, 21, 20, 4)

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
