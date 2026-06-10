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

#include "algopt/rebalancer/algopt_common/Cache.h"
#include "algopt/rebalancer/algopt_common/TestUtils.h"

#include "folly/concurrency/ConcurrentHashMap.h"
#include "folly/container/F14Map.h"
#include <gtest/gtest.h>

using namespace facebook::algopt;

class NonCopyableString {
 public:
  explicit NonCopyableString(const std::string& s) : s_(s) {}
  ~NonCopyableString() = default;

  [[noreturn]] NonCopyableString(const NonCopyableString& /*other*/) = delete;

  NonCopyableString(NonCopyableString&& other) noexcept {
    s_ = std::move(other.s_);
  }

  [[noreturn]] NonCopyableString& operator=(
      const NonCopyableString& /*other*/) = delete;

  [[noreturn]] NonCopyableString& operator=(
      NonCopyableString&& /*other*/) noexcept = delete;

  bool operator==(const NonCopyableString& other) const {
    return (s_ == other.s_);
  }

 protected:
  std::string s_;
};

class CopyableString : public NonCopyableString {
 public:
  explicit CopyableString(const std::string& s) : NonCopyableString(s) {}
  CopyableString(const CopyableString& other) : NonCopyableString(other.s_) {}
  CopyableString& operator=(const CopyableString& /*other*/) = delete;
};

TEST(CacheTest, BasicWithNonThreadSafeCacheAndNonCopyableString) {
  Cache<folly::F14FastMap<int, NonCopyableString>> cache;
  // when used with regular hash map like folly::F14, we want at() to return a
  // const reference. So explcitly test a case where copying is not allowed.
  EXPECT_EQ(NonCopyableString("1"), cache.getSavedOrCompute(1, [&]() {
    return NonCopyableString("1");
  }));
  EXPECT_EQ(NonCopyableString("1"), cache.at(1));

  EXPECT_EQ(NonCopyableString("2"), cache.getSavedOrCompute(2, [&]() {
    return NonCopyableString("2");
  }));
  EXPECT_EQ(NonCopyableString("2"), cache.at(2));

  EXPECT_EQ(NonCopyableString("3"), cache.getSavedOrCompute(3, [&]() {
    return NonCopyableString("3");
  }));
  EXPECT_EQ(NonCopyableString("3"), cache.at(3));
}

TEST(CacheTest, BasicWithThreadSafeCacheCopyNotAllowed) {
  // this test case is to show that when using thread-safe maps like
  // folly::ConcurrentHashMap, we expect at() to return a copy and so if copying
  // is not allowed, we expect an error.
  Cache<folly::ConcurrentHashMapSIMD<int, NonCopyableString>> cache;
  EXPECT_EQ(NonCopyableString("1"), cache.getSavedOrCompute(1, [&]() {
    return NonCopyableString("1");
  }));
}

TEST(CacheTest, BasicWithThreadSafeCacheCopyAllowed) {
  // this test case is to show that when using thread-safe maps like
  // folly::ConcurrentHashMap, we expect at() to return a copy and so just make
  // sure it works with copyable types
  Cache<folly::ConcurrentHashMapSIMD<int, CopyableString>> cache;
  EXPECT_EQ(CopyableString("1"), cache.getSavedOrCompute(1, [&]() {
    return CopyableString("1");
  }));
  EXPECT_EQ(CopyableString("1"), cache.at(1));
  EXPECT_EQ(CopyableString("1"), cache.atOrNullopt(1));

  EXPECT_EQ(CopyableString("2"), cache.getSavedOrCompute(2, [&]() {
    return CopyableString("2");
  }));
  EXPECT_EQ(CopyableString("2"), cache.at(2));
  EXPECT_EQ(CopyableString("2"), cache.atOrNullopt(2));

  EXPECT_EQ(std::nullopt, cache.atOrNullopt(3));
}
