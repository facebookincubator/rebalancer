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

#include "algopt/rebalancer/entities/RoutingRing.h"
#include "algopt/rebalancer/interface/tests/utils.h"

#include <folly/container/irange.h>
#include <gtest/gtest.h>

namespace entities = facebook::rebalancer::entities;
namespace interface = facebook::rebalancer::interface;

TEST(RoutingRingTest, FromThrift) {
  entities::thrift::RoutingRing routingRingThrift;
  routingRingThrift.originScopeItem() = 111;
  routingRingThrift.originTraffic() = 0.5;
  routingRingThrift.destinationScopeItemSets() = {{2, 3, 4}};
  auto routingRingFromThrift = entities::RoutingRing(routingRingThrift);

  std::vector<std::vector<entities::ScopeItemId>> destinationScopeItemSets = {
      {entities::ScopeItemId(2),
       entities::ScopeItemId(3),
       entities::ScopeItemId(4)}};

  const std::optional<std::vector<std::vector<entities::ScopeItemId>>>
      optionalDestinationScopeItemSets =
          std::make_optional(destinationScopeItemSets);

  auto expectedRoutingRing = entities::RoutingRing(
      entities::ScopeItemId(111), 0.5, optionalDestinationScopeItemSets);

  EXPECT_EQ(
      expectedRoutingRing.getOriginScopeItem(),
      routingRingFromThrift.getOriginScopeItem());

  EXPECT_EQ(
      expectedRoutingRing.getOriginTraffic(),
      routingRingFromThrift.getOriginTraffic());

  EXPECT_EQ(
      1, routingRingFromThrift.getDestinationScopeItemSets().value().size());
  EXPECT_EQ(
      3,
      routingRingFromThrift.getDestinationScopeItemSets().value().at(0).size());
  auto expectedDestinationScopeItemSets = std::vector<entities::ScopeItemId>{
      entities::ScopeItemId(2),
      entities::ScopeItemId(3),
      entities::ScopeItemId(4)};
  for (const auto i : folly::irange(3)) {
    EXPECT_EQ(
        expectedDestinationScopeItemSets.at(i),
        routingRingFromThrift.getDestinationScopeItemSets().value().at(0).at(
            i));
  }
}

TEST(RoutingRingTest, ToThrift) {
  entities::thrift::RoutingRing expectedRoutingRingToThrift;
  expectedRoutingRingToThrift.originScopeItem() = 111;
  expectedRoutingRingToThrift.originTraffic() = 0.5;
  expectedRoutingRingToThrift.destinationScopeItemSets() = {{2, 3, 4}};

  std::vector<std::vector<entities::ScopeItemId>> destinationScopeItemSets = {
      {entities::ScopeItemId(2),
       entities::ScopeItemId(3),
       entities::ScopeItemId(4)}};
  const std::optional<std::vector<std::vector<entities::ScopeItemId>>>
      optionalDestinationScopeItemSets =
          std::make_optional(destinationScopeItemSets);
  auto routingRingThrift =
      entities::RoutingRing(
          entities::ScopeItemId(111), 0.5, optionalDestinationScopeItemSets)
          .toThrift();

  EXPECT_EQ(
      *expectedRoutingRingToThrift.originScopeItem(),
      *routingRingThrift.originScopeItem());

  EXPECT_EQ(
      *expectedRoutingRingToThrift.originTraffic(),
      *routingRingThrift.originTraffic());

  EXPECT_EQ(1, routingRingThrift.destinationScopeItemSets().value().size());
  EXPECT_EQ(
      3, routingRingThrift.destinationScopeItemSets().value().at(0).size());
  auto expectedDestinationScopeItemSets = std::vector<int>{{2, 3, 4}};
  for (const auto i : folly::irange(3)) {
    EXPECT_EQ(
        expectedDestinationScopeItemSets.at(i),
        routingRingThrift.destinationScopeItemSets().value().at(0).at(i));
  }
}

TEST(RoutingRingTest, OptionaldestinationScopeItemSets) {
  // from thrift
  {
    entities::thrift::RoutingRing routingRingThrift;
    routingRingThrift.originScopeItem() = 111;
    routingRingThrift.originTraffic() = 0.5;

    auto routingRingFromThrift = entities::RoutingRing(routingRingThrift);

    auto expectedRoutingRing =
        entities::RoutingRing(entities::ScopeItemId(111), 0.5);

    EXPECT_EQ(
        expectedRoutingRing.getOriginScopeItem(),
        routingRingFromThrift.getOriginScopeItem());

    EXPECT_EQ(
        expectedRoutingRing.getOriginTraffic(),
        routingRingFromThrift.getOriginTraffic());
    EXPECT_FALSE(
        routingRingFromThrift.getDestinationScopeItemSets().has_value());
  }
  // to thrift
  {
    auto routingRingThrift =
        entities::RoutingRing(entities::ScopeItemId(111), 0.5).toThrift();

    auto expectedRoutingRingThrift = entities::thrift::RoutingRing();
    expectedRoutingRingThrift.originScopeItem() = 111;
    expectedRoutingRingThrift.originTraffic() = 0.5;

    EXPECT_EQ(
        expectedRoutingRingThrift.originScopeItem(),
        *routingRingThrift.originScopeItem());
    EXPECT_EQ(
        expectedRoutingRingThrift.originTraffic(),
        *routingRingThrift.originTraffic());
    EXPECT_FALSE(routingRingThrift.destinationScopeItemSets().has_value());
  }
}
