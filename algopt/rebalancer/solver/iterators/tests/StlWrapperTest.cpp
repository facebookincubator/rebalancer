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

#include <gtest/gtest.h>

using namespace std;
using namespace facebook::rebalancer;

TEST(StlWrapperTest, MapInContainerWrapper) {
  const map<string, int> input = {{"a", 1}, {"b", 2}, {"c", 3}};
  auto container = makeStlWrapperContainer(input);
  const vector<pair<string, int>> output(container.begin(), container.end());
  const vector<pair<string, int>> expected = {{"a", 1}, {"b", 2}, {"c", 3}};
  EXPECT_EQ(expected, output);
}
