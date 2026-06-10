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

#include "algopt/lp/factory/ProblemFactory.h"
#include "algopt/lp/generic/Constraint.h"
#include "algopt/lp/generic/Expression.h"
#include "algopt/lp/generic/Operators.h"
#include "algopt/lp/generic/Problem.h"
#include "algopt/lp/generic/Relation.h"
#include "algopt/lp/generic/Variable.h"

#include <string>
#include <utility>
#include <vector>

namespace facebook::algopt::lp::binding {

// Wrapper classes that ligen can work with
// These are value types that wrap the underlying lp::Variable, lp::Expression,
// etc.

class Expression;
class Relation;

class Variable {
 public:
  explicit Variable(lp::Variable variable) : variable_(std::move(variable)) {}

  void setLB(double lb) noexcept {
    variable_.setLB(lb);
  }

  void setUB(double ub) noexcept {
    variable_.setUB(ub);
  }

  void setBounds(double lb, double ub) noexcept {
    variable_.setBounds(lb, ub);
  }

  void setThreshold(double threshold) noexcept {
    variable_.setThreshold(threshold);
  }

  double getValue() const {
    return variable_.getValue();
  }

  Expression multiply(double coeff) const noexcept;
  Expression divide(double coeff) const noexcept;
  Expression negate() const noexcept;

  Relation eq(double value) const noexcept;
  Relation le(double value) const noexcept;
  Relation ge(double value) const noexcept;

  const lp::Variable& getImpl() const {
    return variable_;
  }

 private:
  lp::Variable variable_;
};

class Expression {
 public:
  Expression() noexcept : expression_() {}

  explicit Expression(double constant) noexcept : expression_(constant) {}

  explicit Expression(const Variable& variable) noexcept
      : expression_(variable.getImpl()) {}

  explicit Expression(lp::Expression expression) noexcept
      : expression_(std::move(expression)) {}

  void add(double constant) noexcept {
    expression_.add(constant);
  }

  void addExpression(const Expression& expression) noexcept {
    expression_.add(expression.expression_);
  }

  void addVariable(const Variable& variable) noexcept {
    expression_.add(variable.getImpl());
  }

  void subtract(double constant) noexcept {
    expression_.add(-constant);
  }

  void subtractExpression(const Expression& expression) noexcept {
    expression_ -= expression.expression_;
  }

  void multiply(double constant) noexcept {
    expression_.multiply(constant);
  }

  void multiplyExpression(const Expression& expression) noexcept {
    expression_ *= expression.expression_;
  }

  void divide(double constant) noexcept {
    expression_ /= constant;
  }

  double getValue() const {
    return expression_.getValue();
  }

  double computeValue() const {
    return expression_.computeValue();
  }

  bool isConstant() const noexcept {
    return expression_.isConstant();
  }

  double getConstant() const noexcept {
    return expression_.getConstant();
  }

  Relation eq(double value) const noexcept;
  Relation eqExpression(const Expression& other) const noexcept;
  Relation le(double value) const noexcept;
  Relation leExpression(const Expression& other) const noexcept;
  Relation ge(double value) const noexcept;
  Relation geExpression(const Expression& other) const noexcept;

  static Expression fromConstant(double constant) noexcept {
    return Expression(constant);
  }

  static Expression fromVariable(const Variable& variable) noexcept {
    return Expression(variable);
  }

  const lp::Expression& getImpl() const {
    return expression_;
  }

 private:
  lp::Expression expression_;
};

class Relation {
 public:
  explicit Relation(lp::Relation relation) noexcept
      : relation_(std::move(relation)) {}

  bool isConstant() const noexcept {
    return relation_.isConstant();
  }

  double getConstantValue() const noexcept {
    return relation_.getConstantValue();
  }

  const lp::Relation& getImpl() const {
    return relation_;
  }

 private:
  lp::Relation relation_;
};

class Solution {
 public:
  explicit Solution(lp::thrift::ProblemResult result) noexcept
      : result_(std::move(result)) {}

  int32_t getStatus() const noexcept {
    return static_cast<int32_t>(*result_.status());
  }

  double getBestObjective() const noexcept {
    return *result_.bestObjective();
  }

  double getBestBound() const noexcept {
    return *result_.bestBound();
  }

  double getAbsoluteGap() const noexcept {
    return *result_.absoluteGap();
  }

