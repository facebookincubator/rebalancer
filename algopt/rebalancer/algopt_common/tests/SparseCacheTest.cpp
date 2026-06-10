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

#include "algopt/rebalancer/algopt_common/SparseCache.h"

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace facebook::algopt {

TEST(SparseCacheTest, SecondCallReturnsCached) {
  SparseCache<uint64_t, int> cache;
  int callCount = 0;
  cache.getSavedOrCompute(5, [&] { return ++callCount; });
  cache.getSavedOrCompute(5, [&] { return ++callCount; });
  EXPECT_EQ(callCount, 1);
  EXPECT_EQ(cache.getSavedOrCompute(5, [&] { return ++callCount; }), 1);
}

TEST(SparseCacheTest, AtOrNulloptMissing) {
  const SparseCache<uint64_t, int> cache;
  EXPECT_EQ(cache.atOrNullopt(0), std::nullopt);
  EXPECT_EQ(cache.atOrNullopt(9999), std::nullopt);
}

TEST(SparseCacheTest, AtOrNulloptAfterCompute) {
  SparseCache<uint64_t, int> cache;
  cache.getSavedOrCompute(42, [] { return 100; });
  const auto result = cache.atOrNullopt(42);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, 100);
}

TEST(SparseCacheTest, MultipleChunksAndBoundary) {
  // 4095 and 4096 straddle the 4096-entry chunk boundary; 8192 is a third
  // chunk. Each key must resolve to its own entry across chunks.
  SparseCache<uint64_t, int> cache;
  cache.getSavedOrCompute(4095, [] { return 1; });
  cache.getSavedOrCompute(4096, [] { return 2; });
  cache.getSavedOrCompute(8192, [] { return 3; });
  EXPECT_EQ(*cache.atOrNullopt(4095), 1);
  EXPECT_EQ(*cache.atOrNullopt(4096), 2);
  EXPECT_EQ(*cache.atOrNullopt(8192), 3);
}

TEST(SparseCacheTest, ConcurrentComputeOnSameKey) {
  SparseCache<uint64_t, int> cache;
  std::atomic<int> callCount{0};
  std::vector<std::thread> threads;
  threads.reserve(10);
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&cache, &callCount] {
      cache.getSavedOrCompute(42, [&] { return ++callCount; });
    });
  }
  for (auto& t : threads) {
    t.join();
  }
  EXPECT_EQ(callCount, 1);
  EXPECT_EQ(*cache.atOrNullopt(42), 1);
}

TEST(SparseCacheTest, ConcurrentComputeOnDifferentKeys) {
  SparseCache<uint64_t, int> cache;
  std::vector<std::thread> threads;
  threads.reserve(100);
  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([&cache, i] {
      cache.getSavedOrCompute(static_cast<uint64_t>(i), [i] { return i * 10; });
    });
  }
  for (auto& t : threads) {
    t.join();
  }
  for (int i = 0; i < 100; ++i) {
    EXPECT_EQ(*cache.atOrNullopt(static_cast<uint64_t>(i)), i * 10);
  }
}

TEST(SparseCacheTest, EnumKey) {
  enum class ExprId : uint64_t { A = 0, B = 5000, C = 10000 };
  SparseCache<ExprId, double> cache;
  cache.getSavedOrCompute(ExprId::A, [] { return 1.0; });
  cache.getSavedOrCompute(ExprId::B, [] { return 2.0; });
  cache.getSavedOrCompute(ExprId::C, [] { return 3.0; });
  EXPECT_EQ(*cache.atOrNullopt(ExprId::A), 1.0);
  EXPECT_EQ(*cache.atOrNullopt(ExprId::B), 2.0);
  EXPECT_EQ(*cache.atOrNullopt(ExprId::C), 3.0);
}

// Two caches of the same type, accessed with the same keys on one thread, must
// not share state: each instance has its own per-thread chunk cache, so the
// most-recently-used chunk of one is never returned for the other. Guards
// against regressing to a process-wide (static) thread-local cache.
TEST(SparseCacheTest, DistinctInstancesDoNotAlias) {
  SparseCache<uint64_t, int> a;
  SparseCache<uint64_t, int> b;
  for (int i = 0; i < 5; ++i) {
    const auto key = static_cast<uint64_t>(i);
    EXPECT_EQ(a.getSavedOrCompute(key, [i] { return i; }), i);
    EXPECT_EQ(b.getSavedOrCompute(key, [i] { return i + 1000; }), i + 1000);
  }
  for (int i = 0; i < 5; ++i) {
    const auto key = static_cast<uint64_t>(i);
    EXPECT_EQ(*a.atOrNullopt(key), i);
    EXPECT_EQ(*b.atOrNullopt(key), i + 1000);
  }
}

} // namespace facebook::algopt
