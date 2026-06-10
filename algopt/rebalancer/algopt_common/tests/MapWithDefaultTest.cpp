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

#include "algopt/rebalancer/algopt_common/MapWithDefault.h"

#include "folly/container/F14Map.h"
#include <gtest/gtest.h>

namespace facebook::algopt::tests {

TEST(MapWithDefaultTest, BasicWithDefault) {
  MapWithDefault<folly::F14FastMap<std::string, int>> map(44);
  EXPECT_EQ(map.at("one"), 44);
  EXPECT_EQ(*map.getPtr("two"), 44);
  EXPECT_EQ(map["one"], 44); // default value is returned; consistent with at()
  EXPECT_EQ(map["three"], 44); // default value is returned
}

TEST(MapWithDefaultTest, BasicWithMapConstructor) {
  MapWithDefault<folly::F14FastMap<std::string, int>> map({{"one", 1}});
  EXPECT_EQ(map.at("one"), 1);
  EXPECT_EQ(map.getPtr("two"), nullptr);
  EXPECT_EQ(
      map["three"], 0); // default constructed since no default value is set
}

TEST(MapWithDefaultTest, BasicWithMapAndDefaultConstructor) {
  MapWithDefault<folly::F14FastMap<std::string, int>> map(
      {{"one", 1}, {"four", 4}}, 67);
  EXPECT_EQ(map.at("one"), 1);
  EXPECT_EQ(*map.getPtr("two"), 67);
  EXPECT_EQ(map.at("three"), 67);
  EXPECT_EQ(map.at("four"), 4);
  EXPECT_EQ(map["five"], 67);

  map["five"] = 32;
  EXPECT_EQ(map.at("five"), 32);
}

TEST(MapWithDefaultTest, VectorValue) {
  MapWithDefault<folly::F14FastMap<std::string, std::vector<int>>> map(
      {{"one", {1, 2}}}, {100, 200, 300});

  {
    const std::vector<int> oneValue = {1, 2};
    EXPECT_TRUE(map.at("one") == oneValue);
  }

  // modify "one"
  {
    const std::vector<int> oneValue = {1, 2, 3};
    map["one"].push_back(3);
    EXPECT_TRUE(map.at("one") == oneValue);
  }

  {
    const std::vector<int> fourValue = {100, 200, 300};
    EXPECT_TRUE(map.at("four") == fourValue);
  }

  // modify "four"
  {
    map["four"].push_back(-1);
    const std::vector<int> fourValue = {100, 200, 300, -1};
    EXPECT_TRUE(map["four"] == fourValue);
    EXPECT_TRUE(map.at("four") == fourValue);
  }
}

} // namespace facebook::algopt::tests
