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

#include <vector>

namespace facebook::rebalancer {

class AnyPositive : public Expression {
 public:
  // feasibilityTolerance is used to determine the level of POSITIVE
  // For example, if feasibilityTolerance is 1e-4
  // then when expr == 1e-5, it is regarded as NON-POSITIVE
  explicit AnyPositive(
      const std::vector<ExprPtr>& exprs,
      std::shared_ptr<const entities::Universe> universe,
      const double feasibilityTolerance);

  void add(const ExprPtr& expr);

  void combine(const ExprPtr& expr);

  void updateEquivalenceSets(EquivalenceSets& sets) const override;

  virtual bool inner_is_integer(Context& context) override;

  virtual void lpIntent(const LpEvaluator& evaluator, bool minimizing) override;

  std::string innerDigest(size_t maxChildren = 10) const override;

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

  virtual bool isAnyPositive() const override;

 private:
  const double feasibilityTolerance;
  ExprPtr lpProvider_;
  PackerSet<Expression*> violatingChildren_;

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
};
} // namespace facebook::rebalancer
