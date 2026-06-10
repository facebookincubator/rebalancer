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

class SwapsTest : public ExpressionTestsBase {
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

TEST_F(SwapsTest, Swaps) {
  const auto universe = buildUniverse();
  auto s = swaps(
      {{object(1), container(1)},
       {object(2), container(1)},
       {object(3), container(2)},
       {object(4), container(2)},
       {object(5), container(2)},
       {object(6), container(3)}},
      universe,
      {});

  Assignment assignment(
      {{container(1), {object(1), object(2)}},
       {container(2), {object(3), object(4), object(5)}},
       {container(3), {object(6)}}});

  EXPECT_EQ(1, apply(s, assignment));

  // Initial
  EXPECT_EQ(1, evaluate(s, {}, assignment));

  // Single move
  EXPECT_EQ(0, evaluate(s, {{object(1), container(2)}}, assignment));

  // Real swap
  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(1), container(2)}, {object(3), container(1)}},
          assignment));

  // 3-cycle
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(1), container(2)},
           {object(3), container(3)},
           {object(6), container(1)}},
          assignment));

  // Many swap
  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(1), container(2)},
           {object(3), container(1)},
           {object(2), container(2)},
           {object(4), container(1)},
           {object(5), container(3)},
           {object(6), container(2)}},
          assignment));

  // Same checks but after {object(1), container(2)} applied
  assignment.moveTo(object(1), container(2));

  EXPECT_EQ(0, apply(s, assignment));
  EXPECT_EQ(0, evaluate(s, {}, assignment));
  EXPECT_EQ(1, evaluate(s, {{object(3), container(1)}}, assignment));
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(3), container(3)}, {object(6), container(1)}},
          assignment));
  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(3), container(1)},
           {object(2), container(2)},
           {object(4), container(1)},
           {object(5), container(3)},
           {object(6), container(2)}},
          assignment));

  // Undo last apply
  auto changes = ObjectToNewContainer{{object(1), container(1)}};
  EXPECT_EQ(1, applyChanges(s, changes, assignment));
  assignment.moveTo(object(1), container(1));

  EXPECT_EQ(1, evaluate(s, {}, assignment));
  EXPECT_EQ(0, evaluate(s, {{object(1), container(2)}}, assignment));
  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(1), container(2)}, {object(3), container(1)}},
          assignment));

  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(1), container(2)},
           {object(3), container(3)},
           {object(6), container(1)}},
          assignment));
}

TEST_F(SwapsTest, SwapSubsets) {
  const auto universe = buildUniverse();
  auto s = swaps(
      {{object(1), container(1)},
       {object(2), container(1)},
       {object(3), container(2)},
       {object(4), container(2)},
       {object(5), container(2)},
       {object(6), container(3)}},
      universe,
      {{object(1)}});
  Assignment assignment(
      {{container(1), {object(1), object(2)}},
       {container(2), {object(3), object(4), object(5)}},
       {container(3), {object(6)}}});
  EXPECT_EQ(1, apply(s, assignment));

  // Initial
  EXPECT_EQ(1, evaluate(s, {}, assignment));

  // Single move
  EXPECT_EQ(0, evaluate(s, {{object(1), container(2)}}, assignment));

  // Real swap
  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(1), container(2)}, {object(3), container(1)}},
          assignment));

  // Real swap
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(3), container(3)}, {object(5), container(2)}},
          assignment));

  // 3-cycle
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(1), container(2)},
           {object(3), container(3)},
           {object(6), container(1)}},
          assignment));

  // Many swap
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(1), container(2)},
           {object(3), container(1)},
           {object(2), container(2)},
           {object(4), container(1)},
           {object(5), container(3)},
           {object(6), container(2)}},
          assignment));

  // Same checks but after {object(1), container(2)} applied
  assignment.moveTo(object(1), container(2));

  EXPECT_EQ(0, apply(s, assignment));
  EXPECT_EQ(0, evaluate(s, {}, assignment));
  EXPECT_EQ(1, evaluate(s, {{object(3), container(1)}}, assignment));
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(3), container(3)}, {object(6), container(1)}},
          assignment));
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(3), container(1)},
           {object(2), container(2)},
           {object(4), container(1)},
           {object(5), container(3)},
           {object(6), container(2)}},
          assignment));

  // Undo last apply
  auto changes = ObjectToNewContainer{{object(1), container(1)}};
  EXPECT_EQ(1, applyChanges(s, changes, assignment));
  assignment.moveTo(object(1), container(1));

  EXPECT_EQ(1, evaluate(s, {}, assignment));
  EXPECT_EQ(0, evaluate(s, {{object(1), container(2)}}, assignment));
  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(1), container(2)}, {object(3), container(1)}},
          assignment));
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(1), container(2)},
           {object(3), container(3)},
           {object(6), container(1)}},
          assignment));
}

