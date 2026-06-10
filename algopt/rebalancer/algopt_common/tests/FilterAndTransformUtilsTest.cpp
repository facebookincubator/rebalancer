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

#include "algopt/rebalancer/algopt_common/FilterAndTransformUtils.h"

#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::algopt::common;

TEST(FilterAndTransformUtilsTest, defaultTransform) {
  // From a list of string names, select a subset of names using
  // StringListFilterConfig (with allowlist or blocklist)
  const std::vector<std::string> original = {"1", "2", "3", "4", "5"};

  thrift::StringListFilterConfig allowlistConfig;
  allowlistConfig.stringsToFilter() = {"1", "2", "3"};
  allowlistConfig.filterType() = thrift::FilterType::ALLOWLIST;

  thrift::StringListFilterConfig blocklistConfig;
  blocklistConfig.stringsToFilter() = {"4", "5"};
  blocklistConfig.filterType() = thrift::FilterType::BLOCKLIST;

  EXPECT_EQ(
      std::vector<std::string>({"1", "2", "3"}),
      FilterAndTransformUtils::selectAllowedItems<std::string>(
          original, allowlistConfig));
  EXPECT_EQ(
      std::vector<std::string>({"1", "2", "3"}),
      FilterAndTransformUtils::selectAllowedItems<std::string>(
          original, blocklistConfig));

  // test with empty allowlist and blocklist
  allowlistConfig.stringsToFilter() = {};
  blocklistConfig.stringsToFilter() = {};

  EXPECT_TRUE(
      FilterAndTransformUtils::selectAllowedItems<std::string>(
          original, allowlistConfig)
          .empty());
  EXPECT_EQ(
      original,
      FilterAndTransformUtils::selectAllowedItems<std::string>(
          original, blocklistConfig));
}

TEST(FilterAndTransformUtilsTest, customTransform) {
  // From a list of string ids, and a function to map names to ids, select a
  // subset of ids using StringListFilterConfig (with allowlist or blocklist of
  // names)
  const std::vector<int> original = {1, 2, 3, 4, 5};
  auto getIdFromName = [](const std::string& s) { return std::stoi(s); };

  thrift::StringListFilterConfig allowlistConfig;
  allowlistConfig.stringsToFilter() = {"1", "2", "3"};
  allowlistConfig.filterType() = thrift::FilterType::ALLOWLIST;

  thrift::StringListFilterConfig blocklistConfig;
  blocklistConfig.stringsToFilter() = {"4", "5"};
  blocklistConfig.filterType() = thrift::FilterType::BLOCKLIST;

  EXPECT_EQ(
      std::vector<int>({1, 2, 3}),
      FilterAndTransformUtils::selectAllowedItems<int>(
          original, allowlistConfig, getIdFromName));
  EXPECT_EQ(
      std::vector<int>({1, 2, 3}),
      FilterAndTransformUtils::selectAllowedItems<int>(
          original, blocklistConfig, getIdFromName));

  // Test with empty allowlist and blocklist
  allowlistConfig.stringsToFilter() = {};
  blocklistConfig.stringsToFilter() = {};

  EXPECT_TRUE(
      FilterAndTransformUtils::selectAllowedItems<int>(
          original, allowlistConfig, getIdFromName)
          .empty());
  EXPECT_EQ(
      original,
      FilterAndTransformUtils::selectAllowedItems<int>(
          original, blocklistConfig, getIdFromName));
}
