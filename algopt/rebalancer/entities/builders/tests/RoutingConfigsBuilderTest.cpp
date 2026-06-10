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
#include "algopt/rebalancer/entities/builders/RoutingConfigsBuilder.h"
#include "algopt/rebalancer/entities/builders/tests/BuilderTestBase.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/portability/GTest.h>

namespace facebook::rebalancer::entities::tests {

namespace {

std::unique_ptr<RoutingConfig> makeRoutingConfig(
    ScopeId scopeId,
    PartitionId partitionId) {
  const Map<GroupId, std::vector<RoutingRing>> groupToRoutingRings;
  return std::make_unique<RoutingConfig>(
      groupToRoutingRings, /*latencyTable=*/nullptr, scopeId, partitionId);
}

} // namespace

class RoutingConfigsBuilderTest : public BuilderTestBase {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    RoutingConfigsBuilderTest,
    ::testing::Values(1, 3, 20));

CO_TEST_P(RoutingConfigsBuilderTest, AddRoutingConfigs) {
  RoutingConfigsBuilder routingConfigsBuilder(idBuilder_);

  const auto config1Id = routingConfigsBuilder.makeRoutingConfigId("config1");
  const auto config2Id = routingConfigsBuilder.makeRoutingConfigId("config2");
  const auto config3Id = routingConfigsBuilder.makeRoutingConfigId("config3");
  EXPECT_NE(config1Id, config2Id);
  EXPECT_NE(config2Id, config3Id);
  EXPECT_NE(config1Id, config3Id);

  const auto scopeId1 = ScopeId(1);
  const auto scopeId2 = ScopeId(2);
  const auto partitionId1 = PartitionId(10);
  const auto partitionId2 = PartitionId(20);

  co_await folly::coro::co_withExecutor(
      executor.get(),
      folly::coro::collectAll(
          routingConfigsBuilder.add(
              config1Id,
              RoutingConfigData{
                  .routingConfig = makeRoutingConfig(scopeId1, partitionId1)}),
          routingConfigsBuilder.add(
              config2Id,
              RoutingConfigData{
                  .routingConfig = makeRoutingConfig(scopeId2, partitionId1)}),
          routingConfigsBuilder.add(
              config3Id,
              RoutingConfigData{
                  .routingConfig =
                      makeRoutingConfig(scopeId1, partitionId2)})));

  const auto result = co_await routingConfigsBuilder.build();

  EXPECT_EQ(3, result.routingConfigNameToId.size());
  EXPECT_EQ(config1Id, result.routingConfigNameToId.at("config1"));
  EXPECT_EQ(config2Id, result.routingConfigNameToId.at("config2"));
  EXPECT_EQ(config3Id, result.routingConfigNameToId.at("config3"));

  EXPECT_EQ(3, result.routingConfigIdToConfig.size());
  EXPECT_TRUE(result.routingConfigIdToConfig.contains(config1Id));
  EXPECT_TRUE(result.routingConfigIdToConfig.contains(config2Id));
  EXPECT_TRUE(result.routingConfigIdToConfig.contains(config3Id));

  EXPECT_EQ(
      scopeId1, result.routingConfigIdToConfig.at(config1Id)->getScopeId());
  EXPECT_EQ(
      scopeId2, result.routingConfigIdToConfig.at(config2Id)->getScopeId());
  EXPECT_EQ(
      scopeId1, result.routingConfigIdToConfig.at(config3Id)->getScopeId());

  EXPECT_EQ(
      partitionId1,
      result.routingConfigIdToConfig.at(config1Id)->getPartitionId());
  EXPECT_EQ(
      partitionId1,
      result.routingConfigIdToConfig.at(config2Id)->getPartitionId());
  EXPECT_EQ(
      partitionId2,
      result.routingConfigIdToConfig.at(config3Id)->getPartitionId());
}

CO_TEST_P(RoutingConfigsBuilderTest, ConcurrentAdd) {
  RoutingConfigsBuilder routingConfigsBuilder(idBuilder_);

  constexpr int nConfigs = 500;
  std::vector<RoutingConfigId> configIds;
  configIds.reserve(nConfigs);
  for (const auto i : folly::irange(nConfigs)) {
    configIds.push_back(
        routingConfigsBuilder.makeRoutingConfigId(fmt::format("config_{}", i)));
  }

  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve(nConfigs);
  for (const auto i : folly::irange(nConfigs)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&routingConfigsBuilder,
             configId = configIds[i],
             i]() -> folly::coro::Task<void> {
              executeRandomDelay();
              const auto scopeId = ScopeId(i % 10);
              const auto partitionId = PartitionId(i % 5);
              co_await routingConfigsBuilder.add(
                  configId,
                  RoutingConfigData{
                      .routingConfig =
                          makeRoutingConfig(scopeId, partitionId)});
              executeRandomDelay();
              EXPECT_EQ(
                  configId,
                  routingConfigsBuilder.getRoutingConfigId(
                      fmt::format("config_{}", i)));
              const auto data = co_await routingConfigsBuilder.get(configId);
              EXPECT_EQ(scopeId, data->routingConfig->getScopeId());
            }));
  }

  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  const auto result = co_await routingConfigsBuilder.build();

  EXPECT_EQ(nConfigs, result.routingConfigNameToId.size());
  EXPECT_EQ(nConfigs, result.routingConfigIdToConfig.size());

  for (const auto i : folly::irange(nConfigs)) {
    const auto configName = fmt::format("config_{}", i);
    EXPECT_TRUE(result.routingConfigNameToId.contains(configName));
    const auto configId = result.routingConfigNameToId.at(configName);
    EXPECT_TRUE(result.routingConfigIdToConfig.contains(configId));

    const auto expectedScopeId = ScopeId(i % 10);
    const auto expectedPartitionId = PartitionId(i % 5);
    EXPECT_EQ(
        expectedScopeId,
        result.routingConfigIdToConfig.at(configId)->getScopeId());
    EXPECT_EQ(
        expectedPartitionId,
        result.routingConfigIdToConfig.at(configId)->getPartitionId());
  }
}

