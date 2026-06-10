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

#include "algopt/rebalancer/algopt_common/BucketedPriorityQueue.h"
#include "algopt/rebalancer/algopt_common/TestUtils.h"

#include <folly/container/irange.h>
#include <gtest/gtest.h>

#include <string>
#include <utility>

namespace facebook::algopt::tests {

TEST(BucketedPriorityQueueTest, EmptyQueue) {
  const BucketedPriorityQueue<int, int> queue;
  EXPECT_TRUE(queue.empty());
}

TEST(BucketedPriorityQueueTest, Basic) {
  BucketedPriorityQueue<int, int> queue;
  queue.emplace(2, 20);
  queue.emplace(0, 1);
  queue.emplace(1, 10);
  queue.emplace(0, 2);

  EXPECT_EQ(20, queue.top());
  queue.pop();
  EXPECT_EQ(10, queue.top());
  queue.pop();

  // between 1 and 2 at priorityBucket 0, 1 is popped first because it was added
  // first
  EXPECT_EQ(1, queue.top());
  queue.pop();
  EXPECT_EQ(2, queue.top());
  queue.pop();
  EXPECT_TRUE(queue.empty());
}

TEST(BucketedPriorityQueueTest, ManyPriorityBucketsPoppedInOrder) {
  constexpr int kNumPriorityBuckets = 100;
  BucketedPriorityQueue<int, int> queue;
  for (const auto i : folly::irange(kNumPriorityBuckets)) {
    queue.emplace(i, i * 10);
  }

  for (const auto i : folly::irange(kNumPriorityBuckets)) {
    EXPECT_EQ((100 - 1 - i) * 10, queue.top());
    queue.pop();
  }
  EXPECT_TRUE(queue.empty());
}

TEST(BucketedPriorityQueueTest, InsertAfterPop) {
  BucketedPriorityQueue<int, int> queue;
  queue.emplace(0, 1);
  queue.emplace(1, 10);

  EXPECT_EQ(10, queue.top());
  queue.pop();

  queue.emplace(1, 2);
  EXPECT_EQ(2, queue.top());
  queue.pop();

  EXPECT_EQ(1, queue.top());
  queue.pop();
  EXPECT_TRUE(queue.empty());
}

TEST(BucketedPriorityQueueTest, Clear) {
  BucketedPriorityQueue<int, int> queue;
  queue.emplace(0, 1);
  queue.emplace(1, 10);
  queue.emplace(2, 20);

  EXPECT_FALSE(queue.empty());
  queue.clear();
  EXPECT_TRUE(queue.empty());
}

TEST(BucketedPriorityQueueTest, ClearAndReuse) {
  BucketedPriorityQueue<int, int> queue;
  queue.emplace(0, 1);
  queue.clear();

  queue.emplace(1, 10);
  EXPECT_EQ(10, queue.top());
  queue.pop();
  EXPECT_TRUE(queue.empty());
}

TEST(BucketedPriorityQueueTest, WithPairsEmplace) {
  BucketedPriorityQueue<int, std::pair<int, std::string>> queue;
  queue.emplace(1, 10, "ten");
  queue.emplace(0, 1, "one");
  queue.emplace(0, 2, "two");

  const auto& [i1, str1] = queue.top();
  EXPECT_EQ(10, i1);
  EXPECT_EQ("ten", str1);
  queue.pop();

  const auto& [i2, str2] = queue.top();
  EXPECT_EQ(1, i2);
  EXPECT_EQ("one", str2);
  queue.pop();

  const auto& [i3, str3] = queue.top();
  EXPECT_EQ(2, i3);
  EXPECT_EQ("two", str3);
  queue.pop();

  EXPECT_TRUE(queue.empty());
}

TEST(BucketedPriorityQueueTest, ComplexPriorityType) {
  struct CustomPair {
    int x;
    int y;
    bool operator>(const CustomPair& other) const {
      return (x == other.x) ? (y > other.y) : (x > other.x);
    }
  };
  BucketedPriorityQueue<CustomPair, std::string> queue;
  queue.emplace({.x = 1, .y = 10}, "one-ten");
  queue.emplace({.x = 0, .y = 1}, "zero-one");
  queue.emplace({.x = 0, .y = 2}, "zero-two");
  queue.emplace({.x = 1, .y = 2}, "one-two");
  queue.emplace({.x = -1, .y = 2}, "minus-one-two");

  const auto& str0 = queue.top();
  EXPECT_EQ("one-ten", str0);

  queue.pop();

  const auto& str1 = queue.top();
  EXPECT_EQ("one-two", str1);
  queue.pop();

  const auto& str2 = queue.top();
  EXPECT_EQ("zero-two", str2);
  queue.pop();

  const auto& str3 = queue.top();
  EXPECT_EQ("zero-one", str3);
  queue.pop();

  const auto& str4 = queue.top();
  EXPECT_EQ("minus-one-two", str4);
  queue.pop();

  EXPECT_TRUE(queue.empty());
}

TEST(BucketedPriorityQueueTest, EmptyTopException) {
  const BucketedPriorityQueue<int, int> queue;
  REBALANCER_EXPECT_RUNTIME_ERROR(
      std::ignore = queue.top(), "cannot call top()/pop() when queue is empty");
}

TEST(BucketedPriorityQueueTest, EmptyPopException) {
  BucketedPriorityQueue<int, int> queue;
  REBALANCER_EXPECT_RUNTIME_ERROR(
      queue.pop(), "cannot call top()/pop() when queue is empty");
}

} // namespace facebook::algopt::tests
