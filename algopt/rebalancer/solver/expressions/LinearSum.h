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

#include "algopt/rebalancer/algopt_common/AssociativeMap.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/if/gen-cpp2/packer_types.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"

namespace facebook::rebalancer {

class EquivalenceSets;

class LinearSum : public Expression {
 public:
  explicit LinearSum(
      std::shared_ptr<const entities::Universe> universe,
      double constant,
      const PackerMap<std::shared_ptr<Expression>, double>& coef = {});

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

  virtual void lpIntent(const LpEvaluator& evaluator, bool minimizing) override;

  virtual const std::string_view& getType() const override;

  virtual bool isLinearSum() const override;

  void combine(const LinearSum& other, double coef);
  LinearSum operator+(const LinearSum& other) const;
  LinearSum operator+(double other) const;
  LinearSum operator*(double other) const;
  void operator+=(const LinearSum& other);
  void operator+=(double other);
  void operator-=(const LinearSum& other);
  void operator-=(double other);
  void operator*=(double other);
  void operator/=(double other);
  bool operator==(double other) const;
  void add(std::shared_ptr<Expression> expr, double coef);

  double getChildCoefficient(Expression* child) const override;

 private:
  std::string innerDigest(size_t maxChildren = 10) const override;

  virtual bool inner_is_integer(Context& context) override;

  double constant_;

  algopt::SumMap<Expression*, double> collection;
  PackerMap<Expression*, double> coefficients_;
  bool allCoeffsOne_ = true;

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
