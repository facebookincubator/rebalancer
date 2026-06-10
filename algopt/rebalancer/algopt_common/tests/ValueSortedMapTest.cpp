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

#include "algopt/rebalancer/algopt_common/ValueSortedMap.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace facebook::algopt;

class TempClass {
 public:
  explicit TempClass(const std::string& s) : myString(s) {}

  const std::string& myString;
};

TEST(ValueSortedMapTest, Basic) {
  struct compare {
    bool operator()(const pair<string, int>& lhs, const pair<string, int>& rhs)
        const {
      if (lhs.second != rhs.second) {
        return lhs.second > rhs.second;
      }
      return lhs.first < rhs.first;
    }
  };
  ValueSortedMap<string, int, compare> vs;
  vs.assign("a", 8);
  vs.assign("b", 6);
  vs.assign("c", 10);
  ASSERT_EQ(3, vs.size());
  EXPECT_EQ(6, vs.at("b"));
  EXPECT_EQ(6, *vs.getPtrIfExists("b"));
  vector<pair<string, int>> expected({{"c", 10}, {"a", 8}, {"b", 6}});
  vector<pair<string, int>> actual(vs.begin(), vs.end());
  EXPECT_EQ(expected, actual);
  vs.assign("b", 10);
  ASSERT_EQ(3, vs.size());
  EXPECT_EQ(10, vs.at("b"));
  expected = {{"b", 10}, {"c", 10}, {"a", 8}};
  actual = vector<pair<string, int>>(vs.begin(), vs.end());
  EXPECT_EQ(expected, actual);
  vs.erase("b");
  ASSERT_EQ(2, vs.size());
  expected = {{"c", 10}, {"a", 8}};
  actual = vector<pair<string, int>>(vs.begin(), vs.end());
  EXPECT_EQ(expected, actual);
  vs.clear();
  ASSERT_EQ(0, vs.size());
  EXPECT_TRUE(vs.begin() == vs.end());
  EXPECT_TRUE(nullptr == vs.getPtrIfExists("dummy"));

  vs.assign("a", 8);
  ValueSortedMap<string, int, compare> vs2;
  vs2.assign("a_copy", 8);
  EXPECT_NE(vs, vs2);

  vs2.clear();
  vs2.assign("a", 8);
  EXPECT_EQ(vs, vs2);
}

TEST(ValueSortedMapTest, ObjectsWithNoDefaultConstructor) {
  struct compare {
    bool operator()(
        const pair<string, TempClass>& lhs,
        const pair<string, TempClass>& rhs) const {
      if (lhs.second.myString != rhs.second.myString) {
        return lhs.second.myString > rhs.second.myString;
      }
      return lhs.first < rhs.first;
    }
  };
  ValueSortedMap<string, TempClass, compare> vs;

  const std::string string1 = "hello";
  const std::string string2 = "world";

  auto a = TempClass(string1);
  auto b = TempClass(string2);
  vs.assign("first", a);
  vs.assign("second", b);

  EXPECT_EQ(2, vs.size());
  EXPECT_EQ(string1, vs.at("first").myString);
  EXPECT_EQ(string2, vs.at("second").myString);
}
