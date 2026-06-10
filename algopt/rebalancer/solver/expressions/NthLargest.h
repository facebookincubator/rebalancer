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

#include "algopt/rebalancer/solver/expressions/Evaluator.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <map>

namespace facebook::rebalancer {

// NthLargest is an expression that evaluates to the n-th largest child:
// - n is a 0-based index.
// - If unique is true, then the expression evaluates to the n-th largest
//   unique value among children. Otherwise it considers all children values
//   with their potential duplicates.
// - If n is greater than the number of children or unique values, then the
//   expression will evaluate to the smallest value among children.
class NthLargest : public Expression {
 public:
  NthLargest(
      const std::vector<std::shared_ptr<Expression>>& values,
      int n,
      bool unique,
      std::shared_ptr<const entities::Universe> universe);
  virtual void updateEquivalenceSets(
      EquivalenceSets& equivalenceSets) const override;

  ExpressionProperties getProperties() const override;

  using Expression::evaluate;
  virtual double evaluate(
      const BottomToTopEvaluator& evaluator,
      const ChangeSet& changes) const override;

  virtual const std::string_view& getType() const override;

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

  double computeNthLargest(
      const std::map<double, int>& valueFrequencyDelta = {}) const;

  double _applyUsingChildValues(
      const Evaluator& evaluator,
      const Assignment& assignment);

  int n_;
  bool unique_;
  std::map<double, int> valueFrequency_;
};

} // namespace facebook::rebalancer
