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
#include "algopt/rebalancer/entities/RoutingConfig.h"

#include <folly/container/irange.h>
#include <gtest/gtest.h>

namespace entities = facebook::rebalancer::entities;
namespace interface = facebook::rebalancer::interface;

using SortedDestinationLatencyMap = facebook::algopt::ValueSortedMap<
    entities::ScopeItemId,
    double,
    entities::CompareScopeItemLatencyPair>;

TEST(RoutingConfigTest, FromThrift) {
  auto routingRingThrift = entities::thrift::RoutingRing();
  routingRingThrift.originScopeItem() = 1;
  routingRingThrift.originTraffic() = 0.5;
  routingRingThrift.destinationScopeItemSets() = {{2, 3, 4}, {5}};

  auto groupRoutingRingsThrift = entities::thrift::GroupRoutingRings();
  groupRoutingRingsThrift.routingRings() =
      std::vector<entities::thrift::RoutingRing>{routingRingThrift};
  entities::thrift::RoutingConfig routingConfigThrift;

  routingConfigThrift.groupToRoutingRingsEntities() =
      folly::F14FastMap<int, entities::thrift::GroupRoutingRings>{
          {22, groupRoutingRingsThrift}};
  routingConfigThrift.latencyTable() =
      folly::F14FastMap<int, folly::F14FastMap<int, double>>{
          {3, {{11, 0.8}, {12, 0.1}, {13, 0.5}}}};
  routingConfigThrift.scopeId() = 111;
  routingConfigThrift.partitionId() = 222;

  auto destinationScopeItemSets =
      std::vector<std::vector<entities::ScopeItemId>>{
          {entities::ScopeItemId(2),
           entities::ScopeItemId(3),
           entities::ScopeItemId(4)},
          {entities::ScopeItemId(5)}};
  auto optionalDestinationScopeItemSets =
      std::make_optional(destinationScopeItemSets);

  auto routingRing = entities::RoutingRing(
      entities::ScopeItemId(1), 0.5, optionalDestinationScopeItemSets);
  auto groupToRoutingRings =
      entities::Map<entities::GroupId, std::vector<entities::RoutingRing>>();
  groupToRoutingRings.emplace(
      entities::GroupId(22), std::vector<entities::RoutingRing>{routingRing});

  auto latencyTable = std::make_shared<
      entities::Map<entities::ScopeItemId, SortedDestinationLatencyMap>>();
  latencyTable->emplace(
      entities::ScopeItemId(3),
      SortedDestinationLatencyMap{
          {entities::ScopeItemId(12), 0.1},
          {entities::ScopeItemId(13), 0.5},
          {entities::ScopeItemId(11), 0.8}});

  auto defaultOriginToDestinationScopeItemSetsPtr =
      std::make_shared<entities::Map<
          entities::ScopeItemId,
          std::vector<std::vector<entities::ScopeItemId>>>>();

  auto expectedRoutingConfig = entities::RoutingConfig(
      groupToRoutingRings,
      latencyTable,
      entities::ScopeId(111),
      entities::PartitionId(222),
      defaultOriginToDestinationScopeItemSetsPtr);
  auto routingConfigFromThrift = entities::RoutingConfig(routingConfigThrift);

  EXPECT_EQ(1, routingConfigFromThrift.getGroupToRoutingRings().size());
  EXPECT_TRUE(routingConfigFromThrift.getGroupToRoutingRings().contains(
      entities::GroupId(22)));
  auto groupRoutingRingsFromThrift =
      routingConfigFromThrift.getGroupToRoutingRings().at(
          entities::GroupId(22));
  EXPECT_EQ(1, groupRoutingRingsFromThrift.size());
  const auto& routingRingFromThrift = groupRoutingRingsFromThrift.at(0);

  EXPECT_EQ(
      routingRingThrift.originScopeItem(),
      routingRingFromThrift.getOriginScopeItem().asInt());
  EXPECT_EQ(
      routingRingThrift.originTraffic(),
      routingRingFromThrift.getOriginTraffic());
  EXPECT_EQ(
      2, routingRingFromThrift.getDestinationScopeItemSets().value().size());

  for (const auto i :
       folly::irange(routingRingThrift.destinationScopeItemSets()->size())) {
    std::vector<int> destinations =
        routingRingThrift.destinationScopeItemSets()->at(i);
    EXPECT_EQ(
        destinations.size(),
        routingRingFromThrift.getDestinationScopeItemSets()
            .value()
            .at(i)
            .size());
    for (const auto j : folly::irange(destinations.size())) {
      EXPECT_EQ(
          entities::ScopeItemId(destinations.at(j)),
          routingRingFromThrift.getDestinationScopeItemSets().value().at(i).at(
              j));
    }
  }

  EXPECT_EQ(
      *expectedRoutingConfig.getLatencyTablePtr(),
      *routingConfigFromThrift.getLatencyTablePtr());

  // check the order w.r.t each source scopeItem
  EXPECT_EQ(1, routingConfigFromThrift.getLatencyTablePtr()->size());
  auto& destLatencies =
      routingConfigFromThrift.getLatencyTablePtr()->begin()->second;
  EXPECT_EQ(3, destLatencies.size());
  auto expectedOrderOfDestinations = std::vector<entities::ScopeItemId>{
      entities::ScopeItemId(12),
      entities::ScopeItemId(13),
      entities::ScopeItemId(11)};
  int i = 0;
  for (auto& [destination, _] : destLatencies) {
    EXPECT_EQ(expectedOrderOfDestinations.at(i), destination);
    ++i;
  }
}

