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

#include "algopt/rebalancer/algopt_common/TestUtils.h"
#include "algopt/rebalancer/algopt_common/thrift/ThriftUtils.h"

#include <gtest/gtest.h>

namespace facebook::algopt::common::thriftUtils {
namespace {

algopt::common::thrift::Threshold makeThreshold(
    std::optional<double> percent = std::nullopt,
    std::optional<double> absolute = std::nullopt) {
  algopt::common::thrift::Threshold threshold;
  if (percent.has_value()) {
    threshold.percent() = *percent;
  }
  if (absolute.has_value()) {
    threshold.absolute() = *absolute;
  }
  return threshold;
}

TEST(IsSignificantDecreaseTest, NeitherThresholdSet) {
  const auto threshold = makeThreshold();
  REBALANCER_EXPECT_RUNTIME_ERROR(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/50.0),
      "Threshold must have at least one of percent or absolute set");
}

TEST(IsSignificantDecreaseTest, NoDecrease) {
  const auto threshold = makeThreshold(/*percent=*/1.0, /*absolute=*/1.0);
  EXPECT_FALSE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/100.0));
  EXPECT_FALSE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/110.0));
}

TEST(IsSignificantDecreaseTest, AbsoluteOnly) {
  const auto threshold =
      makeThreshold(/*percent=*/std::nullopt, /*absolute=*/5.0);
  EXPECT_TRUE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/90.0));
  EXPECT_FALSE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/97.0));
  EXPECT_FALSE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/95.0));
}

TEST(IsSignificantDecreaseTest, PercentOnly) {
  // 10.0 means 10%
  const auto threshold = makeThreshold(/*percent=*/10.0);
  EXPECT_TRUE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/80.0));
  EXPECT_FALSE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/95.0));
  EXPECT_FALSE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/90.0));
}

TEST(IsSignificantDecreaseTest, BothSetOrSemantics) {
  // A decrease of 10 exceeds absolute (10 > 5) but not percent (10% < 50%)
  const auto threshold = makeThreshold(/*percent=*/50.0, /*absolute=*/5.0);
  EXPECT_TRUE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/90.0));

  // A decrease of 10 exceeds percent (10% > 1%) but not absolute (10 < 50)
  const auto threshold2 = makeThreshold(/*percent=*/1.0, /*absolute=*/50.0);
  EXPECT_TRUE(
      isSignificantDecrease(threshold2, /*before=*/100.0, /*after=*/90.0));

  // Neither exceeded
  const auto threshold3 = makeThreshold(/*percent=*/50.0, /*absolute=*/50.0);
  EXPECT_FALSE(
      isSignificantDecrease(threshold3, /*before=*/100.0, /*after=*/90.0));
}

TEST(IsSignificantDecreaseTest, ZeroBaselinePercentOnly) {
  // Any decrease from zero is significant (percent is undefined at zero)
  const auto threshold = makeThreshold(/*percent=*/10.0);
  EXPECT_TRUE(isSignificantDecrease(threshold, /*before=*/0.0, /*after=*/-5.0));
}

TEST(IsSignificantDecreaseTest, ZeroBaselineAbsolute) {
  const auto threshold =
      makeThreshold(/*percent=*/std::nullopt, /*absolute=*/3.0);
  EXPECT_TRUE(isSignificantDecrease(threshold, /*before=*/0.0, /*after=*/-5.0));
  EXPECT_FALSE(
      isSignificantDecrease(threshold, /*before=*/0.0, /*after=*/-1.0));
}

TEST(IsSignificantDecreaseTest, ZeroBaselineBothSet) {
  // Percent is undefined at zero; any decrease is significant (OR semantics)
  const auto threshold = makeThreshold(/*percent=*/10.0, /*absolute=*/3.0);
  EXPECT_TRUE(isSignificantDecrease(threshold, /*before=*/0.0, /*after=*/-5.0));
  EXPECT_TRUE(isSignificantDecrease(threshold, /*before=*/0.0, /*after=*/-1.0));
}

TEST(IsSignificantDecreaseTest, NegativeValues) {
  // before = -100, after = -110, decrease = 10
  const auto threshold =
      makeThreshold(/*percent=*/std::nullopt, /*absolute=*/5.0);
  EXPECT_TRUE(
      isSignificantDecrease(threshold, /*before=*/-100.0, /*after=*/-110.0));

  // before = -100, after = -102, decrease = 2
  EXPECT_FALSE(
      isSignificantDecrease(threshold, /*before=*/-100.0, /*after=*/-102.0));
}

TEST(IsSignificantDecreaseTest, NegativeValuesPercent) {
  // before = -100, after = -120, decrease = 20, |before| = 100, 20% > 10%
  const auto threshold = makeThreshold(/*percent=*/10.0);
  EXPECT_TRUE(
      isSignificantDecrease(threshold, /*before=*/-100.0, /*after=*/-120.0));

  // before = -100, after = -105, decrease = 5, 5% < 10%
  EXPECT_FALSE(
      isSignificantDecrease(threshold, /*before=*/-100.0, /*after=*/-105.0));
}

TEST(IsSignificantDecreaseTest, ExactBoundary) {
  // Exactly at threshold — not significant (strict >)
  const auto threshold =
      makeThreshold(/*percent=*/std::nullopt, /*absolute=*/10.0);
  EXPECT_FALSE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/90.0));
  EXPECT_TRUE(
      isSignificantDecrease(threshold, /*before=*/100.0, /*after=*/89.99));
}

} // namespace
} // namespace facebook::algopt::common::thriftUtils
