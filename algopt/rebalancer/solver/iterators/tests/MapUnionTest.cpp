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

#include "algopt/rebalancer/solver/iterators/MapUnion.h"

#include <gtest/gtest.h>

using namespace std;
using namespace facebook::rebalancer;

TEST(MapUnionTest, Basic) {
  map<string, int> m1 = {{"a", 10}, {"d", 40}, {"e", 50}, {"f", 70}, {"g", 80}};
  map<string, int> m2 = {
      {"b", 20}, {"c", 30}, {"e", 60}, {"g", 90}, {"h", 100}};
  const MapUnion<map<string, int>::iterator> mapUnion(
      m1.begin(), m1.end(), m2.begin(), m2.end(), 3, [](string x, string y) {
        return x < y;
      });
  const vector<tuple<string, int, int>> output(
      mapUnion.begin(), mapUnion.end());
  const vector<tuple<string, int, int>> expected = {
      {"a", 10, 3},
      {"b", 3, 20},
      {"c", 3, 30},
      {"d", 40, 3},
      {"e", 50, 60},
      {"f", 70, 3},
      {"g", 80, 90},
      {"h", 3, 100}};
  EXPECT_EQ(expected, output);
}

TEST(MapUnionTest, FirstEmpty) {
  map<string, int> m1 = {};
  map<string, int> m2 = {{"a", 10}, {"b", 20}};
  const MapUnion<map<string, int>::iterator> mapUnion(
      m1.begin(), m1.end(), m2.begin(), m2.end(), 3, [](string x, string y) {
        return x < y;
      });
  const vector<tuple<string, int, int>> output(
      mapUnion.begin(), mapUnion.end());
  const vector<tuple<string, int, int>> expected = {{"a", 3, 10}, {"b", 3, 20}};
  EXPECT_EQ(expected, output);
}

TEST(MapUnionTest, SecondEmpty) {
  map<string, int> m1 = {{"a", 10}, {"b", 20}};
  map<string, int> m2 = {};
  const MapUnion<map<string, int>::iterator> mapUnion(
      m1.begin(), m1.end(), m2.begin(), m2.end(), 3, [](string x, string y) {
        return x < y;
      });
  const vector<tuple<string, int, int>> output(
      mapUnion.begin(), mapUnion.end());
  const vector<tuple<string, int, int>> expected = {{"a", 10, 3}, {"b", 20, 3}};
  EXPECT_EQ(expected, output);
}

TEST(MapUnionTest, BothEmpty) {
  map<string, int> m1 = {};
  map<string, int> m2 = {};
  const MapUnion<map<string, int>::iterator> mapUnion(
      m1.begin(), m1.end(), m2.begin(), m2.end(), 3, [](string x, string y) {
        return x < y;
      });
  const vector<tuple<string, int, int>> output(
      mapUnion.begin(), mapUnion.end());
  const vector<tuple<string, int, int>> expected = {};
  EXPECT_EQ(expected, output);
}