TEST(RoutingConfigTest, ToThrift) {
  auto groupToRoutingRings =
      entities::Map<entities::GroupId, std::vector<entities::RoutingRing>>();

  auto routingRings = std::vector<entities::RoutingRing>();
  groupToRoutingRings.emplace(entities::GroupId(41), routingRings);

  auto latencyTable = std::make_shared<
      entities::Map<entities::ScopeItemId, SortedDestinationLatencyMap>>();
  latencyTable->emplace(
      entities::ScopeItemId(2),
      SortedDestinationLatencyMap{{entities::ScopeItemId(12), 0.5}});

  auto defaultOriginToDestinationScopeItemSetsPtr =
      std::make_shared<entities::Map<
          entities::ScopeItemId,
          std::vector<std::vector<entities::ScopeItemId>>>>();
  defaultOriginToDestinationScopeItemSetsPtr->emplace(
      entities::ScopeItemId(21),
      std::vector<std::vector<entities::ScopeItemId>>{
          {entities::ScopeItemId(24), entities::ScopeItemId(99)}});

  auto routingConfig = entities::RoutingConfig(
      groupToRoutingRings,
      latencyTable,
      entities::ScopeId(333),
      entities::PartitionId(112),
      defaultOriginToDestinationScopeItemSetsPtr);

  auto routingConfigThrift = routingConfig.toThrift();
  EXPECT_EQ(333, *routingConfigThrift.scopeId());
  EXPECT_EQ(112, *routingConfigThrift.partitionId());

  auto& thriftGroupToRoutingRings =
      *routingConfigThrift.groupToRoutingRingsEntities();
  auto& thriftLatencyTable = *routingConfigThrift.latencyTable();

  EXPECT_EQ(1, thriftGroupToRoutingRings.size());
  EXPECT_TRUE(thriftGroupToRoutingRings.contains(
      41)); // there is an entry for group with id 41
  EXPECT_EQ(
      entities::thrift::GroupRoutingRings(), thriftGroupToRoutingRings[41]);

  EXPECT_EQ(1, thriftLatencyTable.size());
  EXPECT_TRUE(thriftLatencyTable.contains(
      2)); // there is an entry for scope item with 2
  EXPECT_EQ(0.5, thriftLatencyTable[2][12]);
}

TEST(CompareScopeItemLatencyPairTest, Basic) {
  const entities::CompareScopeItemLatencyPair compare;

  EXPECT_TRUE(compare(
      {entities::ScopeItemId(12), 0.1}, {entities::ScopeItemId(13), 0.5}));

  EXPECT_FALSE(compare(
      {entities::ScopeItemId(13), 0.1}, {entities::ScopeItemId(12), 0.1}));

  EXPECT_TRUE(compare(
      {entities::ScopeItemId(13), 0.01}, {entities::ScopeItemId(12), 0.1}));
}

TEST(GetMinLatencyDestinationsTest, Basic) {
  auto latencyTable = std::make_shared<
      entities::Map<entities::ScopeItemId, SortedDestinationLatencyMap>>();

  latencyTable->emplace(
      entities::ScopeItemId(2),
      SortedDestinationLatencyMap{
          {entities::ScopeItemId(2), 0.1},
          {entities::ScopeItemId(3), 0.5},
          {entities::ScopeItemId(4), 0.8}});

  latencyTable->emplace(
      entities::ScopeItemId(3),
      SortedDestinationLatencyMap{
          {entities::ScopeItemId(2), 0.1},
          {entities::ScopeItemId(3), 0.1},
          {entities::ScopeItemId(4), 0.1}});

  latencyTable->emplace(
      entities::ScopeItemId(4), SortedDestinationLatencyMap{});

  auto defaultOriginToDestinationScopeItemSetsPtr =
      std::make_shared<entities::Map<
          entities::ScopeItemId,
          std::vector<std::vector<entities::ScopeItemId>>>>();

  entities::RoutingConfig routingConfig(
      {},
      latencyTable,
      entities::ScopeId(1),
      entities::PartitionId(100),
      defaultOriginToDestinationScopeItemSetsPtr);

  EXPECT_EQ(
      (folly::small_vector<entities::ScopeItemId, 2>{entities::ScopeItemId(2)}),
      routingConfig.getMinLatencyDestinationsFor(entities::ScopeItemId(2)));

  EXPECT_EQ(
      (folly::small_vector<entities::ScopeItemId, 2>{
          entities::ScopeItemId(2),
          entities::ScopeItemId(3),
          entities::ScopeItemId(4)}),
      routingConfig.getMinLatencyDestinationsFor(entities::ScopeItemId(3)));

  REBALANCER_EXPECT_RUNTIME_ERROR(
      routingConfig.getMinLatencyDestinationsFor(entities::ScopeItemId(4)),
      "latencies for origin 4 not found");
}

