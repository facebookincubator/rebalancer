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

#include "algopt/rebalancer/solver/expressions/GroupRoutingLatencyLookup.h"

#include "algopt/rebalancer/solver/expressions/BottomToTopEvaluator.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"

#include <stdexcept>

namespace {
constexpr std::string_view type = "GroupRoutingLatencyLookup";
}

namespace facebook::rebalancer {

GroupRoutingLatencyLookup::GroupRoutingLatencyLookup(
    std::shared_ptr<GroupRoutingRing> groupRoutingRing,
    interface::RoutingLatencyMetricInfo aggregationMetric,
    std::shared_ptr<const entities::Universe> universe)
    : Expression(std::move(universe)),
      aggregationMetric_(std::move(aggregationMetric)) {
  add_child(groupRoutingRing);

  // Moving objects in or out of any container related to routing ring of the
  // group (that is present in the child) may change the value of this
  // expression. Also, note that although this expression has a child, we still
  // need to set directly_affected_containers for this expression type, since
  // the child expression is multi-valued, and so the Orchestrator cannot
  // directly know if there is an update to the child.

  directlyAffectedContainers =
      groupRoutingRing->getDirectlyAffectedContainers();
}

const std::string_view& GroupRoutingLatencyLookup::getType() const {
  return type;
}

std::optional<AffectedByChange> GroupRoutingLatencyLookup::isAffectedByChange(
    const AffectedByChangeDecisionData& data) const {
  // NOTE: although this expression has a child, we still need to set
  // isAffectedByChange for this expression type, since the child expression is
  // multi-valued, and so the Orchestrator cannot directly know if there is an
  // update to the child.
  return getOnlyChildRawPtr()->isAffectedByChange(data);
}

void GroupRoutingLatencyLookup::updateEquivalenceSets(
    EquivalenceSets& /*equivalenceSets*/) const {
  // equivalent sets is updated by the child GroupRoutingRing expression
}

double GroupRoutingLatencyLookup::evaluate(
    const BottomToTopEvaluator& evaluator,
    const ChangeSet& /*unused*/) const {
  auto groupRoutingRingExpr =
      static_cast<GroupRoutingRing*>(getOnlyChildRawPtr());

  // The child here is multi-valued, meaning it has multiple values of interest.
  // So, we need to explicitly check if there was update for the child and
  // recalculate the value of this expression if so. When performing
  // evaluations, the fact that it has an update depends on whether there is a
  // table associated with the child in context.groupToTempTrafficTable()
  const auto& contextGroupToTempTrafficTable =
      evaluator.getContext().groupToTempTrafficTable();
  if (!contextGroupToTempTrafficTable.contains(groupRoutingRingExpr->getId())) {
    // this means that the changes were not relevant and hence the traffic table
    // was not updated; so, can return the existing value.
    return value;
  }

  auto& tempTrafficTable =
      contextGroupToTempTrafficTable.at(groupRoutingRingExpr->getId());
  return aggregateLatency(tempTrafficTable);
}

double GroupRoutingLatencyLookup::applyUsingTrafficTableFromChild() {
  auto groupRoutingRingExpr =
      static_cast<GroupRoutingRing*>(getOnlyChildRawPtr());

  // use the updated traffic table from the child
  const auto& trafficTable = groupRoutingRingExpr->getTrafficTableWithStats();

  value = aggregateLatency(trafficTable);
  return value;
}

double GroupRoutingLatencyLookup::innerFullApply(
    const TopToBottomEvaluator& evaluator,
    const Assignment& assignment) {
  evaluator.apply(getOnlyChildRawPtr(), assignment);
  return applyUsingTrafficTableFromChild();
}

double GroupRoutingLatencyLookup::innerPartialApply(
    const BottomToTopEvaluator& /*unused*/,
    const Assignment& /*unused*/,
    const ChangeSet& /*unused*/) {
  // note that the changeSet will be used by the child and it is expected that
  // Orchestrator has already updated the child
  return applyUsingTrafficTableFromChild();
}

bool GroupRoutingLatencyLookup::shouldComputeDescendingChildPotentials() const {
  return false;
}

Bounds GroupRoutingLatencyLookup::innerLowerAndUpperBounds(
    Context& /* context */,
    const BoundConstraints& /* bc */) const {
  auto groupRoutingRingExpr =
      static_cast<GroupRoutingRing*>(getOnlyChildRawPtr());
  return {
      .lower_bound = groupRoutingRingExpr->getMinPossibleLatencyValue(),
      .upper_bound = groupRoutingRingExpr->getMaxPossibleLatencyValue()};
}

double GroupRoutingLatencyLookup::aggregateLatency(
    const TrafficTableWithStats<Origin, Destination>& trafficTable) const {
  switch (*aggregationMetric_.type()) {
    case interface::RoutingLatencyMetric::AVG:
      return trafficTable.getWeightedAvgLatency();

    case interface::RoutingLatencyMetric::MAX:
      return trafficTable.getMaxLatency();

    case interface::RoutingLatencyMetric::P99:
      return trafficTable.getPercentileLatency(99);

    case interface::RoutingLatencyMetric::PERCENTILE: {
      if (!aggregationMetric_.percentile().has_value()) {
        throw std::runtime_error(fmt::format("expected percentile value"));
      }
      return trafficTable.getPercentileLatency(
          *aggregationMetric_.percentile());
    }
  }

  throw std::runtime_error(
      fmt::format(
          "should not reach here; unknown aggregation metric {} for GroupRoutingLatencyLookup",
          apache::thrift::util::enumNameSafe(*aggregationMetric_.type())));
}

} // namespace facebook::rebalancer
