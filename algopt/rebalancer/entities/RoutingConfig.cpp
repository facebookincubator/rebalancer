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

#include "algopt/rebalancer/entities/RoutingConfig.h"

#include <stdexcept>

namespace facebook::rebalancer::entities {

RoutingConfig::RoutingConfig(
    GroupToRoutingRings groupToRoutingRings,
    std::shared_ptr<const LatencyTable> latencyTablePtr,
    entities::ScopeId scopeId,
    entities::PartitionId partitionId,
    std::shared_ptr<const OriginToDestinationScopeItemSets>
        defaultOriginToDestinationsPtr)
    : groupToRoutingRings_{std::move(groupToRoutingRings)},
      latencyTablePtr_{std::move(latencyTablePtr)},
      scopeId_{scopeId},
      partitionId_{partitionId},
      defaultOriginToDestinationScopeItemSetsPtr_(
          std::move(defaultOriginToDestinationsPtr)) {}

RoutingConfig::RoutingConfig(const thrift::RoutingConfig& routingConfig)
    : scopeId_(ScopeId(*routingConfig.scopeId())),
      partitionId_(PartitionId(*routingConfig.partitionId())) {
  auto groupToRoutingRings = GroupToRoutingRings();
  for (auto& [groupIdInt, groupRoutingRings] :
       *routingConfig.groupToRoutingRingsEntities()) {
    std::vector<entities::RoutingRing> routingRingsIds;
    for (auto& routingRing : *groupRoutingRings.routingRings()) {
      routingRingsIds.emplace_back(routingRing);
    }
    groupToRoutingRings.emplace(
        GroupId(groupIdInt), std::move(routingRingsIds));
  }
  groupToRoutingRings_ = std::move(groupToRoutingRings);

  auto latencyTablePtr = std::make_shared<LatencyTable>();
  const auto& sourceToDestinationLatencies = *routingConfig.latencyTable();
  for (auto& [source, latencies] : sourceToDestinationLatencies) {
    SortedDestinationLatencyMap destLatencies;
    for (auto& [dest, latency] : latencies) {
      destLatencies.assign(ScopeItemId(dest), latency);
    }

    latencyTablePtr->emplace(ScopeItemId(source), std::move(destLatencies));
  }

  latencyTablePtr_ = latencyTablePtr;

  defaultOriginToDestinationScopeItemSetsPtr_ = nullptr;

  if (routingConfig.defaultOriginToDestinationScopeItemSets().has_value()) {
    auto defaultOriginToDestinationScopeItemSetsPtr =
        std::make_shared<OriginToDestinationScopeItemSets>();
    for (auto& [origin, destinationScopeItemSets] :
         *routingConfig.defaultOriginToDestinationScopeItemSets()) {
      std::vector<std::vector<entities::ScopeItemId>>
          destinationScopeItemSetsVec;
      for (auto& destinationScopeItemSet : destinationScopeItemSets) {
        std::vector<entities::ScopeItemId> destinationScopeItemSetVec;
        destinationScopeItemSetVec.reserve(destinationScopeItemSet.size());
        for (auto& destinationScopeItemId : destinationScopeItemSet) {
          destinationScopeItemSetVec.emplace_back(destinationScopeItemId);
        }
        destinationScopeItemSetsVec.emplace_back(
            std::move(destinationScopeItemSetVec));
      }
      defaultOriginToDestinationScopeItemSetsPtr->emplace(
          entities::ScopeItemId(origin),
          std::move(destinationScopeItemSetsVec));
    }

    defaultOriginToDestinationScopeItemSetsPtr_ =
        std::move(defaultOriginToDestinationScopeItemSetsPtr);
  }
}

std::shared_ptr<const RoutingConfig::OriginToDestinationScopeItemSets>
RoutingConfig::getDefaultOriginToDestinationScopeItemSetsPtr() const {
  return defaultOriginToDestinationScopeItemSetsPtr_;
}

const RoutingConfig::GroupToRoutingRings RoutingConfig::getGroupToRoutingRings()
    const {
  return groupToRoutingRings_;
}

const std::vector<entities::RoutingRing>& RoutingConfig::getRoutingRings(
    entities::GroupId groupId) const {
  auto routingRingsPtr = folly::get_ptr(groupToRoutingRings_, groupId);
  if (!routingRingsPtr) {
    throw std::runtime_error(
        fmt::format("No routing rings found for group {}", groupId));
  }

  return *routingRingsPtr;
}

std::shared_ptr<const RoutingConfig::LatencyTable>
RoutingConfig::getLatencyTablePtr() const {
  return latencyTablePtr_;
}

double RoutingConfig::getLatency(
    entities::ScopeItemId origin,
    entities::ScopeItemId destination) const {
  auto destinationLatencies = folly::get_ptr(*latencyTablePtr_, origin);
  if (destinationLatencies == nullptr) {
    throw std::runtime_error(
        fmt::format("latencies for origin {} not found", origin));
  }
  auto latency = destinationLatencies->getPtrIfExists(destination);
  if (latency == nullptr) {
    throw std::runtime_error(
        fmt::format(
            "latency for origin {} and destination {} not found",
            origin,
            destination));
  }
  return *latency;
}

ScopeId RoutingConfig::getScopeId() const {
  return scopeId_;
}

PartitionId RoutingConfig::getPartitionId() const {
  return partitionId_;
}

const std::vector<std::vector<entities::ScopeItemId>>&
RoutingConfig::getDefaultDestinationScopeItemSetsFromOrigin(
    entities::ScopeItemId scopeItemId) const {
  if (defaultOriginToDestinationScopeItemSetsPtr_ == nullptr) {
    throw std::runtime_error(
        fmt::format("default destinations not set for {}", scopeItemId));
  }
  auto defaultDestinations =
      folly::get_ptr(*defaultOriginToDestinationScopeItemSetsPtr_, scopeItemId);

  if (defaultDestinations == nullptr) {
    throw std::runtime_error(
        fmt::format(
            "default destinations for origin {} not found", scopeItemId));
  }

  return *defaultDestinations;
}

folly::small_vector<entities::ScopeItemId, 2>
RoutingConfig::getMinLatencyDestinationsFor(
    entities::ScopeItemId origin) const {
  auto sortedDestinationLatencies = folly::get_ptr(*latencyTablePtr_, origin);
  if (sortedDestinationLatencies == nullptr ||
      sortedDestinationLatencies->size() == 0) {
    throw std::runtime_error(
        fmt::format("latencies for origin {} not found", origin));
  }

  folly::small_vector<entities::ScopeItemId, 2> minLatencyDestinations;
  auto it = sortedDestinationLatencies->begin();
  auto& [_, minLatency] = *it;
  while (it != sortedDestinationLatencies->end() && it->second == minLatency) {
    minLatencyDestinations.push_back(it->first);
    it++;
  }

  return minLatencyDestinations;
}

thrift::RoutingConfig RoutingConfig::toThrift() const {
  thrift::RoutingConfig routingConfig;
  auto& thriftGroupToRoutingRings =
      *routingConfig.groupToRoutingRingsEntities();
  thriftGroupToRoutingRings.reserve(groupToRoutingRings_.size());
  for (const auto& [groupId, routingRings] : groupToRoutingRings_) {
    thrift::GroupRoutingRings groupRoutingRings;
    auto& thriftRoutingRings = *groupRoutingRings.routingRings();
    thriftRoutingRings.reserve(routingRings.size());
    for (const auto& routingRing : routingRings) {
      thriftRoutingRings.push_back(routingRing.toThrift());
    }
    thriftGroupToRoutingRings.emplace(groupId.asInt(), groupRoutingRings);
  }

  auto& thriftLatencyTable = *routingConfig.latencyTable();
  thriftLatencyTable.reserve(latencyTablePtr_->size());
  for (const auto& [source, destLatencies] : *latencyTablePtr_) {
    thriftLatencyTable.emplace(
        source.asInt(),
        asIntsMap<folly::F14FastMap<int, double>>(destLatencies));
  }

  routingConfig.scopeId() = scopeId_.asInt();
  routingConfig.partitionId() = partitionId_.asInt();

  if (defaultOriginToDestinationScopeItemSetsPtr_ != nullptr) {
    folly::F14FastMap<int, std::vector<std::vector<int>>>
        thriftDestinationScopeItemSets;
    thriftDestinationScopeItemSets.reserve(
        defaultOriginToDestinationScopeItemSetsPtr_->size());
    for (const auto& [origin, destinationScopeItemSets] :
         *defaultOriginToDestinationScopeItemSetsPtr_) {
      std::vector<std::vector<int>> destinationScopeItemSetsVec;
      destinationScopeItemSetsVec.reserve(destinationScopeItemSets.size());
      for (auto& destinationScopeItemSet : destinationScopeItemSets) {
        destinationScopeItemSetsVec.emplace_back(
            entities::asIntsVec(destinationScopeItemSet));
      }
      thriftDestinationScopeItemSets.emplace(
          origin.asInt(), std::move(destinationScopeItemSetsVec));
    }
    routingConfig.defaultOriginToDestinationScopeItemSets() =
        std::move(thriftDestinationScopeItemSets);
  }

  return routingConfig;
}

} // namespace facebook::rebalancer::entities