TEST(RoutingConfigTest, NullDefaultDestinations) {
  {
    // from thrift
    auto routingRingThrift = entities::thrift::RoutingRing();
    routingRingThrift.originScopeItem() = 1;
    routingRingThrift.originTraffic() = 0.5;
    routingRingThrift.destinationScopeItemSets() = {{2, 3, 4}, {5}};

    auto groupRoutingRingsThrift = entities::thrift::GroupRoutingRings();
    groupRoutingRingsThrift.routingRings() =
        std::vector<entities::thrift::RoutingRing>{routingRingThrift};
    entities::thrift::RoutingConfig routingConfigThrift;

    routingConfigThrift.groupToRoutingRingsEntities() =
        folly::F14FastMap<int, entities::thrift::GroupRoutingRings>{
            {22, groupRoutingRingsThrift}};
    routingConfigThrift.latencyTable() =
        folly::F14FastMap<int, folly::F14FastMap<int, double>>{
            {3, {{11, 0.8}, {12, 0.1}, {13, 0.5}}}};
    routingConfigThrift.scopeId() = 111;
    routingConfigThrift.partitionId() = 222;

    auto routingConfigFromThrift = entities::RoutingConfig(routingConfigThrift);
    EXPECT_EQ(
        routingConfigFromThrift.getDefaultOriginToDestinationScopeItemSetsPtr(),
        nullptr);

    REBALANCER_EXPECT_RUNTIME_ERROR(
        routingConfigFromThrift.getDefaultDestinationScopeItemSetsFromOrigin(
            entities::ScopeItemId(1)),
        "default destinations not set for 1");
  }
  {
    // to thrift
    auto groupToRoutingRings =
        entities::Map<entities::GroupId, std::vector<entities::RoutingRing>>();

    auto routingRings = std::vector<entities::RoutingRing>();

    auto latencyTable = std::make_shared<
        entities::Map<entities::ScopeItemId, SortedDestinationLatencyMap>>();

    auto routingConfig = entities::RoutingConfig(
        groupToRoutingRings,
        latencyTable,
        entities::ScopeId(333),
        entities::PartitionId(112),
        nullptr);

    auto routingConfigThrift = routingConfig.toThrift();
    EXPECT_EQ(
        std::nullopt,
        routingConfigThrift.defaultOriginToDestinationScopeItemSets());
  }
}

TEST(RoutingConfigTest, GetDefaultDestinationScopeItemSetsFromOrigin) {
  auto groupToRoutingRingsPtr =
      entities::Map<entities::GroupId, std::vector<entities::RoutingRing>>();

  auto routingRings = std::vector<entities::RoutingRing>();

  auto latencyTable = std::make_shared<
      entities::Map<entities::ScopeItemId, SortedDestinationLatencyMap>>();
  auto defaultOriginToDestinationScopeItemSetsPtr =
      std::make_shared<entities::Map<
          entities::ScopeItemId,
          std::vector<std::vector<entities::ScopeItemId>>>>();
  defaultOriginToDestinationScopeItemSetsPtr->emplace(
      entities::ScopeItemId(21),
      std::vector<std::vector<entities::ScopeItemId>>{
          {entities::ScopeItemId(24), entities::ScopeItemId(99)}});

  auto routingConfig = entities::RoutingConfig(
      groupToRoutingRingsPtr,
      latencyTable,
      entities::ScopeId(333),
      entities::PartitionId(112),
      defaultOriginToDestinationScopeItemSetsPtr);

  EXPECT_EQ(
      routingConfig
          .getDefaultDestinationScopeItemSetsFromOrigin(
              entities::ScopeItemId(21))
          .size(),
      1);

  auto defaultDestinations =
      routingConfig.getDefaultDestinationScopeItemSetsFromOrigin(
          entities::ScopeItemId(21));
  EXPECT_EQ(defaultDestinations.at(0).size(), 2);
  EXPECT_EQ(defaultDestinations.at(0).at(0), entities::ScopeItemId(24));
  EXPECT_EQ(defaultDestinations.at(0).at(1), entities::ScopeItemId(99));

  REBALANCER_EXPECT_RUNTIME_ERROR(
      routingConfig.getDefaultDestinationScopeItemSetsFromOrigin(
          entities::ScopeItemId(22)),
      "default destinations for origin 22 not found");
}
