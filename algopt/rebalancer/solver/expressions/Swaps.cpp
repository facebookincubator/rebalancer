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

#include "algopt/rebalancer/solver/expressions/Swaps.h"

#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/equivalence_sets/EquivalenceSets.h"
#include "algopt/rebalancer/solver/utils/Problem.h"

#include <folly/logging/xlog.h>

namespace {
constexpr std::string_view type = "Swaps";
}

namespace facebook::rebalancer {

using entities::ContainerId;
using entities::EquivalenceSetId;
using entities::ObjectId;

Swaps::Swaps(
    const PackerMap<ObjectId, ContainerId>& initial_assignment,
    std::shared_ptr<const entities::Universe> universe,
    const folly::Optional<PackerSet<ObjectId>>& subset,
    SubsetDefinition subsetDefinition)
    : Expression(std::move(universe), /*initialValue=*/1.0),
      initial_assignment(initial_assignment),
      subset(subset),
      subsetDefinition_(subsetDefinition) {
  set_directly_affected_containers();
  current_assignment = initial_assignment;
}

const std::string_view& Swaps::getType() const {
  return type;
}

void Swaps::updateEquivalenceSets(EquivalenceSets& equivalenceSets) const {
  equivalenceSets.mappingMerge(initial_assignment);
  if (subset.has_value()) {
    PackerMap<ObjectId, int> tmp;
    for (auto& it : *subset) {
      tmp[it] = 1;
    }
    equivalenceSets.mappingMerge(tmp);
  }
}

std::optional<AffectedByChange> Swaps::isAffectedByChange(
    const AffectedByChangeDecisionData& /* data */) const {
  return AffectedByChange(true /*affectedByAllChanges*/);
}

double Swaps::evaluate(
    const BottomToTopEvaluator& /* evaluator */,
    const ChangeSet& changes) const {
  // MoveSet::fromChangeSet guarantees every object is moved at most once
  auto moves = MoveSet::fromChangeSet(changes);
  auto counts_delta = get_delta(moves);
  return evaluate_delta(counts_delta);
}

double Swaps::innerFullApply(
    const TopToBottomEvaluator& /* evaluator */,
    const Assignment& assignment) {
  // step 1: undo all moves
  value = 1;
  counts_base.clear();
  current_assignment = initial_assignment;

  // step 2: compute delta between initial and new assignments
  auto moves = get_moves(assignment);
  auto delta = get_delta(moves);

  // step 3: evaluate delta and update base
  value = evaluate_delta(delta);
  counts_base = std::move(delta);
  for (auto& move : moves) {
    current_assignment.insert_or_assign(
        move.getObject(), move.getDestinationContainer());
  }
  return value;
}

double Swaps::innerPartialApply(
    const BottomToTopEvaluator& /* evaluator */,
    const Assignment& /* assignment */,
    const ChangeSet& changes) {
  auto moves = MoveSet::fromChangeSet(changes);
  auto delta = get_delta(moves);
  value = evaluate_delta(delta);
  for (auto& [transition, count] : delta) {
    counts_base[transition] += count;
  }
  for (auto& move : moves) {
    current_assignment.insert_or_assign(
        move.getObject(), move.getDestinationContainer());
  }
  return value;
}

Bounds Swaps::innerLowerAndUpperBounds(
    Context& /* unused */,
    const BoundConstraints& /* unused */) const {
  return {.lower_bound = 0, .upper_bound = 1};
}

void Swaps::lpIntent(const LpEvaluator& evaluator, bool minimizing) {
  if (!lpExpr_) {
    lpExpr_ = getEquivExprForLp(evaluator);
  }

  return evaluator.computeLpIntent(lpExpr_, minimizing);
}

ExprPtr Swaps::getEquivExprForLp(const LpEvaluator& evaluator) const {
  auto& problem = evaluator.getProblem();
  auto& equivalenceSets = problem.getEquivalenceSets();
  const Assignment& lpAssignment = problem.initial_assignment;
  auto invalidSwapCount = const_expr(0, getUniversePtr());
  PackerMap<ContainerId, PackerSet<EquivalenceSetId>>
      containerToDynamicEquivalenceSets;
  PackerSet<EquivalenceSetId> dynamicEquivalenceSets;
  for (auto& [object, container] : initial_assignment) {
    if (!problem.assignment.isDynamic(object)) {
      continue;
    }

    auto equivalenceSet = equivalenceSets.at(object);
    dynamicEquivalenceSets.insert(equivalenceSet);
    containerToDynamicEquivalenceSets[container].insert(equivalenceSet);
  }

  // Any moves to containers without dynamic objects initially are non swaps.
  for (auto container : problem.containers) {
    if (containerToDynamicEquivalenceSets.contains(container)) {
      continue;
    }
    for (auto equivalenceSet : dynamicEquivalenceSets) {
      auto& eqSet = equivalenceSets.getSet(equivalenceSet);
      auto representativeObject = *eqSet.begin();
      inplace_max(
          invalidSwapCount,
          eqSet.size() *
              variable(
                  representativeObject,
                  container,
                  getUniversePtr(),
                  lpAssignment),
          getUniversePtr());
    }
  }

  XLOG(DBG1) << "Building pairwise swap constraints among "
             << containerToDynamicEquivalenceSets.size() << " containers";
  for (const auto& [src_container, srcEqSets] :
       containerToDynamicEquivalenceSets) {
    if (!evaluator.getDynamicContainers().contains(src_container)) {
      continue;
    }
    for (const auto& [dst_container, dstEqSets] :
         containerToDynamicEquivalenceSets) {
      if (!evaluator.getDynamicContainers().contains(dst_container)) {
        continue;
      }
      if (src_container >= dst_container) {
        // The constraint is symmetrical: avoid processing the same unordered
        // pair of containers twice.
        continue;
      }
      // Objects in subset that move from src to dst.
      auto src_subset = const_expr(0, getUniversePtr());
      // Objects not in subset that move from src to dst.
      auto src_other = const_expr(0, getUniversePtr());
      for (auto eqSetId : srcEqSets) {
        auto& eqSet = equivalenceSets.getSet(eqSetId);
        auto repObjId = *eqSet.begin();
        auto objectCountInEqSet = eqSet.size();
        if (subset.has_value() && !subset->contains(repObjId)) {
          src_other += objectCountInEqSet *
              variable(repObjId, dst_container, getUniversePtr(), lpAssignment);
        } else {
          src_subset += objectCountInEqSet *
              variable(repObjId, dst_container, getUniversePtr(), lpAssignment);
        }
      }
      // Objects in subset that move from dst to src.
      auto dst_subset = const_expr(0, getUniversePtr());
      // Objects not in subset that move from dst to src.
      auto dst_other = const_expr(0, getUniversePtr());
      for (auto eqSetId : dstEqSets) {
        auto& eqSet = equivalenceSets.getSet(eqSetId);
        auto repObjId = *eqSet.begin();
        auto objectCountInEqSet = eqSet.size();
        if (subset.has_value() && !subset->contains(repObjId)) {
          dst_other += objectCountInEqSet *
              variable(repObjId, src_container, getUniversePtr(), lpAssignment);
        } else {
          dst_subset += objectCountInEqSet *
              variable(repObjId, src_container, getUniversePtr(), lpAssignment);
        }
      }
      auto src_total = src_subset + src_other;
      auto dst_total = dst_subset + dst_other;
      // Moved in one direction must match moved in the opposite direction.
      inplace_max(invalidSwapCount, src_total - dst_total, getUniversePtr());
      inplace_max(invalidSwapCount, dst_total - src_total, getUniversePtr());
      if (!subset.has_value()) {
        continue;
      }

      if (subsetDefinition_ == SubsetDefinition::BOTH_SAME_SIDE_OF_SUBSET) {
        // For this definition, the objects should be swapped within the subset.
        // in subset count of both sides should match.
        inplace_max(
            invalidSwapCount, src_subset - dst_subset, getUniversePtr());
        inplace_max(
            invalidSwapCount, dst_subset - src_subset, getUniversePtr());
        continue;
      }

      // With either definition of subset, the non-subset objects of one side
      // must be swapped with subset objects of the other side, so the
      // non-subset object count of one side must not exceed the subset object
      // count of the other side.
      inplace_max(invalidSwapCount, src_other - dst_subset, getUniversePtr());
      inplace_max(invalidSwapCount, dst_other - src_subset, getUniversePtr());
      if (subsetDefinition_ == SubsetDefinition::EXACTLY_ONE_IN_SUBSET) {
        // Definition 2: a valid swap exchanges an object inside the subset with
        // an object outside the subset. This means not only non-subset object
        // count of one side must not exceed the subset object count of the
        // other side, but such counts must match excactly. Here we add the
        // complementary constraint to guarantee that.
        inplace_max(invalidSwapCount, dst_subset - src_other, getUniversePtr());
        inplace_max(invalidSwapCount, src_subset - dst_other, getUniversePtr());
        continue;
      }
    }
  }

  return 1 - step(invalidSwapCount, getUniversePtr());
}

algopt::lp::Expression Swaps::lp(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs) {
  if (!lpExpr_) {
    lpExpr_ = getEquivExprForLp(evaluator);
  }

  return evaluator.lp(lpExpr_.get(), minimizing, configs);
}

void Swaps::set_directly_affected_containers() {
  auto localDirectlyAffectedContainersPtr =
      std::make_shared<PackerSet<ContainerId>>();
  for (auto& it : initial_assignment) {
    localDirectlyAffectedContainersPtr->insert(it.second);
  }

  directlyAffectedContainers.set(localDirectlyAffectedContainersPtr);
}

MoveSet Swaps::get_moves(const Assignment& assignment) const {
  MoveSet moves;
  for (auto& [object, initial_container] : initial_assignment) {
    auto new_container = assignment.getContainer(object);
    if (initial_container != new_container) {
      moves.insert(Move(object, initial_container, new_container));
    }
  }
  return moves;
}

Swaps::CounterMap Swaps::get_delta(const MoveSet& moves) const {
  CounterMap counts_delta;
  for (auto& move : moves) {
    auto initial_container = initial_assignment.at(move.getObject());
    auto current_container = current_assignment.at(move.getObject());
    auto new_container = move.getDestinationContainer();
    assert(current_container == move.getSourceContainer());
    const Counter c{
        .total = 1,
        .in_subset = static_cast<int>(
            subset.has_value() ? subset->count(move.getObject()) : 0)};
    if (initial_container != current_container) {
      // undo previous transition by the same object
      counts_delta[std::make_pair(initial_container, current_container)] -= c;
    }
    if (initial_container != new_container) {
      counts_delta[std::make_pair(initial_container, new_container)] += c;
    }
  }
  return counts_delta;
}

bool Swaps::exchangeable(const Counter& c1, const Counter& c2) const {
  if (c1.total != c2.total) {
    return false;
  }
  if (!subset.has_value()) {
    return true;
  }
  if (subsetDefinition_ == SubsetDefinition::AT_LEAST_ONE_IN_SUBSET) {
    // Definition 1: a valid swap exchanges an object inside the subset with any
    // other object (inside or outside the subset).
    // Formula explanation: objects not in subset of one side must all be
    // swapped with objects in subset of the other side, so non-subset of first
    // side must be not greater than subset of the second side.
    return c1.total - c1.in_subset <= c2.in_subset &&
        c2.total - c2.in_subset <= c1.in_subset;
  }
  if (subsetDefinition_ == SubsetDefinition::EXACTLY_ONE_IN_SUBSET) {
    // Definition 2: a valid swap exchanges an object inside the subset with an
    // object outside the subset.
    // Formula explanation: all objects not in subset of one side, must be
    // swapped with all objects in subset of the other side, so non-subset of
    // one side must match excatly subset of the other side.
    return c1.total - c1.in_subset == c2.in_subset &&
        c2.total - c2.in_subset == c1.in_subset;
  }
  if (subsetDefinition_ == SubsetDefinition::BOTH_SAME_SIDE_OF_SUBSET) {
    // Definition 3: Both same side of subset: objects are only allowed to move
    // via exclusive swaps with other objects on the same side of subset. That
    // means object in_subset can move via exclusive sawps with other objects
    // in_subset and objects outside of subset can move via exclusive swaps
    // with other object outside of subset.
    // Formula explanation: We need to ensure in_subset of one side should match
    // with in_subset of other side and non-subset of one side should match with
    // non-subset of other side. In_subset check will imply the other as we
    // already have c1.total == c2.total check at the start of this method.
    return c1.in_subset == c2.in_subset;
  }
  throw std::runtime_error(
      fmt::format(
          "unknown swaps subset definition {}",
          fmt::underlying(subsetDefinition_)));
}

Swaps::Counter Swaps::get_counts(
    const Transition& transition,
    const Counter& delta) const {
  const Counter zero;
  auto count = folly::get_default(counts_base, transition, zero);
  count += delta;
  return count;
}

double Swaps::evaluate_delta(const CounterMap& counts_delta) const {
  // step 1: check the delta
  const Counter zero;
  for (auto& [transition, delta] : counts_delta) {
    auto counts = get_counts(transition, delta);
    auto opposite_transition = opposite(transition);
    const auto& opposite_delta =
        folly::get_default(counts_delta, opposite_transition, zero);
    auto opposite_counts = get_counts(opposite_transition, opposite_delta);
    if (!exchangeable(counts, opposite_counts)) {
      return 0;
    }
  }

  // shortcut: if all moves so far are swaps, checking the delta is enough
  if (value == 1) {
    return 1;
  }

  // step 2: check the rest of transitions
  for (auto& [transition, counts] : counts_base) {
    auto opposite_transition = opposite(transition);
    if (counts_delta.contains(transition) ||
        counts_delta.contains(opposite_transition)) {
      continue;
    }
    const auto& opposite_counts =
        folly::get_default(counts_base, opposite_transition, zero);
    if (!exchangeable(counts, opposite_counts)) {
      return 0;
    }
  }
  return 1;
}

Swaps::Transition Swaps::opposite(const Transition& transition) {
  return std::make_pair(transition.second, transition.first);
}

bool Swaps::inner_is_integer(Context& /* not used */) {
  return true;
}

} // namespace facebook::rebalancer
