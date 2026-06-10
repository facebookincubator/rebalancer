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

#include "algopt/rebalancer/solver/iterators/Filter.h"

#include <gtest/gtest.h>

using namespace std;
using namespace facebook::rebalancer;

TEST(FilterTest, Empty) {
  const vector<int> input = {};
  const Filter<vector<int>> filter(input, [](int) { return true; });
  const vector<int> output(filter.begin(), filter.end());
  EXPECT_EQ(vector<int>(), output);
}

TEST(FilterTest, KeepAll) {
  const vector<int> input = {1, 2, 3};
  const Filter<vector<int>> filter(input, [](int) { return true; });
  const vector<int> output(filter.begin(), filter.end());
  EXPECT_EQ(vector<int>({1, 2, 3}), output);
}

TEST(FilterTest, RemoveAll) {
  const vector<int> input = {1, 2, 3};
  const Filter<vector<int>> filter(input, [](int) { return false; });
  const vector<int> output(filter.begin(), filter.end());
  EXPECT_EQ(vector<int>(), output);
}

TEST(FilterTest, KeepOdd) {
  const vector<int> input = {1, 2, 3, 4, 5};
  const Filter<vector<int>> filter(input, [](int n) { return n % 2 == 1; });
  const vector<int> output(filter.begin(), filter.end());
  EXPECT_EQ(vector<int>({1, 3, 5}), output);
}

TEST(FilterTest, KeepEven) {
  const vector<int> input = {1, 2, 3, 4, 5};
  const Filter<vector<int>> filter(input, [](int n) { return n % 2 == 0; });
  const vector<int> output(filter.begin(), filter.end());
  EXPECT_EQ(vector<int>({2, 4}), output);
}

TEST(FilterTest, PartialCollection) {
  vector<int> input = {1, 2, 3, 4, 5};
  const Filter<vector<int>> filter(
      input.begin() + 2, input.end(), [](int n) { return n % 2 == 1; });
  const vector<int> output(filter.begin(), filter.end());
  EXPECT_EQ(vector<int>({3, 5}), output);
}
