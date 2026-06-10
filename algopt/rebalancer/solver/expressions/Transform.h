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
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/Util.h"

namespace facebook::rebalancer {

class Change;

struct ApproximationHint {
  bool valid;
  double upper_bound;
  double lower_bound;
  size_t piece_count;
};

class Transform : public Expression {
 public:
  explicit Transform(
      std::shared_ptr<Expression> expr,
      std::shared_ptr<const entities::Universe> universe,
      std::optional<ApproximationHint> hint = std::nullopt);

  void updateEquivalenceSets(EquivalenceSets& equivSets) const override;

  std::string innerDigest(size_t maxChildren = 10) const override;

  using Expression::evaluate;
  virtual double evaluate(
      const BottomToTopEvaluator& evaluator,
      const ChangeSet& changes) const override;

  AbstractContainer<ObjectPotential> getObjectPotentials(
      bool descending) const override;

  folly::Optional<std::pair<double, double>> soft_bounds;
  size_t piece_count;

 protected:
  virtual double perform_transform(double val) const = 0;

  algopt::lp::Expression approximate_function(
      const LpEvaluator& evaluator,
      bool minimizing,
      const interface::OptimalSolverSpec& configs,
      const std::function<double(double)>& function);

  virtual double innerFullApply(
      const TopToBottomEvaluator& evaluator,
      const Assignment& assignment) override;

  virtual double innerPartialApply(
      const BottomToTopEvaluator& evaluator,
      const Assignment& assignment,
      const ChangeSet& changes) override;

  double _applyUsingChildValues(
      const Evaluator& evaluator,
      const Assignment& assignment);

  virtual Bounds innerLowerAndUpperBounds(
      Context& context,
      const BoundConstraints& bc) const override;
};
} // namespace facebook::rebalancer
