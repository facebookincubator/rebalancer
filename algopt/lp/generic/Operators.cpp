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

#include "algopt/lp/generic/Operators.h"

#include "algopt/rebalancer/algopt_common/Precision.h"

namespace facebook::algopt::lp {

Expression operator*(double coeff, const Variable& variable) {
  return variable * coeff;
}

Expression operator*(const Variable& variable, double coeff) {
  return Expression(variable.get()->makeExpression(coeff));
}

Expression operator/(const Variable& variable, double coeff) {
  return variable * (1.0 / coeff);
}

Expression operator-(const Variable& variable) {
  return variable * -1.0;
}

Expression operator-(const Expression& expression) {
  return expression * -1.0;
}

Expression operator+(double constant, const Expression& expression) {
  return expression + constant;
}

Expression operator+(const Expression& expression, double constant) {
  if (algopt::Precision::compare(constant, 0) == 0) {
    return expression;
  }

  auto newExpression = expression;
  newExpression.add(constant);
  return newExpression;
}

Expression operator+(
    const Expression& expression1,
    const Expression& expression2) {
  auto newExpression = expression1;
  newExpression.add(expression2);
  return newExpression;
}

Expression operator-(double constant, const Expression& expression) {
  return constant + -expression;
}

Expression operator-(const Expression& expression, double constant) {
  return expression + -constant;
}

Expression operator-(
    const Expression& expression1,
    const Expression& expression2) {
  return expression1 + -expression2;
}

Expression operator*(double constant, const Expression& expression) {
  return expression * constant;
}

Expression operator*(const Expression& expression, double constant) {
  if (algopt::Precision::compare(constant, 1) == 0) {
    return expression;
  }

  auto newExpression = expression;
  newExpression.multiply(constant);
  return newExpression;
}

Expression operator*(
    const Expression& expression1,
    const Expression& expression2) {
  auto newExpression = expression1;
  newExpression.multiply(expression2);
  return newExpression;
}

Expression operator/(const Expression& expression, double constant) {
  return expression * (1.0 / constant);
}

void operator+=(Expression& expression, double constant) {
  expression.add(constant);
}

void operator+=(Expression& expression, const Variable& variable) {
  expression += Expression(variable);
}

void operator+=(Expression& expression1, const Expression& expression2) {
  expression1.add(expression2);
}

void operator-=(Expression& expression, double constant) {
  expression += -constant;
}

void operator-=(Expression& expression, const Variable& variable) {
  expression += -variable;
}

void operator-=(Expression& expression1, const Expression& expression2) {
  expression1 += -expression2;
}

void operator*=(Expression& expression, double constant) {
  expression.multiply(constant);
}

void operator*=(Expression& expression1, const Expression& expression2) {
  expression1.multiply(expression2);
}

void operator/=(Expression& expression, double constant) {
  expression *= 1.0 / constant;
}

Relation operator==(double constant, const Expression& expression) {
  if (algopt::Precision::compare(constant, 0) == 0) {
    // avoid unnecessary subtraction, which in turn causes extra copies
    return expression.makeEqualZeroRelation();
  }
  return (constant - expression).makeEqualZeroRelation();
}

Relation operator==(const Expression& expression, double constant) {
  if (algopt::Precision::compare(constant, 0) == 0) {
    // avoid unnecessary subtraction, which in turn causes extra copies
    return expression.makeEqualZeroRelation();
  }
  return (expression - constant).makeEqualZeroRelation();
}

Relation operator==(
    const Expression& expression1,
    const Expression& expression2) {
  return (expression1 - expression2).makeEqualZeroRelation();
}

Relation operator<=(double constant, const Expression& expression) {
  if (algopt::Precision::compare(constant, 0) == 0) {
    // avoid unnecessary subtraction, which in turn causes extra copies
    return expression.makeGreaterEqualZeroRelation();
  }
  return (constant - expression).makeLessEqualZeroRelation();
}

Relation operator<=(const Expression& expression, double constant) {
  if (algopt::Precision::compare(constant, 0) == 0) {
    // avoid unnecessary subtraction, which in turn causes extra copies
    return expression.makeLessEqualZeroRelation();
  }
  return (expression - constant).makeLessEqualZeroRelation();
}

Relation operator<=(
    const Expression& expression1,
    const Expression& expression2) {
  return (expression1 - expression2).makeLessEqualZeroRelation();
}

Relation operator>=(double constant, const Expression& expression) {
  if (algopt::Precision::compare(constant, 0) == 0) {
    // avoid unnecessary subtraction, which in turn causes extra copies
    return expression.makeLessEqualZeroRelation();
  }
  return (constant - expression).makeGreaterEqualZeroRelation();
}

Relation operator>=(const Expression& expression, double constant) {
  if (algopt::Precision::compare(constant, 0) == 0) {
    // avoid unnecessary subtraction, which in turn causes extra copies
    return expression.makeGreaterEqualZeroRelation();
  }
  return (expression - constant).makeGreaterEqualZeroRelation();
}

Relation operator>=(
    const Expression& expression1,
    const Expression& expression2) {
  return (expression1 - expression2).makeGreaterEqualZeroRelation();
}

} // namespace facebook::algopt::lp
