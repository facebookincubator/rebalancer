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

#include "algopt/rebalancer/solver/expressions/GroupRoutingTrafficLookup.h"

#include "algopt/rebalancer/solver/expressions/BottomToTopEvaluator.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"

#include <stdexcept>

namespace {
constexpr std::string_view type = "GroupRoutingTrafficLookup";
}

namespace facebook::rebalancer {

GroupRoutingTrafficLookup::GroupRoutingTrafficLookup(
    std::shared_ptr<GroupRoutingRing> groupRoutingRing,
    entities::ScopeItemId destinationScopeItemId,
    std::shared_ptr<const entities::Universe> universe)
    : Expression(std::move(universe)),
      destinationScopeItemId_(destinationScopeItemId) {
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

const std::string_view& GroupRoutingTrafficLookup::getType() const {
  return type;
}

std::optional<AffectedByChange> GroupRoutingTrafficLookup::isAffectedByChange(
    const AffectedByChangeDecisionData& data) const {
  // NOTE: although this expression has a child, we still need to set
  // isAffectedByChange for this expression type, since the child expression is
  // multi-valued, and so the Orchestrator cannot directly know if there is an
  // update to the child.
  return getOnlyChildRawPtr()->isAffectedByChange(data);
}

void GroupRoutingTrafficLookup::updateEquivalenceSets(
    EquivalenceSets& /*equivalenceSets*/) const {
  // equivalent sets is updated by the child GroupRoutingRing expression
}

double GroupRoutingTrafficLookup::evaluate(
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
  return tempTrafficTable.getTotalFractionOfTrafficTo(destinationScopeItemId_);
}

double GroupRoutingTrafficLookup::applyUsingTrafficTableFromChild() {
  auto groupRoutingRingExpr =
      static_cast<GroupRoutingRing*>(getOnlyChildRawPtr());

  // use the updated traffic table from the child
  const auto& trafficTable = groupRoutingRingExpr->getTrafficTableWithStats();

  value = trafficTable.getTotalFractionOfTrafficTo(destinationScopeItemId_);
  return value;
}

double GroupRoutingTrafficLookup::innerFullApply(
    const TopToBottomEvaluator& evaluator,
    const Assignment& assignment) {
  evaluator.apply(getOnlyChildRawPtr(), assignment);
  return applyUsingTrafficTableFromChild();
}

double GroupRoutingTrafficLookup::innerPartialApply(
    const BottomToTopEvaluator& /*evaluator*/,
    const Assignment& /*assignment*/,
    const ChangeSet& /*changes*/) {
  // note that the changeSet will be used by the child and it is expected that
  // Orchestrator has already updated the child
  return applyUsingTrafficTableFromChild();
}

Bounds GroupRoutingTrafficLookup::innerLowerAndUpperBounds(
    Context& /* context */,
    const BoundConstraints& /* bc */) const {
  return {.lower_bound = 0.0, .upper_bound = 1.0};
}

bool GroupRoutingTrafficLookup::shouldComputeDescendingChildPotentials() const {
  return false;
}

entities::Map<entities::ScopeItemId, double>
GroupRoutingTrafficLookup::getTrafficFromEachSource() const {
  auto groupRoutingRingExpr =
      static_cast<GroupRoutingRing*>(getOnlyChildRawPtr());
  const auto& trafficTable = groupRoutingRingExpr->getTrafficTableWithStats();
  return trafficTable.getTotalFractionOfTrafficFromEachOrigin(
      destinationScopeItemId_);
}

} // namespace facebook::rebalancer
