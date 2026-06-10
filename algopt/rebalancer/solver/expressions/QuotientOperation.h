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

#include "algopt/rebalancer/solver/expressions/BinaryOperation.h"
#include "algopt/rebalancer/solver/expressions/Evaluator.h"

#pragma once

namespace facebook::rebalancer {

class QuotientOperation : public BinaryOperation {
 public:
  explicit QuotientOperation(
      std::shared_ptr<Expression> expr1,
      std::shared_ptr<Expression> expr2,
      std::shared_ptr<const entities::Universe> universe);

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

  virtual void lpIntent(const LpEvaluator& evaluator, bool minimizing) override;

  virtual const std::string_view& getType() const override;

 protected:
  virtual double innerFullApply(
      const TopToBottomEvaluator& evaluator,
      const Assignment& assignment) override;

  virtual double innerPartialApply(
      const BottomToTopEvaluator& evaluator,
      const Assignment& assignment,
      const ChangeSet& changes) override;

  virtual std::vector<double> bound_candidates(
      Context& context,
      const BoundConstraints& bc) const override;

 private:
  double _applyUsingChildValues(
      const Evaluator& evaluator,
      const Assignment& assignment);

  algopt::lp::Expression approximate_quotient_log(
      const algopt::lp::Expression& numerator,
      const algopt::lp::Expression& denominator,
      double denom_lb,
      double denom_ub,
      int log_n_segments,
      const LpEvaluator& evaluator);
};

} // namespace facebook::rebalancer
