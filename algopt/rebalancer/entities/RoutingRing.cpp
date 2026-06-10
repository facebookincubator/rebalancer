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

namespace facebook {
namespace rebalancer {
namespace entities {

RoutingRing::RoutingRing(
    entities::ScopeItemId originScopeItem,
    double originTraffic,
    std::optional<std::vector<std::vector<entities::ScopeItemId>>>
        destinationScopeItemSets)
    : originScopeItem_(originScopeItem),
      originTraffic_(originTraffic),
      destinationScopeItemSets_(std::move(destinationScopeItemSets)) {}

RoutingRing::RoutingRing(const thrift::RoutingRing& routingRing)
    : originScopeItem_(ScopeItemId(*routingRing.originScopeItem())),
      originTraffic_(*routingRing.originTraffic()) {
  if (!routingRing.destinationScopeItemSets().has_value()) {
    return;
  }
  std::vector<std::vector<entities::ScopeItemId>> destinationScopeItemSets;
  for (auto& destinationScopeItemSet :
       routingRing.destinationScopeItemSets().value()) {
    std::vector<entities::ScopeItemId> destinationScopeItemSetIds;
    destinationScopeItemSetIds.reserve(destinationScopeItemSet.size());
    for (auto& destinationScopeItem : destinationScopeItemSet) {
      destinationScopeItemSetIds.emplace_back(destinationScopeItem);
    }
    destinationScopeItemSets.emplace_back(
        std::move(destinationScopeItemSetIds));
  }

  destinationScopeItemSets_ = std::move(destinationScopeItemSets);
}

const std::optional<std::vector<std::vector<entities::ScopeItemId>>>&
RoutingRing::getDestinationScopeItemSets() const {
  return destinationScopeItemSets_;
}

const entities::ScopeItemId RoutingRing::getOriginScopeItem() const {
  return originScopeItem_;
}

double RoutingRing::getOriginTraffic() const {
  return originTraffic_;
}

thrift::RoutingRing RoutingRing::toThrift() const {
  thrift::RoutingRing routingRing;
  routingRing.originScopeItem() = originScopeItem_.asInt();
  routingRing.originTraffic() = originTraffic_;
  if (destinationScopeItemSets_.has_value()) {
    routingRing.destinationScopeItemSets() = std::vector<std::vector<int>>();
    auto& thriftDestinationScopeItemSets =
        *routingRing.destinationScopeItemSets();
    thriftDestinationScopeItemSets.reserve(destinationScopeItemSets_->size());
    for (const auto& destinationScopeItemSet : *destinationScopeItemSets_) {
      thriftDestinationScopeItemSets.emplace_back(
          asIntsVec(destinationScopeItemSet));
    }
  }
  return routingRing;
}

} // namespace entities
} // namespace rebalancer
} // namespace facebook
