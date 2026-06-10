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

#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class BipartiteSwapsTest : public ExpressionTestsBase {
 protected:
  void SetUp() override {
    constexpr int kNumContainers = 50;
    entities::Map<std::string, std::vector<std::string>> initialAssignment;
    for (const auto i : folly::irange(kNumContainers)) {
      initialAssignment[fmt::format("container{}", i)] = {
          fmt::format("object{}", i)};
    }
    setInitialAssignment(initialAssignment);
  }
};

TEST_F(BipartiteSwapsTest, BipartiteOppositeSwaps) {
  const auto universe = buildUniverse();
  // Initial assignment:
  //   Container 10: Object 1
  //   Container 20: Object 2
  //   Container 30: Object 3
  //   Container 40: Object 4
  // Bipartite Subset: Container 10, Container 20
  const PackerMap<entities::ObjectId, entities::ContainerId> initialAssignment =
      {{object(1), container(10)},
       {object(2), container(20)},
       {object(3), container(30)},
       {object(4), container(40)}};

  auto bipartiteSwap = bipartite_swaps(
      initialAssignment,
      {{container(10), container(20)}},
      {{container(30), container(40)}},
      universe);

  auto assignment = makeAssignment(initialAssignment);

  EXPECT_EQ(1, apply(bipartiteSwap, assignment));

  //   Swap objects 1 & 2: allowed (TODO: disallow this)
  //   Swap objects 1 & 4: allowed
  //   Swap objects 2 & 3: allowed
  EXPECT_EQ(
      1,
      evaluate(
          bipartiteSwap,
          {{object(1), container(20)}, {object(2), container(10)}},
          assignment));

  EXPECT_EQ(
      1,
      evaluate(
          bipartiteSwap,
          {{object(1), container(40)}, {object(4), container(10)}},
          assignment));

  EXPECT_EQ(
      1,
      evaluate(
          bipartiteSwap,
          {{object(2), container(30)}, {object(3), container(20)}},
          assignment));

  // //   Evaluate o1 to c20: allowed because 10 and 20 are in same side (TODO
  // //   disallow this)
  EXPECT_EQ(
      1, evaluate(bipartiteSwap, {{object(1), container(20)}}, assignment));
}

TEST_F(BipartiteSwapsTest, BipartiteSwapsSameSide) {
  const auto universe = buildUniverse();
  // Initial assignment:
  //   Container 10: Object 1
  //   Container 20: Object 2
  //   Container 30: Object 3
  //   Container 40: Object 4
  // Bipartite Subset: Container 10, Container 20
  const PackerMap<entities::ObjectId, entities::ContainerId> initialAssignment =
      {{object(1), container(10)},
       {object(2), container(20)},
       {object(3), container(30)},
       {object(4), container(40)}};

  auto bipartiteSwap = bipartite_swaps(
      initialAssignment,
      {{container(10), container(20)}},
      {{container(30), container(40)}},
      universe);

  auto assignment = makeAssignment(initialAssignment);

  EXPECT_EQ(1, apply(bipartiteSwap, assignment));

  //   Evaluate and then apply o1 to c30: disallowed
  EXPECT_EQ(
      0, evaluate(bipartiteSwap, {{object(1), container(30)}}, assignment));

  // update the assignment and full apply
  assignment.moveTo(object(1), container(30));
  EXPECT_EQ(0, apply(bipartiteSwap, assignment));

  //   On top of o1 to c30, evaluate a valid swap
  //   and it should still be disallowed
  EXPECT_EQ(
      0,
      evaluate(
          bipartiteSwap,
          {{object(1), container(20)}, {object(2), container(30)}},
          assignment));

  //   o1 moved to c30, was disallowed
  //   Then swap o1<->o4 within same partition which is still disallowed
  EXPECT_EQ(
      0,
      evaluate(
          bipartiteSwap,
          {{object(1), container(40)}, {object(4), container(30)}},
          assignment));

  //   Now correct the broken bipartite swap by moving o3 to c10: allowed
  EXPECT_EQ(
      1, evaluate(bipartiteSwap, {{object(3), container(10)}}, assignment));
}

} // namespace facebook::rebalancer::packer::tests
