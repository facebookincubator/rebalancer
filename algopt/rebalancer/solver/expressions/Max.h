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
#include "algopt/rebalancer/solver/expressions/Evaluator.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/Util.h"

namespace facebook::rebalancer {

class EquivalenceSets;

class Max : public Expression {
 public:
  explicit Max(
      const std::vector<std::shared_ptr<Expression>>&,
      std::shared_ptr<const entities::Universe> universe);

  void combine(const ExprPtr& expr);
  void add(const ExprPtr& expr);

  // Order children by value, highest first. Ties broken arbitrarily on the
  // Expression* to give the underlying set a strict weak ordering.
  struct SortedValuesCompare {
    bool operator()(
        const std::pair<Expression*, double>& lhs,
        const std::pair<Expression*, double>& rhs) const {
      if (lhs.second == rhs.second) {
        return lhs.first < rhs.first;
      }
      return lhs.second > rhs.second;
    }
  };
  using SortedValues =
      algopt::ValueSortedMap<Expression*, double, SortedValuesCompare>;
  SortedValues sorted_values_;

  virtual bool inner_is_integer(Context& context) override;
  std::string innerDigest(size_t maxChildren = 10) const override;

  void updateEquivalenceSets(EquivalenceSets& equivSets) const override;

  void lpIntent(const LpEvaluator& evaluator, bool minimizing) override;

  AbstractContainer<ObjectPotential> getObjectPotentials(
      bool descending) const override;

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

  virtual bool isMax() const override;

 private:
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

  void buildSortedAndSetInitialValue();

  bool is_each_child_binary(const Evaluator& evaluator);
  algopt::lp::Expression binary_inner_lp(
      const LpEvaluator& evaluator,
      bool minimizing,
      const interface::OptimalSolverSpec& configs);

  bool is_each_child_integer(const Evaluator& evaluator);
  algopt::lp::Expression n_to_n_plus_one_inner_lp(
      const LpEvaluator& evaluator,
      bool minimizing,
      const interface::OptimalSolverSpec& configs);

  ExprPtr binaryChildrenSum_;
  ExprPtr equivExprForNonGeneralChildren_;
  std::optional<bool> atleastOneGeneralChild_;
};
} // namespace facebook::rebalancer
