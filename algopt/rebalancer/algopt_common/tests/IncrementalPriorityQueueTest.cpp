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

#include "algopt/rebalancer/algopt_common/IncrementalPriorityQueue.h"
#include "algopt/rebalancer/algopt_common/TestUtils.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace facebook::algopt;

TEST(IncrementalPriorityQueueTest, Basic) {
  IncrementalPriorityQueue<string> sorter;
  ASSERT_EQ(0, sorter.size());

  sorter.update({"a", "b"});
  ASSERT_EQ(2, sorter.size());
  EXPECT_FALSE(sorter.is_top_strict());
  EXPECT_EQ("a", sorter.top());

  sorter.remove("a");
  ASSERT_EQ(1, sorter.size());
  EXPECT_TRUE(sorter.is_top_strict());
  EXPECT_EQ("b", sorter.top());

  sorter.remove("b");
  ASSERT_EQ(0, sorter.size());

  sorter.update({"a"});
  ASSERT_EQ(0, sorter.size());

  sorter.update({"b", "c"});
  ASSERT_EQ(1, sorter.size());
  EXPECT_TRUE(sorter.is_top_strict());
  EXPECT_EQ("c", sorter.top());
}

TEST(IncrementalPriorityQueueTest, Ties) {
  IncrementalPriorityQueue<string> sorter;
  ASSERT_EQ(0, sorter.size());

  sorter.update({"a", "b", "c"});
  ASSERT_EQ(3, sorter.size());
  EXPECT_FALSE(sorter.is_top_strict());
  EXPECT_EQ("a", sorter.top());

  sorter.update({"b", "d", "e"});
  ASSERT_EQ(5, sorter.size());
  EXPECT_TRUE(sorter.is_top_strict());
  EXPECT_EQ("b", sorter.top());

  sorter.remove("b");
  ASSERT_EQ(4, sorter.size());
  EXPECT_FALSE(sorter.is_top_strict());
  EXPECT_EQ("a", sorter.top());

  sorter.update({"b", "c", "e", "f"});
  ASSERT_EQ(5, sorter.size());
  EXPECT_TRUE(sorter.is_top_strict());
  EXPECT_EQ("c", sorter.top());

  sorter.remove("c");
  ASSERT_EQ(4, sorter.size());
  EXPECT_TRUE(sorter.is_top_strict());
  EXPECT_EQ("a", sorter.top());

  sorter.remove("a");
  ASSERT_EQ(3, sorter.size());
  EXPECT_TRUE(sorter.is_top_strict());
  EXPECT_EQ("e", sorter.top());

  sorter.remove("e");
  ASSERT_EQ(2, sorter.size());
  EXPECT_TRUE(sorter.is_top_strict());
  EXPECT_EQ("d", sorter.top());

  sorter.remove("d");
  ASSERT_EQ(1, sorter.size());
  EXPECT_TRUE(sorter.is_top_strict());
  EXPECT_EQ("f", sorter.top());

  sorter.remove("f");
  ASSERT_EQ(0, sorter.size());
}

TEST(IncrementalPriorityQueueTest, EmptyException) {
  const IncrementalPriorityQueue<string> sorter;
  REBALANCER_EXPECT_RUNTIME_ERROR(sorter.top(), "empty");
  REBALANCER_EXPECT_RUNTIME_ERROR(sorter.is_top_strict(), "empty");
}
