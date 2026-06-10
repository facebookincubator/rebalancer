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

#include "algopt/rebalancer/algopt_common/Precision.h"

#include <gtest/gtest.h>

namespace facebook::algopt::tests {

TEST(PrecisionTest, CompareLeq) {
  constexpr double num1 = -3;
  constexpr double num2 = 5;
  EXPECT_EQ(-1, Precision::compare(num1, num2));
  EXPECT_FALSE(Precision::isEqual(num1, num2));
}

TEST(PrecisionTest, CompareGeq) {
  constexpr double num1 = 3;
  constexpr double num2 = -5;
  EXPECT_EQ(1, Precision::compare(num1, num2));
  EXPECT_FALSE(Precision::isEqual(num1, num2));
}

TEST(PrecisionTest, CompareEqEpsilon) {
  constexpr double num1 = 3;
  const double num2 = 3 - 1e-11;
  EXPECT_EQ(0, Precision::compare(num1, num2));
  EXPECT_TRUE(Precision::isEqual(num1, num2));
}

TEST(PrecisionTest, CompareEq) {
  constexpr double num1 = 3;
  constexpr double num2 = 3;
  EXPECT_EQ(0, Precision::compare(num1, num2));
  EXPECT_TRUE(Precision::isEqual(num1, num2));
}

TEST(PrecisionTest, TrimCloseToZero) {
  constexpr double num = -1e-11;
  EXPECT_EQ(0, Precision::trim(num));
}

TEST(PrecisionTest, TrimNotCloseToZero) {
  const double num = 3 - 1e-11;
  EXPECT_EQ(num, Precision::trim(num));
}

TEST(PrecisionTest, IsIntegerTrue) {
  const double num = 3 - 1e-11;
  EXPECT_EQ(true, Precision::isInteger(num));
}

TEST(PrecisionTest, IsIntegerFalse) {
  constexpr double num = 3.14;
  EXPECT_EQ(false, Precision::isInteger(num));
}

TEST(PrecisionTest, IsZeroOrOneTrue) {
  const double num = 1 - 1e-11;
  EXPECT_EQ(true, Precision::isZeroOrOne(num));
  EXPECT_TRUE(Precision::isEqual(num, 1.0));
  EXPECT_FALSE(Precision::isEqual(num, 0.0));
}

TEST(PrecisionTest, IsZeroOrOneFalse) {
  constexpr double num = 0.3;
  EXPECT_EQ(false, Precision::isZeroOrOne(num));
}

TEST(PrecisionTest, Snap) {
  {
    constexpr double num = 1.3;
    EXPECT_NEAR(1.3, Precision::snap(num, 1e-4), 1e-8);
  }
  {
    constexpr double num = 0.99992;
    EXPECT_NEAR(1.0, Precision::snap(num, 1e-4), 1e-8);
  }
  {
    constexpr double num = 3.00009;
    EXPECT_NEAR(3.0, Precision::snap(num, 1e-4), 1e-8);
  }
}

TEST(PrecisionTest, IsStrictlyGreater) {
  constexpr double greater = 10.0;
  constexpr double lesser = 5.0;
  EXPECT_TRUE(Precision::isstrictlyGreater(greater, lesser));
  EXPECT_FALSE(Precision::isstrictlyGreater(lesser, greater));

  // Test with equal values
  constexpr double equal1 = 7.0;
  constexpr double equal2 = 7.0;
  EXPECT_FALSE(Precision::isstrictlyGreater(equal1, equal2));

  // Test with values that are very close
  const double almostEqual1 = 7.0;
  const double almostEqual2 = 7.0 - kEpsilon / 10;
  EXPECT_FALSE(Precision::isstrictlyGreater(almostEqual1, almostEqual2));
  EXPECT_FALSE(Precision::isstrictlyGreater(almostEqual2, almostEqual1));
}

TEST(PrecisionTest, IsStrictlyLesser) {
  constexpr double greater = 10.0;
  constexpr double lesser = 5.0;
  EXPECT_TRUE(Precision::isStrictlyLesser(lesser, greater));
  EXPECT_FALSE(Precision::isStrictlyLesser(greater, lesser));

  // Test with equal values
  constexpr double equal1 = 7.0;
  constexpr double equal2 = 7.0;
  EXPECT_FALSE(Precision::isStrictlyLesser(equal1, equal2));

  // Test with values that are very close
  const double almostEqual1 = 7.0;
  const double almostEqual2 = 7.0 - kEpsilon / 10;
  EXPECT_FALSE(Precision::isStrictlyLesser(almostEqual1, almostEqual2));
  EXPECT_FALSE(Precision::isStrictlyLesser(almostEqual2, almostEqual1));
}

TEST(PrecisionTest, IsLesserOrEqual) {
  constexpr double greater = 10.0;
  constexpr double lesser = 5.0;
  EXPECT_TRUE(Precision::isLesserOrEqual(lesser, greater));
  EXPECT_FALSE(Precision::isLesserOrEqual(greater, lesser));

  // Test with equal values
  constexpr double equal1 = 7.0;
  constexpr double equal2 = 7.0;
  EXPECT_TRUE(Precision::isLesserOrEqual(equal1, equal2));

  // Test with values that are very close
  const double almostEqual1 = 7.0;
  const double almostEqual2 = 7.0 - kEpsilon / 10;
  EXPECT_TRUE(Precision::isLesserOrEqual(almostEqual1, almostEqual2));
  EXPECT_TRUE(Precision::isLesserOrEqual(almostEqual2, almostEqual1));
}

TEST(PrecisionTest, IsGreaterOrEqual) {
  constexpr double greater = 10.0;
  constexpr double lesser = 5.0;
  EXPECT_TRUE(Precision::isGreaterOrEqual(greater, lesser));
  EXPECT_FALSE(Precision::isGreaterOrEqual(lesser, greater));

  // Test with equal values
  constexpr double equal1 = 7.0;
  constexpr double equal2 = 7.0;
  EXPECT_TRUE(Precision::isGreaterOrEqual(equal1, equal2));

  // Test with values that are very close
  const double almostEqual1 = 7.0;
  const double almostEqual2 = 7.0 - kEpsilon / 10;
  EXPECT_TRUE(Precision::isGreaterOrEqual(almostEqual1, almostEqual2));
  EXPECT_TRUE(Precision::isGreaterOrEqual(almostEqual2, almostEqual1));
}

TEST(PrecisionTest, IsStrictlyGtZero) {
  // Test clearly positive value
  constexpr double positive = 5.0;
  EXPECT_TRUE(Precision::isStrictlyGtZero(positive));

  // Test negative value
  constexpr double negative = -3.0;
  EXPECT_FALSE(Precision::isStrictlyGtZero(negative));

  // Test zero exactly
  constexpr double zero = 0.0;
  EXPECT_FALSE(Precision::isStrictlyGtZero(zero));

  // Test small positive value greater than epsilon
  const double smallPositive = kEpsilon * 10;
  EXPECT_TRUE(Precision::isStrictlyGtZero(smallPositive));

  // Test very small positive value within epsilon tolerance
  const double tinyPositive = kEpsilon / 10;
  EXPECT_FALSE(Precision::isStrictlyGtZero(tinyPositive));
}

} // namespace facebook::algopt::tests