TEST_F(SwapsTest, Swaps2) {
  const auto universe = buildUniverse();
  auto s = swaps(
      {{object(1), container(10)},
       {object(2), container(20)},
       {object(3), container(20)}},
      universe,
      {});
  const Assignment assignment1(
      {{container(10), {object(1)}}, {container(20), {object(2), object(3)}}});
  EXPECT_EQ(1, apply(s, assignment1));
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(1), container(20)},
           {object(2), container(10)},
           {object(3), container(10)}},
          assignment1));
  const Assignment assignment2(
      {{container(10), {object(2), object(3)}}, {container(20), {object(1)}}});
  EXPECT_EQ(0, apply(s, assignment2));
}

TEST_F(SwapsTest, SwapSubsets2) {
  const auto universe = buildUniverse();
  auto s = swaps(
      {{object(1), container(10)},
       {object(2), container(20)},
       {object(3), container(30)},
       {object(4), container(40)}},
      universe,
      {{object(1), object(2)}});

  const Assignment assignment1(
      {{container(10), {object(1)}},
       {container(20), {object(2)}},
       {container(30), {object(3)}},
       {container(40), {object(4)}}});
  EXPECT_EQ(1, apply(s, assignment1));

  // swap(object(1), container(2)) + swap(3, 4)
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(1), container(20)},
           {object(2), container(10)},
           {object(3), container(40)},
           {object(4), container(30)}},
          assignment1));
  const Assignment assignment2(
      {{container(20), {object(1)}},
       {container(10), {object(2)}},
       {container(30), {object(4)}},
       {container(40), {object(3)}}});
  EXPECT_EQ(0, apply(s, assignment2));

  // swap(1, 4) + swap(2, 3)
  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(1), container(30)},
           {object(2), container(40)},
           {object(3), container(10)},
           {object(4), container(20)}},
          assignment2));
  const Assignment assignment3(
      {{container(10), {object(3)}},
       {container(20), {object(4)}},
       {container(30), {object(1)}},
       {container(40), {object(2)}}});
  EXPECT_EQ(1, apply(s, assignment3));
}

TEST_F(SwapsTest, SwapSubsetAtLeastOne) {
  const auto universe = buildUniverse();
  // Initial assignment:
  //   Container 10: Object 1, Object 3
  //   Container 20: Object 2, Object 4
  // Subset: Object 1, Object 2
  // Definition: AT_LEAST_ONE_IN_SUBSET
  const PackerMap<entities::ObjectId, entities::ContainerId> initialAssignment =
      {{object(1), container(10)},
       {object(2), container(20)},
       {object(3), container(10)},
       {object(4), container(20)}};

  auto s = swaps(
      initialAssignment,
      universe,
      {{object(1), object(2)}},
      Swaps::SubsetDefinition::AT_LEAST_ONE_IN_SUBSET);

  auto assignment = makeAssignment(initialAssignment);

  EXPECT_EQ(1, apply(s, assignment));

  // Possible swaps and their result with AT_LEAST_ONE_IN_SUBSET:
  //   Swap objects 1 & 2: allowed
  //   Swap objects 1 & 4: allowed
  //   Swap objects 2 & 3: allowed
  //   Swap objects 3 & 4: disallowed
  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(1), container(20)}, {object(2), container(10)}},
          assignment));

  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(1), container(20)}, {object(4), container(10)}},
          assignment));

  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(2), container(10)}, {object(3), container(20)}},
          assignment));

  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(3), container(20)}, {object(4), container(10)}},
          assignment));
}

