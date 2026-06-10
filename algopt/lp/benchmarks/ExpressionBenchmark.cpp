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

#include "algopt/lp/detail/generic/impl/GenericProblemImpl.h"
#include "algopt/lp/factory/ProblemFactory.h"
#include "algopt/lp/generic/Operators.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

#include <memory>

using namespace facebook::algopt::lp;
using namespace facebook::algopt::lp::detail;

Problem makeGenericGurobiProblem() {
  return Problem(
      std::make_unique<GenericProblemImpl>(ProblemFactory::makeGurobiProblem));
}

// 'term' refers to a simpler expression of the form 'coeff * variable'. This
// benchmark is more relevant for Xpress where there is significant difference
// between adding terms and larger expressions.
void runAddTerm(const std::function<Problem()>& factory) {
  auto problem = factory();
  auto expr = problem.makeExpression(0);
  for (const auto _ : folly::irange(100000)) {
    auto tmp = 42 * problem.makeVar();
    expr += tmp;
  }
  auto other = expr;
}

void runAddExpression(const std::function<Problem()>& factory) {
  auto problem = factory();
  auto expr = problem.makeExpression(0);
  for (const auto _ : folly::irange(100000)) {
    const Expression tmp = 22 + 42 * problem.makeVar();
    expr += tmp;
  }
  auto other = expr;
}

void runAddConstantExpression(const std::function<Problem()>& factory) {
  auto problem = factory();
  auto expr = problem.makeExpression(0);
  for (const auto _ : folly::irange(100000)) {
    expr += problem.makeExpression(42);
  }
}

void runMultiplyConstantWithLongExpr(const std::function<Problem()>& factory) {
  auto problem = factory();
  auto expr = problem.makeExpression(0);
  BENCHMARK_SUSPEND {
    // create a large expression which is a sum of 1000 variables
    constexpr int exprLength = 1000;
    for (const auto i : folly::irange(exprLength)) {
      expr += problem.makeIntVar(fmt::format("var_{}", i));
    }
  }

  const int multiplicationCount = 1e6;
  Expression sum;
  for (const auto i : folly::irange(multiplicationCount)) {
    const auto c = i * expr;
    BENCHMARK_SUSPEND {
      sum += c;
    }
  }
}

void runAddConstantWithLongExpr(const std::function<Problem()>& factory) {
  auto problem = factory();
  auto expr = problem.makeExpression(0);
  BENCHMARK_SUSPEND {
    // create a large expression which is a sum of 1000 variables
    constexpr int exprLength = 500;
    for (const auto i : folly::irange(exprLength)) {
      expr += problem.makeIntVar(fmt::format("var_{}", i));
    }
  }

  const int addCount = 1e6;
  Expression sum;
  for (const auto i : folly::irange(addCount)) {
    const auto& c = i + expr;
    BENCHMARK_SUSPEND {
      sum += c;
    }
  }
}

void runMakeSimpleRelations(const std::function<Problem()>& factory) {
  auto problem = factory();
  auto expr = problem.makeExpression(0);
  BENCHMARK_SUSPEND {
    constexpr int exprLength = 100;
    for (const auto i : folly::irange(exprLength)) {
      expr += problem.makeIntVar(fmt::format("var_{}", i));
    }
  }

  const int nRelations = 5e5;
  for (const auto _ : folly::irange(nRelations)) {
    const auto& exprLessZero = Relation(expr <= 0);
    const auto& zeroLessExpr = Relation(0 <= expr);
    const auto& exprEq0 = Relation(expr == 0);
    const auto& zeroEqExpr = Relation(0 == expr);
    const auto& exprGreaterZero = Relation(expr >= 0);
    const auto& zeroGreaterExpr = Relation(0 >= expr);
  }
}

// Add term benchmarks
BENCHMARK(AddTermGurobi) {
  runAddTerm(ProblemFactory::makeGurobiProblem);
}
BENCHMARK(AddTermXpress) {
  runAddTerm(ProblemFactory::makeXpressProblem);
}
BENCHMARK(AddTermGeneric) {
  runAddTerm(makeGenericGurobiProblem);
}

// Add expression benchmarks
BENCHMARK(AddExpressionGurobi) {
  runAddExpression(ProblemFactory::makeGurobiProblem);
}
BENCHMARK(AddExpressionXpress) {
  runAddExpression(ProblemFactory::makeXpressProblem);
}
BENCHMARK(AddExpressionGeneric) {
  runAddExpression(makeGenericGurobiProblem);
}

// Add constant expression benchmark
BENCHMARK(AddConstantExpressionGurobi) {
  runAddConstantExpression(ProblemFactory::makeGurobiProblem);
}
BENCHMARK(AddConstantExpressionXpress) {
  runAddConstantExpression(ProblemFactory::makeXpressProblem);
}
BENCHMARK(AddConstantExpressionGeneric) {
  runAddConstantExpression(makeGenericGurobiProblem);
}

// adding a long expr and a constant benchmarks
BENCHMARK(AddConstantLongExprGurobi) {
  runAddConstantWithLongExpr(ProblemFactory::makeGurobiProblem);
}
BENCHMARK(AddConstantLongExprXpress) {
  runAddConstantWithLongExpr(ProblemFactory::makeXpressProblem);
}
BENCHMARK(AddConstantLongExprGeneric) {
  runAddConstantWithLongExpr(makeGenericGurobiProblem);
}

// multiplying a long expr and a constant benchmarks
BENCHMARK(MultiplyConstantLongExprGurobi) {
  runMultiplyConstantWithLongExpr(ProblemFactory::makeGurobiProblem);
}
BENCHMARK(MultiplyConstantLongExprXpress) {
  runMultiplyConstantWithLongExpr(ProblemFactory::makeXpressProblem);
}
BENCHMARK(MultiplyConstantLongExprGeneric) {
  runMultiplyConstantWithLongExpr(makeGenericGurobiProblem);
}

// make simple relations benchmarks
BENCHMARK(MakeSimpleRelationsGurobi) {
  runMakeSimpleRelations(ProblemFactory::makeGurobiProblem);
}
BENCHMARK(MakeSimpleRelationsXpress) {
  runMakeSimpleRelations(ProblemFactory::makeXpressProblem);
}
BENCHMARK(MakeSimpleRelationsGeneric) {
  runMakeSimpleRelations(makeGenericGurobiProblem);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
