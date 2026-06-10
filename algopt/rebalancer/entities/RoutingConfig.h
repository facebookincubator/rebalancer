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

#include "algopt/rebalancer/algopt_common/ValueSortedMap.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/RoutingRing.h"

#include "folly/small_vector.h"

namespace facebook::rebalancer::entities {

class CompareScopeItemLatencyPair {
 public:
  bool operator()(
      const std::pair<entities::ScopeItemId, double>& p1,
      const std::pair<entities::ScopeItemId, double>& p2) const {
    const auto& [id1, latency1] = p1;
    const auto& [id2, latency2] = p2;

    if (latency1 == latency2) {
      return id1 < id2;
    }

    return latency1 < latency2;
  }
};

class RoutingConfig {
  using SortedDestinationLatencyMap = algopt::ValueSortedMap<
      entities::ScopeItemId,
      double,
      CompareScopeItemLatencyPair>;

  using LatencyTable =
      entities::Map<entities::ScopeItemId, SortedDestinationLatencyMap>;

  using GroupRoutingRings = std::vector<entities::RoutingRing>;

  using GroupToRoutingRings =
      entities::Map<entities::GroupId, GroupRoutingRings>;

  using OriginToDestinationScopeItemSets = entities::Map<
      entities::ScopeItemId,
      std::vector<std::vector<entities::ScopeItemId>>>;

 public:
  explicit RoutingConfig(
      GroupToRoutingRings groupToRoutingRings,
      const std::shared_ptr<const LatencyTable> latencyTable,
      entities::ScopeId scopeId,
      entities::PartitionId partitionId,
      std::shared_ptr<const OriginToDestinationScopeItemSets>
          defaultOriginToDestinationScopeItemSets = nullptr);

  explicit RoutingConfig(const thrift::RoutingConfig& routingConfig);

  // Delete the copy and assignment constructors to prevent accidental copies.
  RoutingConfig(const RoutingConfig&) = delete;
  RoutingConfig& operator=(const RoutingConfig&) = delete;
  // Other operators are as usual.
  RoutingConfig(RoutingConfig&&) = default;
  RoutingConfig& operator=(RoutingConfig&&) = default;
  ~RoutingConfig() = default;

  const GroupToRoutingRings getGroupToRoutingRings() const;

  const std::vector<RoutingRing>& getRoutingRings(
      entities::GroupId groupId) const;

  std::shared_ptr<const LatencyTable> getLatencyTablePtr() const;

  std::shared_ptr<const OriginToDestinationScopeItemSets>
  getDefaultOriginToDestinationScopeItemSetsPtr() const;

  const std::vector<std::vector<entities::ScopeItemId>>&
  getDefaultDestinationScopeItemSetsFromOrigin(
      entities::ScopeItemId scopeItemId) const;

  double getLatency(
      entities::ScopeItemId origin,
      entities::ScopeItemId destination) const;

  ScopeId getScopeId() const;

  PartitionId getPartitionId() const;

  folly::small_vector<entities::ScopeItemId, 2> getMinLatencyDestinationsFor(
      entities::ScopeItemId origin) const;

  thrift::RoutingConfig toThrift() const;

 private:
  GroupToRoutingRings groupToRoutingRings_;
  std::shared_ptr<const LatencyTable> latencyTablePtr_;
  entities::ScopeId scopeId_;
  entities::PartitionId partitionId_;
  std::shared_ptr<const OriginToDestinationScopeItemSets>
      defaultOriginToDestinationScopeItemSetsPtr_;
};

} // namespace facebook::rebalancer::entities
