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

#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/GroupRoutingRing.h"

namespace facebook::rebalancer {

class GroupRoutingLatencyLookup : public Expression {
 public:
  GroupRoutingLatencyLookup(
      std::shared_ptr<GroupRoutingRing> groupRoutingRing,
      interface::RoutingLatencyMetricInfo aggregationMetric,
      std::shared_ptr<const entities::Universe> universe);

  const std::string_view& getType() const override;

  virtual std::optional<AffectedByChange> isAffectedByChange(
      const AffectedByChangeDecisionData& data) const override;

  double evaluate(
      const BottomToTopEvaluator& evaluator,
      const ChangeSet& changes) const override;

  void updateEquivalenceSets(EquivalenceSets& equivalenceSets) const override;

 private:
  double innerFullApply(
      const TopToBottomEvaluator& evaluator,
      const Assignment& assignment) override;

  double innerPartialApply(
      const BottomToTopEvaluator& evaluator,
      const Assignment& assignment,
      const ChangeSet& changes) override;

  Bounds innerLowerAndUpperBounds(Context& context, const BoundConstraints& bc)
      const override;

  bool shouldComputeDescendingChildPotentials() const override;

  double applyUsingTrafficTableFromChild();

  double aggregateLatency(
      const TrafficTableWithStats<Origin, Destination>&) const;

  void throwIfUnknownAggregationMetric() const;

  interface::RoutingLatencyMetricInfo aggregationMetric_;
};

} // namespace facebook::rebalancer
