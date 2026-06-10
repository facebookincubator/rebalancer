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

#ifdef REBALANCER_USE_HIGHS

#include "algopt/lp/detail/highs/HiGHSProblem.h"
#include "algopt/lp/generic/Operators.h"

#include <gtest/gtest.h>

using namespace facebook::algopt::lp;
using namespace facebook::algopt::lp::detail;

TEST(HiGHSTest, SimpleTest) {
  auto model = Problem(std::make_unique<HiGHSProblem>());
  auto x = model.makeVar("x");
  auto y = model.makeVar("y");
  model.newConstraint(x <= 0, "xneg");
  model.newConstraint(y <= 0, "yneg");
  model.newConstraint(5 * x + 2 * y >= -45, "con1");
  model.newConstraint(x - y <= 3, "con2");
  model.newConstraint(x + 4 * y >= -27, "con3");
  model.setObjective(x + y);
  model.solve();
  EXPECT_EQ(model.getStatus(), thrift::ProblemStatus::OPTIMAL_FOUND);
  EXPECT_EQ(x.getValue(), -7);
  EXPECT_EQ(y.getValue(), -5);

  const auto attrib = model.getProblemAttributes();
  EXPECT_EQ(attrib.numVariables(), 2);
  EXPECT_EQ(attrib.numConstraints(), 5);
}

TEST(HiGHSTest, MipTest) {
  auto model = Problem(std::make_unique<HiGHSProblem>());
  auto x = model.makeIntVar("x");
  auto y = model.makeIntVar("y");
  model.newConstraint(x + 7 * y <= 17.5);
  model.newConstraint(0 <= x);
  model.newConstraint(0 <= y);
  model.newConstraint(x <= 3.5);
  model.setObjective(-x - 10 * y);
  model.solve();
  EXPECT_EQ(model.getStatus(), thrift::ProblemStatus::OPTIMAL_FOUND);
  EXPECT_EQ(x.getValue(), 3);
  EXPECT_EQ(y.getValue(), 2);

  const auto attrib = model.getProblemAttributes();
  EXPECT_EQ(attrib.numVariables(), 2);
  EXPECT_EQ(attrib.numConstraints(), 4);
}

#endif
