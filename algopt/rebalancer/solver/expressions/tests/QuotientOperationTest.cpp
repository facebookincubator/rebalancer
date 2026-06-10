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
#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class QuotientOperationTest : public ExpressionTestsBase {
 protected:
  void SetUp() override {
    setInitialAssignment(
        entities::Map<std::string, std::vector<std::string>>{
            {"container0", {}}, {"container1", {"object1"}}});
  }
};

TEST_F(QuotientOperationTest, LpExactWhenQuotientInUnitInterval) {
  // a/b = 1 / (0 + 2) = 0.5, inside the [0, 1] range that lp() can
  // represent.
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto x = variable(object(1), container(0), universe, assignment);
  auto q = quotient(const_expr(1, universe), x + 2, universe);

  EXPECT_NEAR(0.5, apply(q, assignment), 1e-8);
}

TEST_F(QuotientOperationTest, LpInfeasibleWhenQuotientExceedsOne) {
  // a/b = 3 / (0 + 2) = 1.5 is outside [0, 1]; the expression's apply
  // still gets the exact value, but lp()'s relaxation hardcodes the
  // quotient's bound to [0, 1] (see `approximate_quotient_log` in
  // QuotientOperation.cpp) and the MIP comes back infeasible.
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto x = variable(object(1), container(0), universe, assignment);
  auto q = quotient(const_expr(3, universe), x + 2, universe);

  LpAssertOptions lpAssertOptions{
      .exceptionForLpExpr =
          "LP problem has no solution (infeasible or unbounded)"};
  EXPECT_NEAR(1.5, apply(q, assignment, lpAssertOptions), 1e-8);
}

} // namespace facebook::rebalancer::packer::tests
