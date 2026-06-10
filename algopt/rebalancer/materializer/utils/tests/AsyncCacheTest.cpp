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
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/coro/Task.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/portability/GTest.h>
#include <folly/Random.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

namespace {
double fib(std::uint32_t n) {
  if (n <= 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

void executeRandomDelay() {
  const auto n = folly::Random::rand32(15);
  const auto fibN = fib(n);
  folly::doNotOptimizeAway(fibN);
}

} // namespace

class AsyncCacheTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    kNumThreads = GetParam();
  }

  std::shared_ptr<folly::CPUThreadPoolExecutor> makeExecutor() {
    return std::make_shared<folly::CPUThreadPoolExecutor>(kNumThreads);
  }

  int kNumThreads{1};
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    AsyncCacheTest,
    ::testing::Values(1, 2, 30, 100));

CO_TEST_P(AsyncCacheTest, SameKeyConcurrentAndSequential) {
  auto executor = makeExecutor();
  using Value = std::shared_ptr<std::string>;
  AsyncCache<int, Value> cache;
  std::atomic<int> computeCount{0};
  constexpr int numCallers = 100;
  constexpr int key = 42;

  // Phase 1: Concurrent access - only one should compute
  std::vector<folly::coro::Task<Value>> tasks;
  tasks.reserve(numCallers);
  for ([[maybe_unused]] const auto _ : folly::irange(numCallers)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&cache, &computeCount, key]() -> folly::coro::Task<Value> {
              executeRandomDelay();
              co_return co_await cache.getSavedOrCompute(
                  key, [&]() -> folly::coro::Task<Value> {
                    ++computeCount;
                    executeRandomDelay();
                    co_return std::make_shared<std::string>("computed_value");
                  });
            }));
  }

  auto results = co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  for (const auto& result : results) {
    EXPECT_EQ("computed_value", *result);
  }
  EXPECT_EQ(1, computeCount.load());

  // access after caching - should return cached
  const auto& cachedValue =
      co_await cache.getSavedOrCompute(key, [&]() -> folly::coro::Task<Value> {
        ++computeCount;
        co_return std::make_shared<std::string>("should_not_be_returned");
      });

  EXPECT_EQ("computed_value", *cachedValue);
  EXPECT_EQ(1, computeCount.load()); // Still 1, should not recompute

  // insert several new values
  constexpr int kNewKeys = 1000;
  for (const auto i : folly::irange(kNewKeys)) {
    co_await cache.getSavedOrCompute(i, [&]() -> folly::coro::Task<Value> {
      ++computeCount;
      co_return std::make_shared<std::string>(
          fmt::format("computed_value_{}", i));
    });
  }

  // old reference should remain valid
  EXPECT_EQ("computed_value", *cachedValue);
}

CO_TEST_P(AsyncCacheTest, ConcurrentDifferentKeys) {
  auto executor = makeExecutor();
  AsyncCache<int, int> cache;
  std::atomic<int> computeCount{0};
  constexpr int numCalls = 50'000;
  constexpr int numUniqueKeys = 10'000;

  std::vector<folly::coro::Task<int>> tasks;
  tasks.reserve(numCalls);
  for (const auto i : folly::irange(numCalls)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&cache, &computeCount, i]() -> folly::coro::Task<int> {
              executeRandomDelay();
              const auto key = i % numUniqueKeys;
              co_return co_await cache.getSavedOrCompute(
                  key, [&computeCount, key]() -> folly::coro::Task<int> {
                    ++computeCount;
                    co_return key * 10;
                  });
            }));
  }

  auto results = co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  EXPECT_EQ(numUniqueKeys, cache.size());

  for (const auto i : folly::irange(numUniqueKeys)) {
    EXPECT_EQ(i * 10, results[i]);
  }

  EXPECT_EQ(numUniqueKeys, computeCount.load());
}

CO_TEST_P(AsyncCacheTest, ExceptionPropagation) {
  auto executor = makeExecutor();
  AsyncCache<int, std::string> cache;
  std::atomic<int> computeCount{0};
  std::atomic<int> exceptionCount{0};
  constexpr int numCallers = 20;

  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve(numCallers);
  for ([[maybe_unused]] const auto _ : folly::irange(numCallers)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&cache,
             &computeCount,
             &exceptionCount]() -> folly::coro::Task<void> {
              try {
                executeRandomDelay();
                co_await cache.getSavedOrCompute(
                    42, [&]() -> folly::coro::Task<std::string> {
                      ++computeCount;
                      // Delay so other callers start waiting
                      executeRandomDelay();
                      throw std::runtime_error("producer failed");
                      co_return "never reached";
                    });
              } catch (const std::runtime_error&) {
                ++exceptionCount;
              }
            }));
  }

  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  // Only one should have computed (and thrown)
  EXPECT_EQ(1, computeCount.load());
  // All callers should have received the exception
  EXPECT_EQ(numCallers, exceptionCount.load());
}

} // namespace facebook::rebalancer::materializer::tests
