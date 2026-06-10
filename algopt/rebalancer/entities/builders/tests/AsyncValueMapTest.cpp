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
#include "algopt/rebalancer/entities/builders/AsyncValueMap.h"

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

namespace facebook::rebalancer::tests {

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

struct IdData {
  IdData(int id, std::string data) : id(id), data(std::move(data)) {}

  IdData(const IdData& other) : id(other.id), data(other.data) {
    ++nCopies;
  }

  IdData(IdData&& other) noexcept : id(other.id), data(std::move(other.data)) {
    ++nMoves;
  }

  [[maybe_unused]] IdData& operator=(const IdData& other) = default;
  [[maybe_unused]] IdData& operator=(IdData&& other) noexcept = default;

  int id;
  std::string data;

  inline static std::atomic<int> nCopies{0};
  inline static std::atomic<int> nMoves{0};
};
} // namespace

class AsyncValueMapTest : public ::testing::TestWithParam<int> {
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
    AsyncValueMapTest,
    ::testing::Values(1, 3, 12, 30, 64));

CO_TEST_P(AsyncValueMapTest, ConcurrentSetGet) {
  auto executor = makeExecutor();
  AsyncValueMap<std::string, IdData> nameToIdMap;

  // register all keys serially and concurrently call get/set on them
  constexpr int nTasks = 50e3;
  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve(nTasks);
  for (const auto i : folly::irange(nTasks)) {
    const auto key = fmt::format("name_{}", i);
    nameToIdMap.registerKey(key);

    executeRandomDelay();

    tasks.push_back(
        folly::coro::co_invoke(
            [i, key, &nameToIdMap]() -> folly::coro::Task<void> {
              nameToIdMap.set(key, IdData{i, fmt::format("data_{}", i)});
              co_return;
            }));

    tasks.push_back(folly::coro::co_invoke([&]() -> folly::coro::Task<void> {
      executeRandomDelay();
      const auto idInt = folly::Random::rand32(nTasks);
      auto taskNameToAwait = fmt::format("name_{}", idInt);
      const auto idData = co_await nameToIdMap.get(taskNameToAwait);
      EXPECT_EQ(idData->id, idInt);
      EXPECT_EQ(idData->data, fmt::format("data_{}", idInt));
      co_return;
    }));
  }

  std::set<std::string> expectedKeys;
  for (const auto i : folly::irange(nTasks)) {
    expectedKeys.insert(fmt::format("name_{}", i));
  }
  EXPECT_EQ(expectedKeys, toSet<std::string>(nameToIdMap.keys()));

  // run all tasks on the executor
  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  // check that all insertions happened; also verify that no copies where made
  EXPECT_EQ(nameToIdMap.size(), nTasks);
  EXPECT_EQ(0, IdData::nCopies);
  EXPECT_EQ(nTasks, IdData::nMoves);

  // read all elements and check that they are correct
  struct Result {
    std::set<std::string> seenNames;
    std::set<int> seenIds;
    std::set<std::string> seenData;
  };
  Result result;
  co_await nameToIdMap.waitAndReadFromEach(
      [&](const auto& name, const auto& idData) {
        result.seenNames.insert(name);
        result.seenIds.insert(idData.id);
        result.seenData.insert(idData.data);
      });

  EXPECT_EQ(nTasks, result.seenNames.size());
  EXPECT_EQ(nTasks, result.seenIds.size());
  EXPECT_EQ(nTasks, result.seenData.size());
  for (const auto i : folly::irange(nTasks)) {
    EXPECT_TRUE(result.seenNames.contains(fmt::format("name_{}", i)));
    EXPECT_TRUE(result.seenIds.contains(i));
    EXPECT_TRUE(result.seenData.contains(fmt::format("data_{}", i)));
  }
}

CO_TEST_P(AsyncValueMapTest, CanCallGetWithoutRegisterIfSetIsCalledFirst) {
  AsyncValueMap<std::string, IdData> nameToIdMap;

  constexpr int keyId = 1;
  const auto key = fmt::format("name_{}", keyId);
  nameToIdMap.set(key, IdData{keyId, fmt::format("data_{}", keyId)});

  const auto idData = co_await nameToIdMap.get(key);
  EXPECT_EQ(idData->id, keyId);
  EXPECT_EQ(idData->data, fmt::format("data_{}", keyId));
}

TEST_P(AsyncValueMapTest, CallingSetTwiceOnSameKeyThrows) {
  AsyncValueMap<std::string, IdData> nameToIdMap;

  constexpr int keyId = 1;
  nameToIdMap.set(
      fmt::format("name_{}", keyId),
      IdData{keyId, fmt::format("data_{}", keyId)});
  EXPECT_EQ(nameToIdMap.size(), 1);

  auto set = [&nameToIdMap, keyId]() {
    nameToIdMap.set(
        fmt::format("name_{}", keyId),
        IdData{2 * keyId, fmt::format("data_{}", 2 * keyId)});
  };
  REBALANCER_EXPECT_RUNTIME_ERROR(
      set(),
      fmt::format(
          "Unexpected call to set() on previously fulfilled key 'name_{}'",
          keyId));
}