TEST_P(RoutingConfigsBuilderTest, DuplicateMakeRoutingConfigIdThrows) {
  RoutingConfigsBuilder routingConfigsBuilder(idBuilder_);
  std::ignore = routingConfigsBuilder.makeRoutingConfigId("config1");
  REBALANCER_EXPECT_RUNTIME_ERROR(
      routingConfigsBuilder.makeRoutingConfigId("config1"),
      "Unexpected call to makeRoutingConfigId with a previously added name 'config1'");
}

CO_TEST_P(RoutingConfigsBuilderTest, Summarize) {
  RoutingConfigsBuilder builder(idBuilder_);
  builder.makeRoutingConfigId("routing_config_1");
  builder.makeRoutingConfigId("routing_config_2");

  const auto result = co_await builder.summarize();

  const std::string expected =
      "RoutingConfigs:\n"
      "  routing_config_1\n"
      "  routing_config_2\n";
  EXPECT_EQ(expected, result);
}

CO_TEST_P(RoutingConfigsBuilderTest, BuildAddRoutingConfigAndRebuild) {
  RoutingConfigsBuilder routingConfigsBuilder(idBuilder_);

  const auto scopeId1 = ScopeId(1);
  const auto scopeId2 = ScopeId(2);
  const auto partitionId1 = PartitionId(10);
  const auto partitionId2 = PartitionId(20);

  const auto config1Id = routingConfigsBuilder.makeRoutingConfigId("config1");
  co_await routingConfigsBuilder.add(
      config1Id,
      RoutingConfigData{
          .routingConfig = makeRoutingConfig(scopeId1, partitionId1)});

  const auto result1 = co_await routingConfigsBuilder.build();
  EXPECT_EQ(1, result1.routingConfigNameToId.size());
  EXPECT_EQ(config1Id, result1.routingConfigNameToId.at("config1"));
  EXPECT_EQ(1, result1.routingConfigIdToConfig.size());
  EXPECT_EQ(
      scopeId1, result1.routingConfigIdToConfig.at(config1Id)->getScopeId());
  EXPECT_EQ(
      partitionId1,
      result1.routingConfigIdToConfig.at(config1Id)->getPartitionId());

  const auto config2Id = routingConfigsBuilder.makeRoutingConfigId("config2");
  co_await routingConfigsBuilder.add(
      config2Id,
      RoutingConfigData{
          .routingConfig = makeRoutingConfig(scopeId2, partitionId2)});

  const auto result2 = co_await routingConfigsBuilder.build();

  // Verify that result1 was not mutated by the second build
  EXPECT_EQ(1, result1.routingConfigNameToId.size());
  EXPECT_EQ(config1Id, result1.routingConfigNameToId.at("config1"));
  EXPECT_EQ(1, result1.routingConfigIdToConfig.size());
  EXPECT_EQ(
      scopeId1, result1.routingConfigIdToConfig.at(config1Id)->getScopeId());
  EXPECT_EQ(
      partitionId1,
      result1.routingConfigIdToConfig.at(config1Id)->getPartitionId());

  EXPECT_EQ(2, result2.routingConfigNameToId.size());
  EXPECT_EQ(config1Id, result2.routingConfigNameToId.at("config1"));
  EXPECT_EQ(config2Id, result2.routingConfigNameToId.at("config2"));
  EXPECT_EQ(2, result2.routingConfigIdToConfig.size());
  EXPECT_EQ(
      scopeId1, result2.routingConfigIdToConfig.at(config1Id)->getScopeId());
  EXPECT_EQ(
      partitionId1,
      result2.routingConfigIdToConfig.at(config1Id)->getPartitionId());
  EXPECT_EQ(
      scopeId2, result2.routingConfigIdToConfig.at(config2Id)->getScopeId());
  EXPECT_EQ(
      partitionId2,
      result2.routingConfigIdToConfig.at(config2Id)->getPartitionId());
}

} // namespace facebook::rebalancer::entities::tests
