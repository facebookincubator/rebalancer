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
#include "algopt/rebalancer/solver/if/gen-cpp2/packer_types.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"

namespace facebook::rebalancer {

class EquivalenceSets;

class BoundsOverride : public Expression {
 public:
  explicit BoundsOverride(
      std::shared_ptr<Expression> child,
      std::optional<double> lowerBound,
      std::optional<double> upperBound,
      std::shared_ptr<const entities::Universe> universe);

  void updateEquivalenceSets(EquivalenceSets& equivSets) const override;

  AbstractContainer<ObjectPotential> getObjectPotentials(
      bool descending) const override;
  ExpressionProperties getProperties() const override;

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

  virtual std::optional<std::pair<entities::ObjectId, entities::ContainerId>>
  getVar() const override;

  virtual std::vector<std::pair<Expression*, double>> get_sorted_children(
      bool descending) const override;

  virtual std::optional<AffectedByChange> isAffectedByChange(
      const AffectedByChangeDecisionData& data) const override;

  virtual double getChildCoefficient(Expression* child) const override;

  virtual bool hasNoLpIntent() const override;

 private:
  virtual Bounds innerLowerAndUpperBounds(
      Context& context,
      const BoundConstraints& bc) const override;

  virtual double innerFullApply(
      const TopToBottomEvaluator& evaluator,
      const Assignment& assignment) override;

  virtual double innerPartialApply(
      const BottomToTopEvaluator& evaluator,
      const Assignment& assignment,
      const ChangeSet& changes) override;

  virtual void lpIntent(const LpEvaluator& evaluator, bool minimizing) override;

  std::string innerDigest(size_t maxChildren = 10) const override;

  virtual bool inner_is_integer(Context& context) override;

 private:
  std::optional<double> lowerBound_;
  std::optional<double> upperBound_;
};
} // namespace facebook::rebalancer
