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
#include "algopt/rebalancer/solver/if/gen-cpp2/packer_types.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/Util.h"

namespace facebook::rebalancer {

class EquivalenceSets;

class Piecewise : public Expression {
 public:
  // y = piecewise(x)
  // points: vertices of the piecewise linear function
  // defines the whole graph for Piecewise function
  //
  // expr is x here, x should be in range of what points define
  // x is not allowed to pass first point and last point
  //
  // continuous in most cases should be true
  // if false, then y does not have only value sometimes
  // so caller really needs to know what they are doing
  // when set it to false
  //
  // in apply, y takes the value from smaller points
  explicit Piecewise(
      const std::vector<std::pair<double, double>>& points,
      std::shared_ptr<Expression> expr,
      std::shared_ptr<const entities::Universe> universe,
      bool continuous = true);

  std::string innerDigest(size_t maxChildren = 10) const override;

  void updateEquivalenceSets(EquivalenceSets& equivSets) const override;

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

 private:
  // points which defines piecewise graph
  // for pairs format is (x, y)
  // x from small to large
  const std::vector<std::pair<double, double>> points_;

  // to get (y - prevY)/(x - prevX)
  std::vector<double> slopes_;

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

  double performPiecewise(double val) const;
};

std::vector<std::pair<double, double>> sample_real_function(
    const std::function<double(double)>& function,
    double lb,
    double ub,
    int count);

algopt::lp::Expression build_piecewise_linear_function(
    const std::vector<std::pair<double, double>>& points,
    ExprPtr expr,
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs);

algopt::lp::Expression build_piecewise_linear_function(
    const std::function<double(double)>& function,
    double hard_lb,
    double hard_ub,
    double soft_lb,
    double soft_ub,
    size_t soft_segments,
    ExprPtr expr,
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs);

algopt::lp::Expression build_piecewise_linear_function(
    const std::function<double(double)>& function,
    double lb,
    double ub,
    size_t segments,
    ExprPtr expr,
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs);
} // namespace facebook::rebalancer
