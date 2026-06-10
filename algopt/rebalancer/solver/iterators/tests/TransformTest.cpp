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

#include "algopt/rebalancer/solver/iterators/StlWrapper.h"
#include "algopt/rebalancer/solver/iterators/Transform.h"

#include <gtest/gtest.h>

#include <string>

using namespace std;
using namespace facebook::rebalancer;

TEST(TransformTest, Basic) {
  const vector<int> input = {1, 2, 3, 7};
  auto container = makeStlWrapperContainer(input);
  const std::function<int(int)> multiplyBy2 = [](int x) { return 2 * x; };
  auto transformed = makeTransformContainer(container, multiplyBy2);
  const vector<int> output(transformed.begin(), transformed.end());
  const vector<int> expected = {2, 4, 6, 14};
  EXPECT_EQ(expected, output);
}

TEST(TransformTest, DifferentTypes) {
  const vector<int> input = {1, 2, 3, 7};
  auto container = makeStlWrapperContainer(input);
  const std::function<string(int)> convertToString = [](int x) {
    return std::to_string(x);
  };
  auto transformed = makeTransformContainer(container, convertToString);
  const vector<string> output(transformed.begin(), transformed.end());
  const vector<string> expected = {"1", "2", "3", "7"};
  EXPECT_EQ(expected, output);
}
