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

#include "algopt/rebalancer/algopt_common/AssociativeMap.h"

#include <folly/container/irange.h>
#include <gtest/gtest.h>

#include <map>
#include <random>
#include <string>

using namespace std;
using namespace facebook::algopt;

int sum_values(const map<int, int>& m) {
  int result = 0;
  for (auto& p : m) {
    result += p.second;
  }
  return result;
}

TEST(AssociativeMapTest, Sum) {
  SumMap<string, int> sum;
  EXPECT_EQ(0, sum.query());
  sum.update("a", 10);
  sum.update("b", 20);
  sum.update("c", 40);
  EXPECT_EQ(70, sum.query());

  EXPECT_EQ(10, sum.getValueIfExists("a").value());
  EXPECT_EQ(20, sum.getValueIfExists("b").value());
  EXPECT_EQ(40, sum.getValueIfExists("c").value());
  EXPECT_FALSE(sum.getValueIfExists("1a").has_value());
  EXPECT_FALSE(sum.getValueIfExists("d").has_value());

  sum.update("b", 21);
  EXPECT_EQ(71, sum.query());
  EXPECT_EQ(21, sum.getValueIfExists("b").value());

  sum.remove("a");
  EXPECT_EQ(61, sum.query());
  EXPECT_FALSE(sum.getValueIfExists("a").has_value());

  sum.update("e", 160);
  sum.update("d", 80);
  EXPECT_EQ(301, sum.query());
  EXPECT_EQ(160, sum.getValueIfExists("e").value());
  EXPECT_EQ(80, sum.getValueIfExists("d").value());
}

TEST(AssociativeMapTest, SumStress) {
  std::mt19937 gen;
  std::uniform_int_distribution<> op_gen(0, 1);
  std::uniform_int_distribution<> key_gen(-100, 100);
  std::uniform_int_distribution<> value_gen(-100, 100);

  for (const auto _ : folly::irange(10)) {
    SumMap<int, int> sum_map;
    map<int, int> regular_map;
    for (const auto _ : folly::irange(1000)) {
      const int op = op_gen(gen);
      const int key = key_gen(gen);
      if (op == 0) { // update
        const int value = value_gen(gen);
        sum_map.update(key, value);
        regular_map[key] = value;
      } else { // remove
        sum_map.remove(key);
        regular_map.erase(key);
      }
      const int expected = sum_values(regular_map);
      EXPECT_EQ(expected, sum_map.query());
    }
  }
}

TEST(AssociativeMapTest, QueryLessThanOrEqual) {
  SumMap<string, int> sum;
  sum.update("a", 10);
  sum.update("b", 20);
  sum.update("d", 40);
  sum.update("e", 80);
  EXPECT_EQ(0, sum.query_lt(""));
  EXPECT_EQ(0, sum.query_le(""));
  EXPECT_EQ(0, sum.query_lt("a"));
  EXPECT_EQ(10, sum.query_le("a"));
  EXPECT_EQ(10, sum.query_lt("b"));
  EXPECT_EQ(30, sum.query_le("b"));
  EXPECT_EQ(30, sum.query_lt("c"));
  EXPECT_EQ(30, sum.query_le("c"));
  EXPECT_EQ(30, sum.query_lt("d"));
  EXPECT_EQ(70, sum.query_le("d"));
  EXPECT_EQ(70, sum.query_lt("e"));
  EXPECT_EQ(150, sum.query_le("e"));
  EXPECT_EQ(150, sum.query_lt("z"));
  EXPECT_EQ(150, sum.query_le("z"));
}
