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

#include "algopt/rebalancer/entities/tests/UniverseBuilderTestUtils.h"
#include "algopt/rebalancer/solver/moves/Move.h"
#include "algopt/rebalancer/solver/tests/IdConverterTestUtils.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

TEST(MoveTest, compareMoves) {
  {
    // Same move will return 0 when compared
    const Move move1 = Move{object(1), container(2), container(3)};
    const Move move2 = Move{object(1), container(2), container(3)};
    EXPECT_EQ(0, Move::compare(move1, move2));
  }

  {
    // compare is antisymmetric for different moves
    const Move move1 = Move{object(1), container(2), container(3)};
    const Move move2 = Move{object(9), container(2), container(3)};
    auto cmp = Move::compare(move1, move2);
    EXPECT_NE(0, cmp);
    EXPECT_EQ(-cmp, Move::compare(move2, move1));
  }

  {
    // compare is antisymmetric for different moves (different object)
    const Move move1 = Move{object(1), container(2), container(3)};
    const Move move2 = Move{object(0), container(2), container(3)};
    auto cmp = Move::compare(move1, move2);
    EXPECT_NE(0, cmp);
    EXPECT_EQ(-cmp, Move::compare(move2, move1));
  }
}

TEST(MoveTest, getHashValue) {
  const Move move1 = Move{object(1), container(2), container(3)};
  const Move move2 = Move{object(1), container(2), container(3)};
  const Move move3 = Move{object(1), container(2), container(4)};

  // 2 different moves with same object, source and destination containers will
  // have same hash value
  EXPECT_EQ(move1.getHashValue(), move2.getHashValue());

  EXPECT_NE(move3.getHashValue(), move1.getHashValue());
}

TEST(MoveTest, toString) {
  entities::tests::UniverseBuilderTestUtils builder("task", "host");
  builder.setInitialAssignment(
      {{"host1", {"task1"}}, {"host2", {"task2"}}, {"host3", {}}});

  // Create object and container IDs with specific names
  auto objectId = builder.object("task1");
  auto srcContainerId = builder.container("host2");
  auto dstContainerId = builder.container("host3");

  // Create a move with these IDs
  const Move move{objectId, srcContainerId, dstContainerId};

  // Call toString and verify the output format
  const auto universe = builder.buildUniverse();
  const std::string result = move.toString(*universe);

  // Verify that the string contains the expected components
  EXPECT_TRUE(result.find("Object = task1") != std::string::npos);
  EXPECT_TRUE(result.find("Source Container = host2") != std::string::npos);
  EXPECT_TRUE(
      result.find("Destination Container = host3") != std::string::npos);
  EXPECT_TRUE(result.find("id:") != std::string::npos);
}

} // namespace facebook::rebalancer::packer::tests
