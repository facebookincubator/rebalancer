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
#include "algopt/rebalancer/entities/builders/DimensionsBuilder.h"
#include "algopt/rebalancer/entities/builders/tests/BuilderTestBase.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/portability/GTest.h>

namespace facebook::rebalancer::entities::tests {

constexpr int kObjectCount = 50;

class DimensionsBuilderTest : public BuilderTestBase {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    DimensionsBuilderTest,
    ::testing::Values(1, 3, 20));

CO_TEST_P(DimensionsBuilderTest, AddDimensions) {
  DimensionsBuilder dimensionsBuilder(idBuilder_);
  const auto scopeId = idBuilder_.next<ScopeId>();

  const auto memoryId = dimensionsBuilder.makeObjectDimensionId("memory");
  const auto capacityId =
      dimensionsBuilder.makeScopeDimensionId("capacity", scopeId);
  EXPECT_NE(memoryId, capacityId);

  const auto sharedObjectDimId =
      dimensionsBuilder.makeObjectDimensionId("sharedDimension");
  const auto sharedScopeDimId =
      dimensionsBuilder.makeScopeDimensionId("sharedDimension", scopeId);

  EXPECT_EQ(sharedObjectDimId, sharedScopeDimId);
  EXPECT_EQ(
      sharedObjectDimId, dimensionsBuilder.getDimensionId("sharedDimension"));

  co_await folly::coro::co_withExecutor(
      executor.get(),
      folly::coro::collectAll(
          dimensionsBuilder.add(
              memoryId,
              ObjectDimensionData{
                  .dimension = std::make_shared<const ObjectDimension>(
                      ObjectIdToDoubleMap(
                          /*totalSize=*/kObjectCount,
                          /*defaultValue=*/10.0,
                          /*expectedNonDefaultSize=*/0))}),
          dimensionsBuilder.add(
              capacityId,
              scopeId,
              ScopeDimensionData{
                  .dimension = std::make_shared<const ScopeDimension>(
                      Map<ScopeItemId, double>{}, 100.0)}),
          dimensionsBuilder.add(
              sharedObjectDimId,
              ObjectDimensionData{
                  .dimension = std::make_shared<const ObjectDimension>(
                      ObjectIdToDoubleMap(
                          /*totalSize=*/kObjectCount,
                          /*defaultValue=*/1.0,
                          /*expectedNonDefaultSize=*/0))}),
          dimensionsBuilder.add(
              sharedScopeDimId,
              scopeId,
              ScopeDimensionData{
                  .dimension = std::make_shared<const ScopeDimension>(
                      Map<ScopeItemId, double>{}, 50.0)})));

  const auto result = co_await dimensionsBuilder.build();

  // 3 unique names: memory, capacity, sharedDimension
  EXPECT_EQ(3, result.dimensionNameToId.size());
  EXPECT_EQ(memoryId, result.dimensionNameToId.at("memory"));
  EXPECT_EQ(capacityId, result.dimensionNameToId.at("capacity"));
  EXPECT_EQ(sharedObjectDimId, result.dimensionNameToId.at("sharedDimension"));

  // 2 object dimensions: memory, shared
  EXPECT_EQ(2, result.dimensionIdToObjectDimension.size());
  EXPECT_TRUE(result.dimensionIdToObjectDimension.contains(memoryId));
  EXPECT_TRUE(result.dimensionIdToObjectDimension.contains(sharedObjectDimId));

  // 2 scope dimensions under scopeId: capacity, shared
  EXPECT_EQ(1, result.scopeIdToDimensionIdToDimension.size());
  const auto& scopeDimensions =
      result.scopeIdToDimensionIdToDimension.at(scopeId);
  EXPECT_EQ(2, scopeDimensions.size());
  EXPECT_TRUE(scopeDimensions.contains(capacityId));
  EXPECT_TRUE(scopeDimensions.contains(sharedScopeDimId));
}

CO_TEST_P(DimensionsBuilderTest, ConcurrentAdd) {
  DimensionsBuilder dimensionsBuilder(idBuilder_);
  const auto scopeId = idBuilder_.next<ScopeId>();

  // Dimensions with same name (shared ID between object and scope)
  constexpr int nSharedDimensions = 250;
  std::vector<DimensionId> sharedObjectDimIds;
  std::vector<DimensionId> sharedScopeDimIds;
  sharedObjectDimIds.reserve(nSharedDimensions);
  sharedScopeDimIds.reserve(nSharedDimensions);
  for (const auto i : folly::irange(nSharedDimensions)) {
    const auto name = fmt::format("shared_dimension_{}", i);
    const auto objectDimensionId =
        dimensionsBuilder.makeObjectDimensionId(name);
    const auto scopeDimensionId =
        dimensionsBuilder.makeScopeDimensionId(name, scopeId);
    sharedObjectDimIds.push_back(objectDimensionId);
    sharedScopeDimIds.push_back(scopeDimensionId);
    EXPECT_EQ(objectDimensionId, scopeDimensionId);
  }

  // Dimensions with different names (distinct IDs)
  constexpr int nDistinctDimensions = 250;
  std::vector<DimensionId> distinctObjectDimIds;
  std::vector<DimensionId> distinctScopeDimIds;
  distinctObjectDimIds.reserve(nDistinctDimensions);
  distinctScopeDimIds.reserve(nDistinctDimensions);
  for (const auto i : folly::irange(nDistinctDimensions)) {
    distinctObjectDimIds.push_back(
        dimensionsBuilder.makeObjectDimensionId(fmt::format("obj_dim_{}", i)));
    distinctScopeDimIds.push_back(dimensionsBuilder.makeScopeDimensionId(
        fmt::format("scope_dim_{}", i), scopeId));
  }

  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve((nSharedDimensions + nDistinctDimensions) * 2);

  auto addObjectDimensionTask =
      [&dimensionsBuilder](DimensionId id) -> folly::coro::Task<void> {
    executeRandomDelay();
    co_await dimensionsBuilder.add(
        id,
        ObjectDimensionData{
            .dimension =
                std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
                    /*totalSize=*/kObjectCount,
                    /*defaultValue=*/1.0,
                    /*expectedNonDefaultSize=*/0))});
  };

  auto addScopeDimensionTask =
      [&dimensionsBuilder, scopeId](DimensionId id) -> folly::coro::Task<void> {
    executeRandomDelay();
    co_await dimensionsBuilder.add(
        id,
        scopeId,
        ScopeDimensionData{
            .dimension = std::make_shared<const ScopeDimension>(
                Map<ScopeItemId, double>{}, 100.0)});
  };

  for (const auto i : folly::irange(nSharedDimensions)) {
    tasks.push_back(addObjectDimensionTask(sharedObjectDimIds[i]));
    tasks.push_back(addScopeDimensionTask(sharedScopeDimIds[i]));
  }

  for (const auto i : folly::irange(nDistinctDimensions)) {
    tasks.push_back(addObjectDimensionTask(distinctObjectDimIds[i]));
    tasks.push_back(addScopeDimensionTask(distinctScopeDimIds[i]));
  }

  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  const auto result = co_await dimensionsBuilder.build();

  constexpr int nExpectedNames = nSharedDimensions + 2 * nDistinctDimensions;
  EXPECT_EQ(nExpectedNames, result.dimensionNameToId.size());

  // nSharedDimensions + nDistinctDimensions object dimensions
  constexpr int expectedObjectDimensions =
      nSharedDimensions + nDistinctDimensions;
  EXPECT_EQ(
      expectedObjectDimensions, result.dimensionIdToObjectDimension.size());

  // nSharedDimensions + nDistinctDimensions scope dimensions under scopeId
  constexpr int expectedScopeDimensions =
      nSharedDimensions + nDistinctDimensions;
  EXPECT_EQ(1, result.scopeIdToDimensionIdToDimension.size());
  EXPECT_EQ(
      expectedScopeDimensions,
      result.scopeIdToDimensionIdToDimension.at(scopeId).size());
}

