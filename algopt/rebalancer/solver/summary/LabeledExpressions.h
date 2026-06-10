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

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace facebook::rebalancer {

struct LabeledExpression {
  LabeledExpression(std::string name, ExprPtr expression, double weight);

  std::string name;
  ExprPtr expression;
  double weight;
};

using LabeledExpressionPtr = std::shared_ptr<const LabeledExpression>;
using LabeledExpressionSet = std::vector<LabeledExpressionPtr>;

class LabeledExpressions {
 public:
  void setRoot(ConstExprPtr expression);
  void addSingle(std::string name, ExprPtr expression, double weight = 1);
  void addSingle(std::shared_ptr<const LabeledExpression> expr);

  LabeledExpressionSet::const_iterator begin() const;
  LabeledExpressionSet::const_iterator end() const;
  LabeledExpressionSet::size_type size() const;

  ConstExprPtr getRoot() const;
  const LabeledExpressionSet& getExpressions() const;

 private:
  ConstExprPtr root;
  LabeledExpressionSet expressions;
};

} // namespace facebook::rebalancer