TEST_P(AsyncValueMapTest, GetBeforeRegisterOrSetThrows) {
  // get is called before either register or set is called
  AsyncValueMap<std::string, IdData> map;
  REBALANCER_EXPECT_RUNTIME_ERROR(
      std::ignore = folly::coro::blockingWait(map.get("missingKey")),
      "Unexpected call to get() w.r.t a key 'missingKey' that was neither explicitly registered nor set");
}

TEST_P(AsyncValueMapTest, RegisterKeyTwice) {
  AsyncValueMap<std::string, int> map;
  EXPECT_TRUE(map.registerKey("key"));
  EXPECT_FALSE(map.registerKey("key"));
}

CO_TEST_P(AsyncValueMapTest, ConcurrentSetGetWithRead) {
  auto executor = makeExecutor();
  AsyncValueMap<std::string, IdData> nameToIdMap;

  // register all keys serially and concurrently call get/set on them
  constexpr int nTasks = 50e3;
  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve(nTasks);
  for (const auto i : folly::irange(nTasks)) {
    const auto key = fmt::format("name_{}", i);
    nameToIdMap.registerKey(key);

    executeRandomDelay();

    tasks.push_back(
        folly::coro::co_invoke(
            [i, key, &nameToIdMap]() -> folly::coro::Task<void> {
              nameToIdMap.set(key, IdData{i, fmt::format("data_{}", i)});
              co_return;
            }));

    tasks.push_back(folly::coro::co_invoke([&]() -> folly::coro::Task<void> {
      executeRandomDelay();
      const auto idInt = folly::Random::rand32(nTasks);
      auto taskNameToAwait = fmt::format("name_{}", idInt);
      const auto idData = co_await nameToIdMap.get(taskNameToAwait);
      EXPECT_EQ(idData->id, idInt);
      EXPECT_EQ(idData->data, fmt::format("data_{}", idInt));
      co_return;
    }));
  }

  // run all tasks on the executor
  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  // read all elements and check that they are correct
  struct Result {
    std::set<std::string> seenNames;
    std::set<int> seenIds;
    std::set<std::string> seenData;
  };
  Result result;
  co_await nameToIdMap.waitAndReadFromEach(
      [&](const auto& name, const auto& idData) {
        result.seenNames.insert(name);
        result.seenIds.insert(idData.id);
        result.seenData.insert(idData.data);
      });

  EXPECT_EQ(nTasks, result.seenNames.size());
  EXPECT_EQ(nTasks, result.seenIds.size());
  EXPECT_EQ(nTasks, result.seenData.size());
  for (const auto i : folly::irange(nTasks)) {
    EXPECT_TRUE(result.seenNames.contains(fmt::format("name_{}", i)));
    EXPECT_TRUE(result.seenIds.contains(i));
    EXPECT_TRUE(result.seenData.contains(fmt::format("data_{}", i)));
  }

  // map should still be usable after waitAndReadFromEach
  EXPECT_EQ(nameToIdMap.size(), nTasks);
  const auto value = co_await nameToIdMap.get("name_0");
  EXPECT_EQ(value->id, 0);
}

CO_TEST_P(AsyncValueMapTest, MapUsableAfterRead) {
  AsyncValueMap<std::string, IdData> map;
  map.set("key", IdData{1, "data"});

  // Read should succeed
  int keyCount = 0;
  co_await map.waitAndReadFromEach(
      [&](const auto&, const auto&) { keyCount++; });
  EXPECT_EQ(1, keyCount);

  // All operations should still work after waitAndReadFromEach
  EXPECT_TRUE(map.registerKey("newKey"));

  map.set("newKey", IdData{2, "data2"});

  const auto value = co_await map.get("key");
  EXPECT_EQ(value->id, 1);

  auto keys = map.keys();
  EXPECT_EQ(keys.size(), 2);

  // A second waitAndReadFromEach should also succeed
  int secondCount = 0;
  co_await map.waitAndReadFromEach(
      [&](const auto&, const auto&) { secondCount++; });
  EXPECT_EQ(2, secondCount);
}

CO_TEST_P(AsyncValueMapTest, EmptyMapReadBehavior) {
  AsyncValueMap<std::string, int> map;
  EXPECT_EQ(map.size(), 0);
  EXPECT_TRUE(map.keys().empty());

  int count = 0;
  co_await map.waitAndReadFromEach([&](const auto&, const auto&) { count++; });
  EXPECT_EQ(count, 0);

  // map should still be usable after reading from empty map
  map.set("key", 42);
  EXPECT_EQ(map.size(), 1);
}

CO_TEST_P(AsyncValueMapTest, KeysDoesNotExtract) {
  AsyncValueMap<std::string, int> map;
  map.set("key", 42);

  // First call to keys()
  auto keys1 = map.keys();
  EXPECT_EQ(keys1.size(), 1);

  // Can still call get after keys()
  const auto value = co_await map.get("key");
  EXPECT_EQ(*value, 42);

  // Can call keys() again
  auto keys2 = map.keys();
  EXPECT_EQ(keys2.size(), 1);

  // Can still add new keys
  map.set("newKey", 100);
  auto keys3 = map.keys();
  EXPECT_EQ(keys3.size(), 2);
}

} // namespace facebook::rebalancer::tests