TEST_F(SwapsTest, SwapSubsetExactlyOne) {
  const auto universe = buildUniverse();
  // Initial assignment:
  //   Container 10: Object 1, Object 3
  //   Container 20: Object 2, Object 4
  // Subset: Object 1, Object 2
  // Definition: EXACTLY_ONE_IN_SUBSET
  const PackerMap<entities::ObjectId, entities::ContainerId> initialAssignment =
      {{object(1), container(10)},
       {object(2), container(20)},
       {object(3), container(10)},
       {object(4), container(20)}};

  auto s = swaps(
      initialAssignment,
      universe,
      {{object(1), object(2)}},
      Swaps::SubsetDefinition::EXACTLY_ONE_IN_SUBSET);

  auto assignment = makeAssignment(initialAssignment);

  EXPECT_EQ(1, apply(s, assignment));

  // Possible swaps and their result with EXACTLY_ONE_IN_SUBSET:
  //   Swap objects 1 & 2: disallowed
  //   Swap objects 1 & 4: allowed
  //   Swap objects 2 & 3: allowed
  //   Swap objects 3 & 4: disallowed
  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(1), container(20)}, {object(2), container(10)}},
          assignment));

  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(1), container(20)}, {object(4), container(10)}},
          assignment));

  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(2), container(10)}, {object(3), container(20)}},
          assignment));

  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(3), container(20)}, {object(4), container(10)}},
          assignment));
}

TEST_F(SwapsTest, SwapBothSameSideOfSubset) {
  const auto universe = buildUniverse();
  // Initial assignment:
  //   Container 10: Object 1, Object 3, Object 5
  //   Container 20: Object 2, Object 4
  // Subset: Object 1, Object 2, Object 3
  // Definition: BOTH_SAME_SIDE_OF_SUBSET
  const PackerMap<entities::ObjectId, entities::ContainerId> initialAssignment =
      {{object(1), container(10)},
       {object(2), container(20)},
       {object(3), container(10)},
       {object(4), container(20)},
       {object(5), container(10)}};

  auto s = swaps(
      initialAssignment,
      universe,
      {{object(1), object(2), object(3)}},
      Swaps::SubsetDefinition::BOTH_SAME_SIDE_OF_SUBSET);

  auto assignment = makeAssignment(initialAssignment);

  EXPECT_EQ(1, apply(s, assignment));

  // Possible swaps and their result with BOTH_SAME_SIDE_OF_SUBSET:
  //   Swap objects 1 & 2: allowed
  //   Swap objects 1 & 4: disallowed
  //   Swap objects 2 & 3: allowed
  //   Swap objects 3 & 4: disallowed
  //   Swap objects 4 & 5: allowed
  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(1), container(20)}, {object(2), container(10)}},
          assignment));

  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(1), container(20)}, {object(4), container(10)}},
          assignment));

  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(2), container(10)}, {object(3), container(20)}},
          assignment));

  EXPECT_EQ(
      0,
      evaluate(
          s,
          {{object(3), container(20)}, {object(4), container(10)}},
          assignment));

  EXPECT_EQ(
      1,
      evaluate(
          s,
          {{object(4), container(10)}, {object(5), container(20)}},
          assignment));
}

TEST_F(SwapsTest, SwapsInitialValue) {
  const auto universe = buildUniverse();

  auto sw =
      swaps({{object(0), container(0)}, {object(1), container(1)}}, universe);
  EXPECT_EQ(1.0, sw->getInitialValue());
}

} // namespace facebook::rebalancer::packer::tests
