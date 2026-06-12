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

#include <cmath>
#include <limits>

namespace facebook::rebalancer::packer::tests {

class LogTest : public ExpressionTestsBase {};

TEST_F(LogTest, InitialValue) {
  setInitialAssignment(
      entities::Map<std::string, std::vector<std::string>>{
          {"container0", {"object0"}}});
  const auto universe = buildUniverse();
  const Assignment assignment(universe->getContainers().getInitialAssignment());

  auto v = variable(object(0), container(0), universe, assignment);
  // v=1, log(1) = 0
  EXPECT_DOUBLE_EQ(0.0, log(v, universe)->getInitialValue());

  // 4*v = 4, log(4)
  EXPECT_DOUBLE_EQ(std::log(4.0), log(4.0 * v, universe)->getInitialValue());

  // Non-positive input falls back to -DBL_MAX (matches Log::perform_transform).
  EXPECT_EQ(
      -std::numeric_limits<double>::max(),
      log(0.0 * v, universe)->getInitialValue());
}

} // namespace facebook::rebalancer::packer::tests