TEST_P(DimensionsBuilderTest, DuplicateMakeObjectOrScopeDimensionIdThrows) {
  DimensionsBuilder dimensionsBuilder(idBuilder_);
  const auto scopeId1 = idBuilder_.next<ScopeId>();
  std::ignore = dimensionsBuilder.makeObjectDimensionId("memory");
  REBALANCER_EXPECT_RUNTIME_ERROR_CONTAINS(
      dimensionsBuilder.makeObjectDimensionId("memory"),
      "Unexpected call to makeObjectDimensionId with a previously added dimension name 'memory'");

  std::ignore = dimensionsBuilder.makeScopeDimensionId("memory", scopeId1);
  REBALANCER_EXPECT_RUNTIME_ERROR_CONTAINS(
      dimensionsBuilder.makeScopeDimensionId("memory", scopeId1),
      "Unexpected call to makeScopeDimensionId with a previously added dimension name 'memory'");

  // it is ok to have same name for scopeDimension if they are defined on
  // different scopes
  const auto scopeId2 = idBuilder_.next<ScopeId>();
  EXPECT_NO_THROW(dimensionsBuilder.makeScopeDimensionId("memory", scopeId2));
}

CO_TEST_P(DimensionsBuilderTest, Summarize) {
  DimensionsBuilder builder(idBuilder_);
  const auto scopeId = idBuilder_.next<ScopeId>();

  const auto memoryId = builder.makeObjectDimensionId("memory");
  const auto capacityId = builder.makeScopeDimensionId("capacity", scopeId);

  co_await folly::coro::collectAll(
      builder.add(
          memoryId,
          ObjectDimensionData{
              .dimension =
                  std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
                      /*totalSize=*/kObjectCount,
                      /*defaultValue=*/10.0,
                      /*expectedNonDefaultSize=*/0))}),
      builder.add(
          capacityId,
          scopeId,
          ScopeDimensionData{
              .dimension = std::make_shared<const ScopeDimension>(
                  Map<ScopeItemId, double>{}, 100.0)}));

  const auto result = co_await builder.summarize();

  const std::string expected =
      "Dimensions:\n"
      "  Object Dimensions:\n"
      "    memory\n"
      "  Scope Dimensions:\n"
      "    capacity\n";
  EXPECT_EQ(expected, result);
}

