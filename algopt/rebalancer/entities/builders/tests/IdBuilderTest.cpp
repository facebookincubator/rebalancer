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
#include "algopt/rebalancer/entities/builders/IdBuilder.h"
#include "algopt/rebalancer/entities/builders/tests/BuilderTestBase.h"

#include <folly/container/irange.h>
#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/portability/GTest.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::entities::tests {

class IdBuilderTest : public BuilderTestBase {};

INSTANTIATE_TEST_CASE_P(NumThreads, IdBuilderTest, ::testing::Values(1, 10));

TEST_P(IdBuilderTest, IdBuilderPerTypeCounters) {
  IdBuilder idBuilder;
  EXPECT_EQ(0, static_cast<size_t>(idBuilder.next<ObjectId>()));
  EXPECT_EQ(0, static_cast<size_t>(idBuilder.next<ContainerId>()));

  constexpr size_t n = 100;
  for ([[maybe_unused]] const auto _ : folly::irange(n)) {
    idBuilder.next<ObjectId>();
  }
  EXPECT_EQ(1 + n, static_cast<size_t>(idBuilder.next<ObjectId>()));
  EXPECT_EQ(1, idBuilder.getCount<ContainerId>());
  EXPECT_EQ(2 + n, idBuilder.getCount<ObjectId>());
}

TEST_P(IdBuilderTest, IdBuilderCount) {
  IdBuilder idBuilder;
  EXPECT_EQ(0, idBuilder.getCount<ObjectId>());

  constexpr size_t nObjects = 50;
  constexpr size_t nContainers = 30;
  for (const auto _ : folly::irange(nObjects)) {
    idBuilder.next<ObjectId>();
  }
  for (const auto _ : folly::irange(nContainers)) {
    idBuilder.next<ContainerId>();
  }

  EXPECT_EQ(nObjects, idBuilder.getCount<ObjectId>());
  EXPECT_EQ(nContainers, idBuilder.getCount<ContainerId>());
}

CO_TEST_P(IdBuilderTest, ConcurrentNextIdProducesUniqueIds) {
  IdBuilder idBuilder;

  constexpr int nTasks = 10000;
  folly::Synchronized<std::set<EntityIdType>> allIds;

  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve(nTasks);

  for ([[maybe_unused]] const auto _ : folly::irange(nTasks)) {
    tasks.push_back(folly::coro::co_invoke([&]() -> folly::coro::Task<void> {
      executeRandomDelay();
      const auto id = idBuilder.next<ObjectId>();
      allIds.wlock()->insert(id.asInt());
      co_return;
    }));
  }

  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  EXPECT_EQ(nTasks, allIds.rlock()->size());
}

TEST_P(IdBuilderTest, DuplicateMakeIdFromNameThrows) {
  IdBuilder idBuilder;
  folly::Synchronized<Map<std::string, ScopeId>> scopeNameToId{
      {{"region", ScopeId(0)}}};
  AsyncValueMap<ScopeId, std::string> asyncMap;

  REBALANCER_EXPECT_RUNTIME_ERROR(
      std::ignore = idBuilder.makeIdFromName<ScopeId>(
          "region", scopeNameToId, asyncMap, "makeScopeId"),
      "Unexpected call to makeScopeId with a previously added name 'region'");
}

} // namespace facebook::rebalancer::entities::tests
