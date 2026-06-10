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
#include "algopt/rebalancer/entities/Set.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/utils/TrafficTable.h"

namespace facebook::rebalancer {

using Origin = entities::ScopeItemId;
using Destination = entities::ScopeItemId;

class GroupRoutingRing : public Expression {
 public:
  GroupRoutingRing(
      entities::RoutingConfigId routingConfigId,
      entities::GroupId groupId,
      std::shared_ptr<const entities::Universe> universe,
      const Assignment& initialAssignment);

  const std::string_view& getType() const override;

  virtual std::optional<AffectedByChange> isAffectedByChange(
      const AffectedByChangeDecisionData& data) const override;

  // Evaluate function that evaluates changes and adds a temp multi-valued
  // attribute (i.e., temp traffic table) to Context if required. This table
  // will then be used by the parent for its update. Note that this function
  // always returns a DUMMY_VALUE (= 0.0)
  double evaluate(
      const BottomToTopEvaluator& evaluator,
      const ChangeSet& changes) const override;

  const TrafficTableWithStats<Origin, Destination>& getTrafficTableWithStats()
      const;

  double getLatencyValue(
      Origin originScopeItemId,
      Destination destinationScopeItemId) const;

  double getMinPossibleLatencyValue() const;
  double getMaxPossibleLatencyValue() const;

  const entities::Set<entities::ObjectId>& getObjectsInGroup() const;

  std::shared_ptr<const entities::Set<entities::ObjectId>>
  getObjectsInGroupPtr() const;

  double getTotalOriginTraffic() const;

  void updateEquivalenceSets(EquivalenceSets& equivalenceSets) const override;

  bool shouldComputeBounds() const override;

 private:
  // Apply with an assignment and update the multi-valued attributes (i.e.,
  // traffic table) of this expression. Note that this function always returns a
  // DUMMY_VALUE (= 0.0)
  double innerFullApply(
      const TopToBottomEvaluator& evaluator,
      const Assignment& assignment) override;

  // Apply with changes and update the multi-valued attributes (i.e.,
  // traffic table) of this expression. Note that this function always returns a
  // DUMMY_VALUE (= 0.0)
  double innerPartialApply(
      const BottomToTopEvaluator& evaluator,
      const Assignment& assignment,
      const ChangeSet& changes) override;

  // Computes the amount of traffic received by each (origin, destination)
  // pair and the corresponding latency value. The traffic value for each pair
  // is the fraction of the total traffic (total is computed including all
  // origins) that is received by the destination
  TrafficTableWithStats<Origin, Destination> createTrafficTableWithStats(
      const entities::Map<entities::ScopeItemId, int>& scopeItemToObjectCount)
      const;

  entities::Map<Destination, TrafficLatencyPair> createTrafficTableForOrigin(
      Origin originScopeItemId,
      double originTraffic,
      const std::vector<std::vector<entities::ScopeItemId>>&
          destinationScopeItemSets,
      const entities::Map<entities::ScopeItemId, int>& scopeItemToObjectCount)
      const;

  void computeMinAndMaxLatencyValues();

  const entities::ScopeItemId* checkIfRelevantAndReturnScopeItemPtr(
      const Change& change) const;

  void updateDirectlyAffectedContainers(
      const std::vector<std::vector<entities::ScopeItemId>>&
          destinationScopeItemSets,
      PackerSet<entities::ContainerId>& localDirectlyAffectedContainers);

  double applyAssignment(const Assignment& assignment);

  Bounds innerLowerAndUpperBounds(Context& context, const BoundConstraints& bc)
      const override;

 private:
  entities::RoutingConfigId routingConfigId_;
  entities::GroupId groupId_;
  std::shared_ptr<const entities::Universe> universe_;
  const entities::RoutingConfig* routingConfigPtr_;
  std::optional<double> totalOriginTraffic_ = std::nullopt;

  // traffic table which stores for (origin, destination) pair, the fraction
  // of totalTraffic that is destination receives from origin, and also some
  // stats like weighted average and max latency
  TrafficTableWithStats<Origin, Destination> trafficTableWithStats_;

  entities::Map<entities::ContainerId, entities::ScopeItemId>
      containerIdToScopeItemId_;
  std::shared_ptr<entities::Set<entities::ObjectId>> groupObjectIdsPtr_;
  entities::Map<entities::ScopeItemId, int> scopeItemToObjectCount_;

  double maxLatencyValue_ = std::numeric_limits<double>::lowest();
  double minLatencyValue_ = std::numeric_limits<double>::infinity();
};

} // namespace facebook::rebalancer