CO_TEST_P(DimensionsBuilderTest, BuildAddDimensionsAndRebuild) {
  DimensionsBuilder dimensionsBuilder(idBuilder_);
  const auto scopeId = idBuilder_.next<ScopeId>();

  const auto memoryId = dimensionsBuilder.makeObjectDimensionId("memory");
  const auto capacityId =
      dimensionsBuilder.makeScopeDimensionId("capacity", scopeId);

  co_await dimensionsBuilder.add(
      memoryId,
      ObjectDimensionData{
          .dimension =
              std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
                  /*totalSize=*/kObjectCount,
                  /*defaultValue=*/10.0,
                  /*expectedNonDefaultSize=*/0))});
  co_await dimensionsBuilder.add(
      capacityId,
      scopeId,
      ScopeDimensionData{
          .dimension = std::make_shared<const ScopeDimension>(
              Map<ScopeItemId, double>{}, 100.0)});

  const auto result1 = co_await dimensionsBuilder.build();
  EXPECT_EQ(2, result1.dimensionNameToId.size());
  EXPECT_EQ(memoryId, result1.dimensionNameToId.at("memory"));
  EXPECT_EQ(capacityId, result1.dimensionNameToId.at("capacity"));
  EXPECT_EQ(1, result1.dimensionIdToObjectDimension.size());
  EXPECT_EQ(1, result1.scopeIdToDimensionIdToDimension.size());
  EXPECT_EQ(1, result1.scopeIdToDimensionIdToDimension.at(scopeId).size());

  const auto cpuId = dimensionsBuilder.makeObjectDimensionId("cpu");
  const auto bandwidthId =
      dimensionsBuilder.makeScopeDimensionId("bandwidth", scopeId);

  co_await dimensionsBuilder.add(
      cpuId,
      ObjectDimensionData{
          .dimension =
              std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
                  /*totalSize=*/kObjectCount,
                  /*defaultValue=*/5.0,
                  /*expectedNonDefaultSize=*/0))});
  co_await dimensionsBuilder.add(
      bandwidthId,
      scopeId,
      ScopeDimensionData{
          .dimension = std::make_shared<const ScopeDimension>(
              Map<ScopeItemId, double>{}, 50.0)});

  const auto result2 = co_await dimensionsBuilder.build();

  // Verify that result1 was not mutated by the second build
  EXPECT_EQ(2, result1.dimensionNameToId.size());
  EXPECT_EQ(memoryId, result1.dimensionNameToId.at("memory"));
  EXPECT_EQ(capacityId, result1.dimensionNameToId.at("capacity"));
  EXPECT_EQ(1, result1.dimensionIdToObjectDimension.size());
  EXPECT_EQ(1, result1.scopeIdToDimensionIdToDimension.size());
  EXPECT_EQ(1, result1.scopeIdToDimensionIdToDimension.at(scopeId).size());

  EXPECT_EQ(4, result2.dimensionNameToId.size());
  EXPECT_EQ(cpuId, result2.dimensionNameToId.at("cpu"));
  EXPECT_EQ(bandwidthId, result2.dimensionNameToId.at("bandwidth"));
  EXPECT_EQ(2, result2.dimensionIdToObjectDimension.size());
  EXPECT_EQ(1, result2.scopeIdToDimensionIdToDimension.size());
  EXPECT_EQ(2, result2.scopeIdToDimensionIdToDimension.at(scopeId).size());
}

} // namespace facebook::rebalancer::entities::tests
