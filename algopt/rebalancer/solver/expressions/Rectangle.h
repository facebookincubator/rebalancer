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
#include "algopt/rebalancer/solver/expressions/Transform.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"

namespace facebook::rebalancer {

class Rectangle : public Transform {
 public:
  Rectangle(
      std::shared_ptr<Expression> expr,
      double lowerBound,
      double upperBound,
      std::shared_ptr<const entities::Universe> universe);

  virtual bool inner_is_integer(Context& context) override;

  virtual ExpressionProperties getProperties() const override;

  using Expression::lp;
  virtual algopt::lp::Expression lp(
      const LpEvaluator& evaluator,
      bool minimizing,
      const interface::OptimalSolverSpec& configs) override;

  virtual void lpIntent(const LpEvaluator& evaluator, bool minimizing) override;

  virtual const std::string_view& getType() const override;

  std::string innerDigest(size_t maxChildren = 10) const override;

 private:
  virtual double perform_transform(double val) const override;

  virtual Bounds innerLowerAndUpperBounds(
      Context& context,
      const BoundConstraints& bc) const override;

  ExprPtr exprForLp_ = nullptr;

  double lowerBound_;
  double upperBound_;
};
} // namespace facebook::rebalancer
