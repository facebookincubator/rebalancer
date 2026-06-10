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

#include "algopt/rebalancer/solver/iterators/Range.h"
#include "algopt/rebalancer/solver/iterators/Timeout.h"

#include <gtest/gtest.h>

using namespace facebook::rebalancer;

TEST(TimeoutTest, TimeoutNotReached) {
  Timeout<Range<int>> timeout(Range<int>(0, 100), 10);
  timeout.start_timer();
  int last = -1;
  for (const int i : timeout) {
    last = i;
  }
  EXPECT_EQ(99, last);
}

TEST(TimeoutTest, TimeoutReached) {
  Timeout<Range<int>> timeout(Range<int>(0, 1000000000), 0.1);
  timeout.start_timer();
  int last = -1;
  for (const int i : timeout) {
    last = i;
  }
  EXPECT_LT(100, last);
  EXPECT_GT(500000000, last);
}

TEST(TimeoutTest, InitiallyExpired) {
  Timeout<Range<int>> timeout(Range<int>(0, 100), 0);
  timeout.start_timer();
  int last = -1;
  for (const int i : timeout) {
    last = i;
  }
  EXPECT_EQ(-1, last);
}
