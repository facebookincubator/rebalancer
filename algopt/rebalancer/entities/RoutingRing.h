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

#pragma once

#include "algopt/rebalancer/entities/Identifiers.h"

namespace facebook {
namespace rebalancer {
namespace entities {

class RoutingRing {
 public:
  explicit RoutingRing(
      entities::ScopeItemId originScopeItem,
      double originTraffic,
      std::optional<std::vector<std::vector<entities::ScopeItemId>>>
          destinationScopeItemSets = std::nullopt);

  explicit RoutingRing(const thrift::RoutingRing& routingRing);

  const std::optional<std::vector<std::vector<entities::ScopeItemId>>>&
  getDestinationScopeItemSets() const;

  const entities::ScopeItemId getOriginScopeItem() const;

  double getOriginTraffic() const;

  thrift::RoutingRing toThrift() const;

 private:
  entities::ScopeItemId originScopeItem_;
  double originTraffic_;
  std::optional<std::vector<std::vector<entities::ScopeItemId>>>
      destinationScopeItemSets_;
};
} // namespace entities
} // namespace rebalancer
} // namespace facebook
