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

#include "algopt/rebalancer/algopt_common/AssociativeVector.h"

#include <folly/container/irange.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <random>

using namespace std;
using namespace facebook::algopt;

int sum(const vector<int>& v) {
  return std::accumulate(v.begin(), v.end(), 0);
}

TEST(AssociativeVectorTest, Sum) {
  SumVector<int> sum;
  sum.init({1, 3, 2, 7, 9});
  EXPECT_EQ(22, sum.query());
  EXPECT_EQ(22, sum.query(0, 5));
  EXPECT_EQ(12, sum.query(1, 4));
  sum.update(2, 4);
  EXPECT_EQ(24, sum.query());
  EXPECT_EQ(4, sum.query(0, 2));
  EXPECT_EQ(7, sum.query(1, 3));
}

TEST(AssociativeVectorTest, Max) {
  MaxVector<double> maxc({1.0, 2.5, 3.1, 0.5});
  EXPECT_EQ(3.1, maxc.query());
  EXPECT_EQ(2.5, maxc.query(1, 2));
  EXPECT_EQ(3.1, maxc.query(1, 4));
  maxc.update(1, 4.2);
  EXPECT_EQ(4.2, maxc.query());
  EXPECT_EQ(3.1, maxc.query(2, 4));
  maxc.update(1, -4.2);
  EXPECT_EQ(1.0, maxc.query(0, 2));
}

TEST(AssociativeVectorTest, SumStress) {
  std::mt19937 gen;
  std::uniform_int_distribution<> sz_gen(1, 100);
  std::uniform_int_distribution<> value_gen(-100, 100);

  for (const auto _ : folly::irange(10)) {
    const int sz = sz_gen(gen);
    std::uniform_int_distribution<> index_gen(0, sz - 1);
    std::vector<int> regular_vector(sz);
    for (const auto i : folly::irange(sz)) {
      regular_vector[i] = value_gen(gen);
    }
    SumVector<int> sum_vector(regular_vector);
    EXPECT_EQ(sum(regular_vector), sum_vector.query());

    for (const auto _ : folly::irange(1000)) {
      const int index = index_gen(gen);
      const int value = value_gen(gen);
      regular_vector[index] = value;
      sum_vector.update(index, value);
      EXPECT_EQ(sum(regular_vector), sum_vector.query());
    }
  }
}