  double getRelativeGap() const noexcept {
    return *result_.relativeGap();
  }

  int32_t getNumVariables() const noexcept {
    return static_cast<int32_t>(*result_.problemAttributes()->numVariables());
  }

  int32_t getNumConstraints() const noexcept {
    return static_cast<int32_t>(*result_.problemAttributes()->numConstraints());
  }

 private:
  lp::thrift::ProblemResult result_;
};

class Constraint {
 public:
  explicit Constraint(lp::Constraint constraint) noexcept
      : constraint_(std::move(constraint)) {}

  lp::Constraint& getImpl() {
    return constraint_;
  }

 private:
  lp::Constraint constraint_;
};

class Problem {
 public:
  explicit Problem(lp::Problem problem) : problem_(std::move(problem)) {}

  Variable makeVar(const std::string& name) {
    return Variable(problem_.makeVar(name));
  }

  Variable makeIntVar(const std::string& name) {
    return Variable(problem_.makeIntVar(name));
  }

  Variable makeBoolVar(const std::string& name) {
    return Variable(problem_.makeBoolVar(name));
  }

  Variable makeSemiContVar(const std::string& name, double threshold) {
    return Variable(problem_.makeSemiContVar(name, threshold));
  }

  Variable makeSemiIntVar(const std::string& name, double threshold) {
    return Variable(problem_.makeSemiIntVar(name, threshold));
  }

  Constraint newConstraint(const Relation& relation, const std::string& name) {
    return Constraint(problem_.newConstraint(relation.getImpl(), name));
  }

  void deleteConstraint(const Constraint& constraint) {
    problem_.deleteConstraint(const_cast<Constraint&>(constraint).getImpl());
  }

  void setObjective(const Expression& objective) {
    problem_.setObjective(objective.getImpl());
  }

  void solve() {
    problem_.solve();
  }

  double getParameter(const std::string& name) {
    return problem_.getParameter(name);
  }

  void setParameter(const std::string& name, double value) {
    problem_.setParameter(name, value);
  }

  void setMaxSolveTime(double solveTime) {
    problem_.setMaxSolveTime(solveTime);
  }

  void disableLogs() {
    problem_.disableLogs();
  }

  void setLogfile(const std::string& filename) {
    problem_.setLogfile(filename);
  }

  void saveToFile(const std::string& filename) {
    problem_.saveToFile(filename);
  }

  void addStartValue(const Variable& variable, double value) {
    problem_.addStartValue(variable.getImpl(), value);
  }

  Solution getSolution() {
    return Solution(problem_.getOnlyResult());
  }

 private:
  lp::Problem problem_;
};

class ProblemFactory {
 public:
  static Problem makeXpressProblem() {
    return Problem(lp::ProblemFactory::makeXpressProblem());
  }
};

// Helper functions
Expression sumExpressions(const std::vector<Expression>& expressions) noexcept;
Expression sumVariables(const std::vector<Variable>& variables) noexcept;

// Inline implementations for methods that need Relation/Expression defined

inline Expression Variable::multiply(double coeff) const noexcept {
  return Expression(variable_ * coeff);
}

inline Expression Variable::divide(double coeff) const noexcept {
  return Expression(variable_ / coeff);
}

inline Expression Variable::negate() const noexcept {
  return Expression(-variable_);
}

inline Relation Variable::eq(double value) const noexcept {
  return Relation(variable_ == value);
}

inline Relation Variable::le(double value) const noexcept {
  return Relation(variable_ <= value);
}

inline Relation Variable::ge(double value) const noexcept {
  return Relation(variable_ >= value);
}

inline Relation Expression::eq(double value) const noexcept {
  return Relation(expression_ == value);
}

inline Relation Expression::eqExpression(
    const Expression& other) const noexcept {
  return Relation(expression_ == other.expression_);
}

inline Relation Expression::le(double value) const noexcept {
  return Relation(expression_ <= value);
}

inline Relation Expression::leExpression(
    const Expression& other) const noexcept {
  return Relation(expression_ <= other.expression_);
}

inline Relation Expression::ge(double value) const noexcept {
  return Relation(expression_ >= value);
}

inline Relation Expression::geExpression(
    const Expression& other) const noexcept {
  return Relation(expression_ >= other.expression_);
}

} // namespace facebook::algopt::lp::binding
