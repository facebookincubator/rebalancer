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

#include "algopt/rebalancer/algopt_common/TestUtils.h"
#include "algopt/rebalancer/materializer/utils/Cache.h"

#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <gtest/gtest.h>

using namespace facebook::rebalancer::materializer;
constexpr int kHighNumThreads = 300;
constexpr int kNumThreads = 30;

TEST(CacheTest, Basic) {
  Cache<int, std::string> cache;

  int key = 42;
  std::string value = "abc";

  int missCount = 0;
  auto onMiss = [&]() {
    ++missCount;
    return value;
  };

  EXPECT_EQ(value, cache.getSavedOrCompute(key, onMiss));
  EXPECT_EQ(1, missCount);

  key = 100;
  value = "abcabc";

  EXPECT_EQ(value, cache.getSavedOrCompute(key, onMiss));
  EXPECT_EQ(2, missCount);
}

TEST(CacheInsertUniqueKeysTest, Basic) {
  Cache<int, std::string> cache;
  int i;
  constexpr int key = 42;
  std::string value = "abc";

  int missCount = 0;
  auto onMiss = [&]() {
    ++missCount;
    return value;
  };
  for (i = 0; i < 30; i++) {
    EXPECT_EQ(value, cache.getSavedOrCompute(key + i, onMiss));
  }
  EXPECT_EQ(i, missCount);
}

TEST(CacheGetSavedTest, Basic) {
  Cache<int, int> cache;
  constexpr int keyCount = 10;

  for (const auto i : folly::irange(keyCount)) {
    cache.getSavedOrCompute(i, [i]() { return i; });
  }

  for (const auto i : folly::irange(keyCount)) {
    EXPECT_EQ(cache.at(i), i);
  }
}

TEST(CacheGetSavedTest, ThrowIfNotSaved) {
  Cache<int, int> cache;
  // trying to at(0) should result in an exception
  REBALANCER_EXPECT_RUNTIME_ERROR(cache.at(0), "key 0 does not exist");
}

TEST(CacheClearAndSizeTest, Basic) {
  Cache<int, int> cache;
  constexpr int keyCount = 10;

  for (const auto i : folly::irange(keyCount)) {
    cache.getSavedOrCompute(i, [i]() { return i; });
  }
  EXPECT_EQ(cache.size(), keyCount);
  EXPECT_EQ(cache.at(0), 0);

  // clear the cache and test size again
  cache.clear();

  EXPECT_EQ(cache.size(), 0);
  REBALANCER_EXPECT_RUNTIME_ERROR(cache.at(0), "key 0 does not exist");
}

TEST(MultiThreadCacheTest, Basic) {
  Cache<int, std::string> cache;
  auto executor = folly::CPUThreadPoolExecutor(kNumThreads);
  std::atomic<int> missCount = 0;
  for (const auto _ : folly::irange(kNumThreads)) {
    executor.add([&]() {
      std::string value = "abc";
      auto onMiss = [&]() {
        ++missCount;
        return value;
      };
      auto cacheVal = cache.getSavedOrCompute(20, onMiss);
      EXPECT_EQ(value, cacheVal);

      value = "abcabc";
      cacheVal = cache.getSavedOrCompute(25, onMiss);
      EXPECT_EQ(value, cacheVal);
    });
  }
  executor.join();
  EXPECT_EQ(2, missCount);
}

TEST(MultiThreadCacheSameKeyInsertTest, Basic) {
  Cache<int, std::string> cache;
  auto executor = folly::CPUThreadPoolExecutor(kNumThreads);
  std::atomic<int> missCount = 0;
  for (const auto _ : folly::irange(kNumThreads)) {
    executor.add([&]() {
      std::string value = "abc";
      auto onMiss = [&]() {
        ++missCount;
        return value;
      };
      auto cacheVal = cache.getSavedOrCompute(20, onMiss);
      EXPECT_EQ(value, cacheVal);
      EXPECT_EQ(value, cacheVal);
      EXPECT_EQ(value, cacheVal);
    });
  }
  executor.join();
  EXPECT_EQ(1, missCount);
}

TEST(MultiThreadCacheMultipleKeyTest, Basic) {
  Cache<int, std::string> cache;
  auto executor = folly::CPUThreadPoolExecutor(kNumThreads);
  std::atomic<int> missCount = 0;

  auto value = std::string("abc");

  auto onMiss = [&]() {
    ++missCount;
    return value;
  };
  for (const auto i : folly::irange(kNumThreads)) {
    executor.add([&, i]() {
      auto cacheVal = cache.getSavedOrCompute(20 + i, onMiss);
      EXPECT_EQ(value, cacheVal);
    });
  }

  executor.join();

  auto cacheVal = cache.getSavedOrCompute(25, onMiss);
  EXPECT_EQ(value, cacheVal);

  cacheVal = cache.getSavedOrCompute(23, onMiss);
  EXPECT_EQ(value, cacheVal);
}

TEST(SingleEntryCacheTest, Basic) {
  SingleEntryCache<std::string> cache;
  std::string value = "abc";
  int missCount = 0;
  auto onMiss = [&]() {
    ++missCount;
    return value;
  };
  EXPECT_EQ(value, cache.getSavedOrCompute(onMiss));
  EXPECT_EQ(1, missCount);
}

TEST(MultithreadSingleEntryCacheTest, Basic) {
  auto executor = folly::CPUThreadPoolExecutor(kNumThreads);
  SingleEntryCache<std::string> cache;
  int missCount = 0;
  for (const auto _ : folly::irange(kNumThreads)) {
    executor.add([&]() {
      std::string value = "abc";
      auto onMiss = [&]() {
        ++missCount;
        return value;
      };
      auto savedVal = cache.getSavedOrCompute(onMiss);
      EXPECT_EQ(value, savedVal);
      EXPECT_EQ(1, missCount);
    });
  }
  EXPECT_EQ(kNumThreads, executor.numThreads());
  executor.join();
}

TEST(MultithreadSingleEntryCacheIntTest, Basic) {
  auto executor = folly::CPUThreadPoolExecutor(kNumThreads);
  SingleEntryCache<int> cache;
  int missCount = 0;
  for (const auto _ : folly::irange(kNumThreads)) {
    executor.add([&]() {
      auto value = 234;
      auto onMiss = [&]() {
        ++missCount;
        return value;
      };
      auto savedVal = cache.getSavedOrCompute(onMiss);
      EXPECT_EQ(value, savedVal);
      EXPECT_EQ(1, missCount);
    });
  }
  EXPECT_EQ(kNumThreads, executor.numThreads());
  executor.join();
}

TEST(MultiCacheSingleHashMapTest, Basic) {
  auto executor = folly::CPUThreadPoolExecutor(kHighNumThreads);
  SingleEntryCache<std::map<double, double>> cache;
  int missCount = 0;
  for (const auto _ : folly::irange(kHighNumThreads)) {
    executor.add([&]() {
      std::map<double, double> value = {{0.01, 0.01}, {0.01, 0.02}};
      auto onMiss = [&]() {
        ++missCount;
        return value;
      };
      auto savedVal = cache.getSavedOrCompute(onMiss);
      EXPECT_EQ(value, savedVal);
      EXPECT_EQ(1, missCount);
    });
  }
  EXPECT_EQ(kHighNumThreads, executor.numThreads());
  executor.join();
}
