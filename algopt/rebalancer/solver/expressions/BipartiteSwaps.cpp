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

#include "algopt/rebalancer/solver/expressions/BipartiteSwaps.h"

#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/PropertiesHelper.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/equivalence_sets/EquivalenceSets.h"
#include "algopt/rebalancer/solver/utils/Problem.h"

#include <folly/logging/xlog.h>

namespace {
constexpr std::string_view type = "BipartiteSwaps";
}

namespace facebook::rebalancer {

using entities::ContainerId;
using entities::EquivalenceSetId;
using entities::ObjectId;

BipartiteSwaps::BipartiteSwaps(
    PackerMap<ObjectId, ContainerId> initial_assignment,
    PackerSet<ContainerId> left_subset,
    PackerSet<ContainerId> right_subset,
    std::shared_ptr<const entities::Universe> universe)
    : Expression(std::move(universe), /*initialValue=*/1.0),
      initial_assignment(std::move(initial_assignment)),
      left_subset_(std::move(left_subset)),
      right_subset_(std::move(right_subset)) {
  XLOG(DBG1) << "Left bipartite size: " << left_subset_.size()
             << ", right bipartite size:" << right_subset_.size();
  set_directly_affected_containers();
}

const std::string_view& BipartiteSwaps::getType() const {
  return type;
}

void BipartiteSwaps::updateEquivalenceSets(
    EquivalenceSets& equivalenceSets) const {
  equivalenceSets.mappingMerge(initial_assignment);
}

double BipartiteSwaps::evaluate(
    const BottomToTopEvaluator& /* unused */,
    const ChangeSet& changes) const {
  auto moves = MoveSet::fromChangeSet(changes);
  auto counts_delta = get_delta(moves);
  return evaluate_delta(counts_delta);
}

double BipartiteSwaps::innerFullApply(
    const TopToBottomEvaluator& /* unused */,
    const Assignment& assignment) {
  // step 1: undo all moves
  value = 1;
  counts_base.clear();

  // step 2: compute delta between initial and new assignments
  auto moves = get_moves(assignment);
  auto delta = get_delta(moves);

  // step 3: evaluate delta and update base
  value = evaluate_delta(delta);
  for (auto& [transition, count] : delta) {
    counts_base[transition] += count;
  }
  return value;
}

double BipartiteSwaps::innerPartialApply(
    const BottomToTopEvaluator& /* unused */,
    const Assignment& /* unused */,
    const ChangeSet& changes) {
  auto moves = MoveSet::fromChangeSet(changes);
  auto delta = get_delta(moves);
  value = evaluate_delta(delta);
  return value;
}

std::optional<AffectedByChange> BipartiteSwaps::isAffectedByChange(
    const AffectedByChangeDecisionData& /*data*/) const {
  return AffectedByChange(true /*affectedByAllChanges*/);
}

Bounds BipartiteSwaps::innerLowerAndUpperBounds(
    Context& /* unused */,
    const BoundConstraints& /* unused */) const {
  return {.lower_bound = 0, .upper_bound = 1};
}

ExprPtr BipartiteSwaps::getExprForLp(const LpEvaluator& evaluator) const {
  XLOG(DBG1) << "Building bipartite swap constraints between two partitions "
             << left_subset_.size() << " of left partition "
             << right_subset_.size() << " of right partition";

  auto& problem = evaluator.getProblem();
  const Assignment& lpAssignment = problem.initial_assignment;
  auto& equivalenceSets = problem.getEquivalenceSets();
  auto invalidSwapsCount = const_expr(0, getUniversePtr());
  PackerMap<ContainerId, PackerSet<EquivalenceSetId>> containerToEqSet;
  PackerSet<EquivalenceSetId> all_equivalence_sets;
  for (auto& [object, container] : initial_assignment) {
    auto eqSet = equivalenceSets.at(object);
    all_equivalence_sets.insert(eqSet);
    containerToEqSet[container].insert(eqSet);
  }

  // Any moves to initially empty containers are non swaps.
  for (auto container : problem.containers) {
    if (containerToEqSet.contains(container)) {
      continue;
    }
    for (auto eqSetId : all_equivalence_sets) {
      auto repObject = *equivalenceSets.getSet(eqSetId).begin();
      inplace_max(
          invalidSwapsCount,
          variable(repObject, container, getUniversePtr(), lpAssignment),
          getUniversePtr());
    }
  }

  for (auto leftContainer : left_subset_) {
    for (auto rightContainer : right_subset_) {
      auto movingLeftToRight = const_expr(0, getUniversePtr());
      for (auto leftEqSetId : containerToEqSet[leftContainer]) {
        // for each equivalence class of object initially in the left container,
        // we count how many objects end up in the right container
        auto repObject = *equivalenceSets.getSet(leftEqSetId).begin();
        auto size = equivalenceSets.getSet(leftEqSetId).size();
        movingLeftToRight += size *
            variable(repObject, rightContainer, getUniversePtr(), lpAssignment);
      }

      auto movingRightToLeft = const_expr(0, getUniversePtr());
      for (auto rightEqSetId : containerToEqSet[rightContainer]) {
        // symmetrical case: count objects from the right container that show up
        // in the left container
        auto repObject = *equivalenceSets.getSet(rightEqSetId).begin();
        auto size = equivalenceSets.getSet(rightEqSetId).size();
        movingRightToLeft += size *
            variable(repObject, leftContainer, getUniversePtr(), lpAssignment);
      }

      // Difference between each pair of left and right container should be 0
      // NOTE: This doesn't guarante no moves within same side of the
      // bipartite
      auto diff = movingRightToLeft - movingLeftToRight;
      inplace_max(invalidSwapsCount, diff, getUniversePtr());
      inplace_max(invalidSwapsCount, -1 * diff, getUniversePtr());
    }
  }

  return 1 - step(std::move(invalidSwapsCount), getUniversePtr());
}

algopt::lp::Expression BipartiteSwaps::lp(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs) {
  if (!lpExpr_) {
    lpExpr_ = getExprForLp(evaluator);
  }

  return evaluator.lp(lpExpr_.get(), minimizing, configs);
}

void BipartiteSwaps::set_directly_affected_containers() {
  auto localDirectlyAffectedContainersPtr =
      std::make_shared<PackerSet<ContainerId>>();
  for (auto& it : initial_assignment) {
    localDirectlyAffectedContainersPtr->insert(it.second);
  }

  directlyAffectedContainers.set(localDirectlyAffectedContainersPtr);
}

MoveSet BipartiteSwaps::get_moves(const Assignment& assignment) const {
  MoveSet moves;
  for (auto& [object, initial_container] : initial_assignment) {
    auto new_container = assignment.getContainer(object);
    if (initial_container != new_container) {
      moves.insert(Move(object, initial_container, new_container));
    }
  }
  return moves;
}

BipartiteSwaps::CounterMap BipartiteSwaps::get_delta(
    const MoveSet& moves) const {
  CounterMap counts_delta;
  // For each move, calculate left objects moved and right objects moved for
  // every pair of source and destination container.
  // For every pair of these containers, the Counters should be identical.
  for (auto& move : moves) {
    auto initial_container = initial_assignment.at(move.getObject());
    auto current_container = move.getSourceContainer();
    auto new_container = move.getDestinationContainer();

    if (initial_container != current_container) {
      // undo previous transition by the same object
      counts_delta[std::make_pair(initial_container, current_container)] -= 1;
    }
    if (initial_container != new_container) {
      counts_delta[std::make_pair(initial_container, new_container)] += 1;
    }
  }
  return counts_delta;
}

bool BipartiteSwaps::is_same_side_move(const Transition& transition) const {
  return left_subset_.contains(transition.first) ==
      left_subset_.contains(transition.second);
}

double BipartiteSwaps::evaluate_delta(const CounterMap& counts_delta) const {
  // A valid swap exchanges an object inside the subset with an
  // object outside the subset.
  // Formula explanation: counts_delta contains pairs <c1,c2>
  // as keys where c1 and c2 may or may not be on opposite sides..

  // step 1: check the total delta from current transitions and past transitions
  // The move will be invalid if total exchanged delta between c1,c2 don't match
  for (auto& [transition, delta] : counts_delta) {
    if (is_same_side_move(transition)) {
      continue;
    }
    auto opposite_transition =
        std::make_pair(transition.second, transition.first);
    const int opposite_delta =
        folly::get_default(counts_delta, opposite_transition, 0);
    const int count = folly::get_default(counts_base, transition, 0) + delta;
    const int opposite_count =
        folly::get_default(counts_base, opposite_transition, 0) +
        opposite_delta;
    if (count != opposite_count) {
      return 0;
    }
  }
  // shortcut: if all moves so far are swaps, checking the delta is enough
  if (value == 1) {
    return 1;
  }

  // step 2: check the rest of transitions from past set of moves
  for (auto& [transition, count] : counts_base) {
    // Don't care about same side moves
    if (is_same_side_move(transition)) {
      continue;
    }
    auto opposite_transition =
        std::make_pair(transition.second, transition.first);
    if (counts_delta.contains(transition) ||
        counts_delta.contains(opposite_transition)) {
      continue;
    }
    const int opposite_count =
        folly::get_default(counts_base, opposite_transition, 0);
    // For moves across sides, counts must match
    if (count != opposite_count) {
      return 0;
    }
  }
  return 1;
}

ExpressionProperties BipartiteSwaps::getProperties() const {
  ExpressionProperties properties;
  properties.properties()->emplace(
      "left_subset", PropertiesHelper::makeContainerIdListValue(left_subset_));
  properties.properties()->emplace(
      "right_subset",
      PropertiesHelper::makeContainerIdListValue(right_subset_));
  return properties;
}

void BipartiteSwaps::lpIntent(const LpEvaluator& evaluator, bool minimizing) {
  if (!lpExpr_) {
    lpExpr_ = getExprForLp(evaluator);
  }

  return evaluator.computeLpIntent(lpExpr_, minimizing);
}

} // namespace facebook::rebalancer
