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

#include "algopt/lp/generic/Expression.h"
#include "algopt/lp/generic/Relation.h"
#include "algopt/lp/generic/Variable.h"

namespace facebook::algopt::lp {

Expression operator*(double coeff, const Variable& variable);
Expression operator*(const Variable& variable, double coeff);
Expression operator/(const Variable& variable, double coeff);
Expression operator-(const Variable& variable);

Expression operator-(const Expression& expression);
Expression operator+(double constant, const Expression& expression);
Expression operator+(const Expression& expression, double constant);
Expression operator+(
    const Expression& expression1,
    const Expression& expression2);
Expression operator-(double constant, const Expression& expression);
Expression operator-(const Expression& expression, double constant);
Expression operator-(
    const Expression& expression1,
    const Expression& expression2);
Expression operator*(double constant, const Expression& expression);
Expression operator*(const Expression& expression, double constant);
Expression operator*(
    const Expression& expression1,
    const Expression& expression2);
Expression operator/(const Expression& expression, double constant);

// operators modifying expressions
void operator+=(Expression& expression, double constant);
void operator+=(Expression& expression, const Variable& variable);
void operator+=(Expression& expression1, const Expression& expression2);
void operator-=(Expression& expression, double constant);
void operator-=(Expression& expression, const Variable& variable);
void operator-=(Expression& expression1, const Expression& expression2);
void operator*=(Expression& expression, double constant);
void operator*=(Expression& expression1, const Expression& expression2);
void operator/=(Expression& expression, double constant);

// operators producing relations
Relation operator==(double constant, const Expression& expression);
Relation operator==(const Expression& expression, double constant);
Relation operator==(
    const Expression& expression1,
    const Expression& expression2);
Relation operator<=(double constant, const Expression& expression);
Relation operator<=(const Expression& expression, double constant);
Relation operator<=(
    const Expression& expression1,
    const Expression& expression2);
Relation operator>=(double constant, const Expression& expression);
Relation operator>=(const Expression& expression, double constant);
Relation operator>=(
    const Expression& expression1,
    const Expression& expression2);

} // namespace facebook::algopt::lp
