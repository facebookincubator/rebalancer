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

#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/moves/MoveSet.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/Util.h"

namespace facebook::rebalancer {

// The Swaps expression evaluates to 1 if the current assignment can be obtained
// by applying independent object swaps to the initial assignment, 0 otherwise.
// A non-empty subset imposes the extra constraint that each of the object swaps
// must move at least one object in the subset.
class Swaps : public Expression {
 public:
  enum SubsetDefinition {
    // Definition 1: a valid swap exchanges an object inside the subset with any
    // other object (inside or outside the subset).
    AT_LEAST_ONE_IN_SUBSET,
    // Definition 2: a valid swap exchanges an object inside the subset with an
    // object outside the subset.
    EXACTLY_ONE_IN_SUBSET,
    // Definition 3: Both same side of subset: objects are only allowed to move
    // via exclusive swaps with other objects on the same side of subset. That
    // means object in_subset can move via exclusive sawps with other objects
    // in_subset and objects outside of subset can move via exclusive swaps
    // with other object outside of subset.
    BOTH_SAME_SIDE_OF_SUBSET,
  };

 public:
  explicit Swaps(
      const PackerMap<entities::ObjectId, entities::ContainerId>&
          initial_assignment,
      std::shared_ptr<const entities::Universe> universe,
      const folly::Optional<PackerSet<entities::ObjectId>>& subset = {},
      SubsetDefinition subsetDefinition =
          SubsetDefinition::AT_LEAST_ONE_IN_SUBSET);

  virtual bool inner_is_integer(Context& context) override;

  void updateEquivalenceSets(EquivalenceSets& equivSets) const override;

  using Expression::evaluate;
  virtual double evaluate(
      const BottomToTopEvaluator& evaluator,
      const ChangeSet& changes) const override;

  using Expression::lp;
  virtual algopt::lp::Expression lp(
      const LpEvaluator& evaluator,
      bool minimizing,
      const interface::OptimalSolverSpec& configs) override;

  virtual const std::string_view& getType() const override;

  virtual std::optional<AffectedByChange> isAffectedByChange(
      const AffectedByChangeDecisionData& data) const override;

  void lpIntent(const LpEvaluator& evaluator, bool minimizing) override;

  PackerMap<std::pair<entities::ContainerId, int>, int> container_move_cnts;
  PackerMap<std::pair<entities::ContainerId, int>, int>
      container_move_cnts_subset;

 private:
  struct Counter {
    int total = 0; // total number of unique objects in the transition
    int in_subset = 0; // number of unique objects that belong to the subset

    void operator+=(const Counter& other) {
      total += other.total;
      in_subset += other.in_subset;
    }

    void operator-=(const Counter& other) {
      total -= other.total;
      in_subset -= other.in_subset;
    }
  };
  using Transition = std::pair<entities::ContainerId, entities::ContainerId>;
  using CounterMap = PackerMap<Transition, Counter>;

  virtual double innerFullApply(
      const TopToBottomEvaluator& evaluator,
      const Assignment& assignment) override;

  virtual double innerPartialApply(
      const BottomToTopEvaluator& evaluator,
      const Assignment& assignment,
      const ChangeSet& changes) override;

  virtual Bounds innerLowerAndUpperBounds(
      Context& context,
      const BoundConstraints& bc) const override;

  void set_directly_affected_containers();

  // get minimal set of moves that produce the given assignment when applied
  // to the initial assignment
  MoveSet get_moves(const Assignment& assignment) const;

  // compute counters delta for a given set of moves
  CounterMap get_delta(const MoveSet& moves) const;

  // whether two different containers with given counters can exchange their
  // objects with independent swaps that meet the subsets rule if applicable
  bool exchangeable(const Counter& c1, const Counter& c2) const;

  // get count for given transition after applying given delta
  Counter get_counts(const Transition& transition, const Counter& delta) const;

  // evaluate the expression given a counters delta
  double evaluate_delta(const CounterMap& counts_delta) const;

  static Transition opposite(const Transition& transition);

  ExprPtr getEquivExprForLp(const LpEvaluator& evaluator) const;

 private:
  PackerMap<entities::ObjectId, entities::ContainerId> initial_assignment;
  PackerMap<entities::ObjectId, entities::ContainerId> current_assignment;
  folly::Optional<PackerSet<entities::ObjectId>> subset;
  SubsetDefinition subsetDefinition_;

  // counts_base[t] is the count of unique objects that moved from container
  // t.first to container t.second with respect to the initial assignment
  CounterMap counts_base;

  ExprPtr lpExpr_ = nullptr;
};
} // namespace facebook::rebalancer
