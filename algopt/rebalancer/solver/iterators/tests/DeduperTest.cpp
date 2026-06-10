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

#include "algopt/rebalancer/solver/iterators/Deduper.h"
#include "algopt/rebalancer/solver/iterators/StlWrapper.h"

#include <gtest/gtest.h>

using namespace std;
using namespace facebook::rebalancer;

TEST(DeduperTest, Basic) {
  const vector<int> input = {4, 3, 5, 3, 8, 4, 4, 4, 2, 2};
  auto container = makeStlWrapperContainer(input);
  auto deduped = makeDeduperContainer(container);
  const vector<int> output(deduped.begin(), deduped.end());
  const vector<int> expected = {4, 3, 5, 8, 2};
  EXPECT_EQ(expected, output);
}
