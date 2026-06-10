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

#include "algopt/rebalancer/solver/expressions/GroupRoutingRing.h"

#include "algopt/rebalancer/solver/expressions/BottomToTopEvaluator.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"

#include <cmath>

namespace {
constexpr std::string_view type = "GroupRoutingRing";
static constexpr auto DUMMY_VALUE = 0.0;
} // namespace

namespace facebook::rebalancer {
GroupRoutingRing::GroupRoutingRing(
    entities::RoutingConfigId routingConfigId,
    entities::GroupId groupId,
    std::shared_ptr<const entities::Universe> universe,
    const Assignment& initialAssignment)
    : Expression(std::move(universe)),
      routingConfigId_(routingConfigId),
      groupId_(groupId),
      universe_(getUniversePtr()),
      routingConfigPtr_(&universe_->getRoutingConfig(routingConfigId_)) {
  auto localDirectlyAffectedContainersPtr =
      std::make_shared<PackerSet<entities::ContainerId>>();
  totalOriginTraffic_.emplace();
  for (auto& routingRing : routingConfigPtr_->getRoutingRings(groupId_)) {
    totalOriginTraffic_.value() += routingRing.getOriginTraffic();
    auto& destinationScopeItemSets =
        routingRing.getDestinationScopeItemSets().has_value()
        ? routingRing.getDestinationScopeItemSets().value()
        : routingConfigPtr_->getDefaultDestinationScopeItemSetsFromOrigin(
              routingRing.getOriginScopeItem());

    // Moving objects in or out of any container related to routing ring of the
    // group may change the value of this expression. So collect those and add
    // to directly_affected_containers
    updateDirectlyAffectedContainers(
        destinationScopeItemSets, *localDirectlyAffectedContainersPtr);
  }
  directlyAffectedContainers.set(localDirectlyAffectedContainersPtr);

  // collect all objects in the group.
  auto partitionId = routingConfigPtr_->getPartitionId();
  auto& partition = universe_->getPartition(partitionId);
  auto& objectIds = partition.getObjectIds(groupId_);
  // TODO: remove this unnecessary conversion once ObjectId is used throughout,
  // and just make Universe return a shared_ptr to objects
  groupObjectIdsPtr_ = std::make_shared<entities::Set<entities::ObjectId>>();
  groupObjectIdsPtr_->insert(objectIds.begin(), objectIds.end());

  computeMinAndMaxLatencyValues();

  setInitialValue(applyAssignment(initialAssignment));
}

const std::string_view& GroupRoutingRing::getType() const {
  return type;
}

void GroupRoutingRing::updateDirectlyAffectedContainers(
    const std::vector<std::vector<entities::ScopeItemId>>&
        destinationScopeItemSets,
    PackerSet<entities::ContainerId>& localDirectlyAffectedContainers) {
  auto insertToContainerIdsSet = [&](const auto& containerIds,
                                     auto scopeItemId) {
    for (auto& containerId : containerIds) {
      localDirectlyAffectedContainers.insert(containerId);
      auto [_, inserted] =
          containerIdToScopeItemId_.emplace(containerId, scopeItemId);
      if (!inserted &&
          scopeItemId != containerIdToScopeItemId_.at(containerId)) {
        throw std::runtime_error(
            fmt::format(
                "Container {} belongs to two scopeItems, {} and {}. This is not supported in GroupRoutingRing.",
                universe_->getEntityName(containerId),
                universe_->getEntityName(
                    containerIdToScopeItemId_.at(containerId)),
                universe_->getEntityName(scopeItemId)));
      }
    }
  };

  auto scopeId = routingConfigPtr_->getScopeId();
  auto& scope = universe_->getScope(scopeId);
  for (auto& scopeItemSet : destinationScopeItemSets) {
    for (auto& scopeItemId : scopeItemSet) {
      auto& containerIds = scope.getContainerIds(scopeItemId);

      insertToContainerIdsSet(containerIds, scopeItemId);
    }
  }
}

std::optional<AffectedByChange> GroupRoutingRing::isAffectedByChange(
    const AffectedByChangeDecisionData& data) const {
  return AffectedByChange(
      directlyAffectedContainers.getSetPtr(), groupObjectIdsPtr_, data);
}

void GroupRoutingRing::updateEquivalenceSets(
    EquivalenceSets& equivalenceSets) const {
  equivalenceSets.combine(*groupObjectIdsPtr_);
}

const entities::ScopeItemId* FOLLY_NULLABLE
GroupRoutingRing::checkIfRelevantAndReturnScopeItemPtr(
    const Change& change) const {
  if (!groupObjectIdsPtr_->contains(change.getObject())) {
    // This change is not relevant to the group.
    return nullptr;
  }

  return folly::get_ptr(containerIdToScopeItemId_, change.getContainer());
}

double GroupRoutingRing::evaluate(
    const BottomToTopEvaluator& evaluator,
    const ChangeSet& changes) const {
  entities::Map<entities::ScopeItemId, int> scopeItemToObjectDelta;
  for (auto& change : changes) {
    auto scopeItemIdPtr = checkIfRelevantAndReturnScopeItemPtr(change);
    if (scopeItemIdPtr == nullptr) {
      // This change is not relevant w.r.t. the routing rings for this group.
      continue;
    }
    // Add/subtract the object from the relevant destination scope item.
    scopeItemToObjectDelta[*scopeItemIdPtr] += change.getValue();
  }

  if (scopeItemToObjectDelta.empty()) {
    // The changes are not relevant
    return DUMMY_VALUE;
  }

  // Create an updated temporary copy of scopeItemToObjectCount_.
  auto scopeItemToObjectCount = scopeItemToObjectCount_;
  for (auto& [scopeItemId, delta] : scopeItemToObjectDelta) {
    scopeItemToObjectCount[scopeItemId] += delta;
  }

  // add the temp traffic table to context and also update context to note that
  // the expression has been evaluated
  evaluator.getContext().groupToTempTrafficTable().save(
      this->getId(), createTrafficTableWithStats(scopeItemToObjectCount));

  return DUMMY_VALUE;
}

double GroupRoutingRing::applyAssignment(const Assignment& assignment) {
  scopeItemToObjectCount_.clear();
  for (auto objectId : *groupObjectIdsPtr_) {
    auto containerId = assignment.getContainer(objectId);
    auto scopeItemIdPtr =
        folly::get_ptr(containerIdToScopeItemId_, containerId);
    if (scopeItemIdPtr != nullptr) {
      ++scopeItemToObjectCount_[*scopeItemIdPtr];
    }
  }
  trafficTableWithStats_ = createTrafficTableWithStats(scopeItemToObjectCount_);
  return DUMMY_VALUE;
}

double GroupRoutingRing::innerFullApply(
    const TopToBottomEvaluator& /*unused*/,
    const Assignment& assignment) {
  return applyAssignment(assignment);
}

double GroupRoutingRing::innerPartialApply(
    const BottomToTopEvaluator& /*unused*/,
    const Assignment& /*unused*/,
    const ChangeSet& changes) {
  for (auto& change : changes) {
    auto scopeItemIdPtr = checkIfRelevantAndReturnScopeItemPtr(change);
    if (scopeItemIdPtr == nullptr) {
      // This change is not relevant w.r.t. the routing rings for this group.
      continue;
    }
    // Add/subtract the object from the relevant destination scope item.
    scopeItemToObjectCount_[*scopeItemIdPtr] += change.getValue();
  }

  trafficTableWithStats_ = createTrafficTableWithStats(scopeItemToObjectCount_);

  return DUMMY_VALUE;
}

Bounds GroupRoutingRing::innerLowerAndUpperBounds(
    Context& /* context */,
    const BoundConstraints& /* bc */) const {
  throw std::runtime_error(
      "innerLowerAndUpperBounds() should not be called for GroupRoutingRing");
}

bool GroupRoutingRing::shouldComputeBounds() const {
  return false;
}

entities::Map<Destination, TrafficLatencyPair>
GroupRoutingRing::createTrafficTableForOrigin(
    Origin originScopeItemId,
    double originTraffic,
    const std::vector<std::vector<entities::ScopeItemId>>&
        destinationScopeItemSets,
    const entities::Map<entities::ScopeItemId, int>& scopeItemToObjectCount)
    const {
  entities::Map<Destination, TrafficLatencyPair>
      destinationToTrafficLatencyPair;
  for (auto& scopeItemSet : destinationScopeItemSets) {
    for (auto& destinationScopeItemId : scopeItemSet) {
      const int count =
          folly::get_default(scopeItemToObjectCount, destinationScopeItemId, 0);
      if (count > 0) {
        // these destinations will be updated with their exact traffic share
        // below
        destinationToTrafficLatencyPair.try_emplace(destinationScopeItemId);
      }
    }

    if (destinationToTrafficLatencyPair.size() == 0) {
      // This scope item set does not have any objects in the group. No
      // traffic will be sent to the current scope item set.
      continue;
    }

    // If at least one scopeItem in the current destinationScopeItemSet S
    // has an object in the group, then originTraffic is sent evenly to all
    // scope items in S that have objects that belong to the group. The traffic
    // value stored is the fraction of total traffic (from all origins) that is
    // received by this destinationScopeItemId from current originScopeItemId
    for (auto& [destinationScopeItemId, _] : destinationToTrafficLatencyPair) {
      auto traffic = originTraffic /
          (totalOriginTraffic_.value() *
           destinationToTrafficLatencyPair.size());
      destinationToTrafficLatencyPair[destinationScopeItemId] = std::make_pair(
          traffic, getLatencyValue(originScopeItemId, destinationScopeItemId));
    }

    // Traffic is only sent (according to the logic above) to the first
    // destinationScopeItemSet that has at least one scopeItem with an
    // object that belongs to the group.
    break;
  }

  return destinationToTrafficLatencyPair;
}

TrafficTableWithStats<Origin, Destination>
GroupRoutingRing::createTrafficTableWithStats(
    const entities::Map<entities::ScopeItemId, int>& scopeItemToObjectCount)
    const {
  TrafficTableWithStats<Origin, Destination> trafficTableWithStats;
  auto& routingRings = routingConfigPtr_->getRoutingRings(groupId_);
  for (auto& routingRing : routingRings) {
    auto& originScopeItem = routingRing.getOriginScopeItem();
    auto originTraffic = routingRing.getOriginTraffic();

    // use the routingLogic for originScopeItemId and update the traffic
    // table
    auto& destinationScopeItemSets =
        routingRing.getDestinationScopeItemSets().has_value()
        ? routingRing.getDestinationScopeItemSets().value()
        : routingConfigPtr_->getDefaultDestinationScopeItemSetsFromOrigin(
              routingRing.getOriginScopeItem());

    trafficTableWithStats.appendToTrafficTable(
        originScopeItem,
        createTrafficTableForOrigin(
            originScopeItem,
            originTraffic,
            destinationScopeItemSets,
            scopeItemToObjectCount));
  }

  return trafficTableWithStats;
}

const TrafficTableWithStats<Origin, Destination>&
GroupRoutingRing::getTrafficTableWithStats() const {
  return trafficTableWithStats_;
}

double GroupRoutingRing::getLatencyValue(Origin origin, Destination destination)
    const {
  return routingConfigPtr_->getLatency(origin, destination);
}

double GroupRoutingRing::getMinPossibleLatencyValue() const {
  return minLatencyValue_;
}

double GroupRoutingRing::getMaxPossibleLatencyValue() const {
  return maxLatencyValue_;
}

void GroupRoutingRing::computeMinAndMaxLatencyValues() {
  auto originToDestinationLatencyPtr = routingConfigPtr_->getLatencyTablePtr();
  for (auto& [origin, destinationMap] : *originToDestinationLatencyPtr) {
    for (auto& [destination, latency] : destinationMap) {
      minLatencyValue_ = std::min(latency, minLatencyValue_);
      maxLatencyValue_ = std::max(latency, maxLatencyValue_);
    }
  }
}

const entities::Set<entities::ObjectId>& GroupRoutingRing::getObjectsInGroup()
    const {
  return *groupObjectIdsPtr_;
}

std::shared_ptr<const entities::Set<entities::ObjectId>>
GroupRoutingRing::getObjectsInGroupPtr() const {
  return groupObjectIdsPtr_;
}

double GroupRoutingRing::getTotalOriginTraffic() const {
  if (!totalOriginTraffic_.has_value()) {
    throw std::runtime_error(
        "Unexpected attempt to get total origin traffic when it is not initialized. It is expected that totalOriginTraffic_ is initialized in the constructor.");
  }
  return *totalOriginTraffic_;
}

} // namespace facebook::rebalancer
