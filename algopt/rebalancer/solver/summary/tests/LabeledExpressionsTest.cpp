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

#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/summary/LabeledConstraints.h"
#include "algopt/rebalancer/solver/summary/LabeledObjectives.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"

#include <gtest/gtest.h>

using namespace facebook::rebalancer;
using namespace std;

TEST(LabeledExpressionsTest, LabeledConstraintsSummary) {
  auto universe = std::make_shared<const entities::Universe>();
  auto expr1 = const_expr(-2, universe);
  auto expr2 = const_expr(0, universe);
  auto expr3 = const_expr(2, universe);
  auto expr4 = max({expr1, expr2, expr3}, universe);

  Context context;
  expr4->fullApply(TopToBottomEvaluator(context), Assignment());

  LabeledConstraints labeled_constraints;
  labeled_constraints.setRoot(expr4);
  labeled_constraints.addSingle("expression-1", expr1);
  labeled_constraints.addSingle("expression-2", expr2);
  labeled_constraints.addSingle("expression-3", expr3);

  auto summary = labeled_constraints.getSummary();
  EXPECT_EQ(2, *summary.brokenVal());
  EXPECT_EQ(1, *summary.brokenCount());
  EXPECT_EQ("expression-1", *summary.constraints()[0].name());
  EXPECT_EQ("", *summary.constraints()[0].desc());
  EXPECT_EQ(-2, *summary.constraints()[0].value());
  EXPECT_EQ("expression-2", *summary.constraints()[1].name());
  EXPECT_EQ("", *summary.constraints()[1].desc());
  EXPECT_EQ(0, *summary.constraints()[1].value());
  EXPECT_EQ("expression-3", *summary.constraints()[2].name());
  EXPECT_EQ("", *summary.constraints()[2].desc());
  EXPECT_EQ(2, *summary.constraints()[2].value());
}

TEST(LabeledExpressionsTest, LabeledObjectivesSummary) {
  auto universe = std::make_shared<const entities::Universe>();
  auto expr1 = const_expr(2, universe);
  auto expr2 = const_expr(8, universe);
  auto expr3 = expr1 + 4 * expr2;

  Context context;
  expr3->fullApply(TopToBottomEvaluator(context), Assignment());

  LabeledObjectives labeled_objectives;
  labeled_objectives.setRoot(expr3);
  labeled_objectives.addSingle("expression-1", expr1);
  labeled_objectives.addSingle("expression-2", expr2, 4);

  auto summary = labeled_objectives.getSummary();
  EXPECT_EQ(34, *summary.value());
  ASSERT_EQ(2, summary.objs()->size());
  EXPECT_EQ("expression-1", *summary.objs()[0].name());
  EXPECT_EQ("", *summary.objs()[0].desc());
  EXPECT_EQ(1, *summary.objs()[0].weight());
  EXPECT_EQ(2, *summary.objs()[0].value());
  EXPECT_EQ("expression-2", *summary.objs()[1].name());
  EXPECT_EQ("", *summary.objs()[1].desc());
  EXPECT_EQ(4, *summary.objs()[1].weight());
  EXPECT_EQ(8, *summary.objs()[1].value());
}

TEST(LabeledExpressionsTest, Iterator) {
  auto universe = std::make_shared<const entities::Universe>();
  auto expr1 = const_expr(1, universe);
  auto expr2 = const_expr(2, universe);
  auto expr3 = const_expr(3, universe);

  LabeledObjectives labeled_objectives;
  labeled_objectives.addSingle("expression-1", expr1);
  labeled_objectives.addSingle("expression-2", expr2, 5);
  labeled_objectives.addSingle("expression-3", expr3);

  vector<LabeledExpressionPtr> expressions(
      labeled_objectives.begin(), labeled_objectives.end());
  ASSERT_EQ(3, expressions.size());
  EXPECT_EQ("expression-1", expressions[0]->name);
  EXPECT_EQ(expr1, expressions[0]->expression);
  EXPECT_EQ(1, expressions[0]->weight);
  EXPECT_EQ("expression-2", expressions[1]->name);
  EXPECT_EQ(expr2, expressions[1]->expression);
  EXPECT_EQ(5, expressions[1]->weight);
  EXPECT_EQ("expression-3", expressions[2]->name);
  EXPECT_EQ(expr3, expressions[2]->expression);
  EXPECT_EQ(1, expressions[2]->weight);
}
