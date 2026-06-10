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

#include "algopt/rebalancer/algopt_common/AccessOrderedFastSet.h"

#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <gtest/gtest.h>

namespace facebook::algopt::tests {

class AccessOrderedFastSetTest : public ::testing::Test {
 protected:
  template <typename T>
  void iterateOverSetAndCheck(
      int lineNumber, /*line number from which this function was called; for
                        printing purposes*/
      const AccessOrderedFastSet<T>& accessOrderedSet,
      const std::vector<T>& expectedIterOrder,
      std::optional<int> stopIterationAfterCollectingNthElement =
          std::nullopt) {
    std::vector<T> iteratedValues;
    for (auto i : accessOrderedSet) {
      if (stopIterationAfterCollectingNthElement.has_value() &&
          iteratedValues.size() == *stopIterationAfterCollectingNthElement) {
        break;
      }

      iteratedValues.push_back(i);
    }

    EXPECT_EQ(iteratedValues, expectedIterOrder)
        << " assertion failed on function call from line " << lineNumber;
  }

  template <typename T>
  void insertToSet(
      AccessOrderedFastSet<T>& accessOrderedSet,
      const std::vector<T>& values) {
    for (auto& i : values) {
      accessOrderedSet.insert(i);
    }
  }
};

TEST_F(AccessOrderedFastSetTest, BasicIterationWithJustInsert) {
  AccessOrderedFastSet<int> accessOrderedSet;

  EXPECT_EQ(accessOrderedSet.size(), 0);
  EXPECT_EQ(accessOrderedSet.begin(), accessOrderedSet.end());

  // NOLINTNEXTLINE(clang-diagnostic-unreachable-code-loop-increment)
  for ([[maybe_unused]] auto _ : accessOrderedSet) {
    throw std::runtime_error("Should not iterate over empty set");
  }

  insertToSet(accessOrderedSet, {2, 1, 3});

  // we expect begin to point to the first element
  EXPECT_EQ(*accessOrderedSet.begin(), 2);
  // just get end iterator and let it go out of scope; destructor should not
  // update currIter
  {
    auto endIt = accessOrderedSet.end();
    EXPECT_EQ(endIt, accessOrderedSet.end());
  }

  // perform a full iteration over the set and check that the order is
  // correct; also after full iteration, we expect begin() to point to the
  // first element again
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {2, 1, 3});
  EXPECT_EQ(*accessOrderedSet.begin(), 2);

  // access the first element; we now begin() to point to the second element
  // (the next starting point)
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {2}, 1);
  EXPECT_EQ(*accessOrderedSet.begin(), 1);

  // insert more elements
  insertToSet(accessOrderedSet, {43, 5, 7, 8, 9});

  EXPECT_EQ(accessOrderedSet.size(), 8);

  // repeat insert; will not be inserted
  accessOrderedSet.insert(43);
  EXPECT_EQ(accessOrderedSet.size(), 8);

  // perform a full iteration over the set and check that the order is
  // correct; NOTE: we start from 2nd element because the first element was
  // accessed last time
  iterateOverSetAndCheck<int>(
      __LINE__, accessOrderedSet, {1, 3, 43, 5, 7, 8, 9, 2}, 8);
  EXPECT_EQ(*accessOrderedSet.begin(), 1);

  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {1, 3, 43, 5, 7}, 5);
  EXPECT_EQ(*accessOrderedSet.begin(), 8);

  iterateOverSetAndCheck<int>(
      __LINE__, accessOrderedSet, {8, 9, 2, 1, 3, 43, 5}, 7);
  EXPECT_EQ(*accessOrderedSet.begin(), 7);

  iterateOverSetAndCheck<int>(
      __LINE__, accessOrderedSet, {7, 8, 9, 2, 1, 3, 43, 5}, 8);
  EXPECT_EQ(*accessOrderedSet.begin(), 7);
}

TEST_F(AccessOrderedFastSetTest, BasicIterationWithInsertAndErase) {
  AccessOrderedFastSet<int> accessOrderedSet;

  EXPECT_EQ(accessOrderedSet.size(), 0);
  EXPECT_EQ(accessOrderedSet.begin(), accessOrderedSet.end());

  // NOLINTNEXTLINE(clang-diagnostic-unreachable-code-loop-increment)
  for ([[maybe_unused]] auto _ : accessOrderedSet) {
    throw std::runtime_error("Should not iterate over empty set");
  }

  accessOrderedSet.insert(2);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {2}, 1);
  EXPECT_EQ(*accessOrderedSet.begin(), 2);

  accessOrderedSet.erase(2);
  EXPECT_EQ(accessOrderedSet.size(), 0);
  EXPECT_EQ(accessOrderedSet.begin(), accessOrderedSet.end());

  insertToSet(accessOrderedSet, {2, 1, 3, 4, 5});

  EXPECT_EQ(accessOrderedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {2, 1, 3, 4, 5});
  EXPECT_EQ(*accessOrderedSet.begin(), 2);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {2, 1, 3, 4}, 4);
  EXPECT_EQ(*accessOrderedSet.begin(), 5);

  // erase the last accessed element from the previous iteration
  accessOrderedSet.erase(4);
  EXPECT_FALSE(accessOrderedSet.contains(4));
  EXPECT_EQ(accessOrderedSet.size(), 4);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {5, 2, 1, 3}, 4);
  EXPECT_EQ(*accessOrderedSet.begin(), 5);

  // erase non-existent element
  accessOrderedSet.erase(4);
  EXPECT_FALSE(accessOrderedSet.contains(4));
  EXPECT_EQ(accessOrderedSet.size(), 4);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {5, 2}, 2);
  EXPECT_EQ(*accessOrderedSet.begin(), 1);

  // erase the first unaccessed element from the previous iteration
  accessOrderedSet.erase(1);
  EXPECT_FALSE(accessOrderedSet.contains(1));
  EXPECT_EQ(accessOrderedSet.size(), 3);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {3, 5}, 2);
  EXPECT_EQ(*accessOrderedSet.begin(), 2);

  accessOrderedSet.erase(3);
  EXPECT_FALSE(accessOrderedSet.contains(3));
  EXPECT_EQ(accessOrderedSet.size(), 2);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {2, 5});
  EXPECT_EQ(*accessOrderedSet.begin(), 2);

  accessOrderedSet.erase(5);
  EXPECT_FALSE(accessOrderedSet.contains(5));
  EXPECT_EQ(accessOrderedSet.size(), 1);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {2});
  EXPECT_EQ(*accessOrderedSet.begin(), 2);

  accessOrderedSet.erase(2);
  EXPECT_FALSE(accessOrderedSet.contains(2));
  EXPECT_TRUE(accessOrderedSet.empty());
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {});
  EXPECT_TRUE(accessOrderedSet.begin() == accessOrderedSet.end());
}

TEST_F(AccessOrderedFastSetTest, CopyAndMoveConstructors) {
  AccessOrderedFastSet<int> accessOrderedSet;
  insertToSet(accessOrderedSet, {2, 1, 3, 4, 5});

  AccessOrderedFastSet<int> copiedSet(accessOrderedSet);
  // begin() should be different between the two sets, since there is a new
  // elementList after copying
  EXPECT_TRUE(accessOrderedSet.begin() != copiedSet.begin());
  EXPECT_EQ(copiedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, copiedSet, {2, 1, 3, 4, 5});
  EXPECT_EQ(*copiedSet.begin(), 2);

  // inserting into the copied set should not affect the original set
  copiedSet.insert(6);
  EXPECT_EQ(copiedSet.size(), 6);
  EXPECT_EQ(accessOrderedSet.size(), 5);

  // check move constructor
  const AccessOrderedFastSet<int> movedSet(std::move(accessOrderedSet));
  EXPECT_EQ(movedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, movedSet, {2, 1, 3}, 3);
  EXPECT_EQ(*movedSet.begin(), 4);
}

TEST_F(AccessOrderedFastSetTest, CopyAndMoveConstructorPartialIteration) {
  AccessOrderedFastSet<int> accessOrderedSet;
  insertToSet(accessOrderedSet, {2, 1, 3, 4, 5});

  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {2, 1, 3}, 3);
  EXPECT_EQ(*accessOrderedSet.begin(), 4);
  const AccessOrderedFastSet<int> copiedSet(accessOrderedSet);
  EXPECT_FALSE(*accessOrderedSet.begin() == *copiedSet.begin());
  EXPECT_EQ(copiedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, copiedSet, {2, 1, 3, 4, 5});

  const AccessOrderedFastSet<int> movedSet(std::move(accessOrderedSet));
  EXPECT_EQ(movedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, movedSet, {2, 1, 3}, 3);
}

TEST_F(AccessOrderedFastSetTest, CopyAndMoveAssignmentBasic) {
  AccessOrderedFastSet<int> accessOrderedSet;
  insertToSet(accessOrderedSet, {6, 1, 3, 9, 11});

  AccessOrderedFastSet<int> copiedSet;
  copiedSet = accessOrderedSet;
  // begin() should be different between the two sets, since there is a new
  // elementList after copying
  EXPECT_FALSE(accessOrderedSet.begin() == copiedSet.begin());
  EXPECT_EQ(copiedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, copiedSet, {6, 1, 3, 9, 11});
  EXPECT_EQ(*copiedSet.begin(), 6);

  AccessOrderedFastSet<int> movedSet;
  movedSet = std::move(accessOrderedSet);
  EXPECT_EQ(movedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, movedSet, {6}, 1);
  EXPECT_EQ(*movedSet.begin(), 1);
}

TEST_F(AccessOrderedFastSetTest, CopyAndMoveAssignmentAfterPartialIteration) {
  AccessOrderedFastSet<int> accessOrderedSet;
  insertToSet(accessOrderedSet, {6, 1, 3, 9, 11});

  // if accessOrderedSet is copied after partial iteration, begin() of the new
  // set will start from the first element of the original set
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {6, 1}, 2);
  EXPECT_EQ(*accessOrderedSet.begin(), 3);
  AccessOrderedFastSet<int> copiedSet2;
  copiedSet2 = accessOrderedSet;
  EXPECT_EQ(copiedSet2.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, copiedSet2, {6, 1}, 2);

  // changes to the original set should not affect the copied set
  accessOrderedSet.insert(2);
  EXPECT_EQ(accessOrderedSet.size(), 6);
  EXPECT_EQ(copiedSet2.size(), 5);
  EXPECT_TRUE(accessOrderedSet.contains(2));
  EXPECT_FALSE(copiedSet2.contains(2));

  // if accessOrderedSet is moved after partial iteration, begin() of the new
  // set will start from the first element of the set
  EXPECT_EQ(*accessOrderedSet.begin(), 3);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSet, {3}, 1);
  EXPECT_EQ(*accessOrderedSet.begin(), 9);
  AccessOrderedFastSet<int> movedSet2;
  movedSet2 = std::move(accessOrderedSet);
  EXPECT_EQ(movedSet2.size(), 6);
  iterateOverSetAndCheck<int>(__LINE__, movedSet2, {6, 1}, 2);
}

TEST_F(AccessOrderedFastSetTest, ParallelIteration) {
  constexpr int numThreads = 200;
  const int nFunctions = numThreads * 10;
  int nElements = 10e3;
  auto executor = folly::CPUThreadPoolExecutor(numThreads);

  folly::Synchronized<folly::F14FastMap<int, std::vector<int>>>
      threadIdToAccessElements;

  std::vector<int> allElements;
  allElements.reserve(nElements);
  for (const auto i : folly::irange(nElements)) {
    allElements.push_back(i);
  }

  AccessOrderedFastSet<int> accessOrderedSet;
  insertToSet(accessOrderedSet, allElements);

  for (const auto i : folly::irange(nFunctions)) {
    executor.add([&, i]() {
      // each thread just iterates over the set collects all seen elements
      std::vector<int> seenElements;
      for (auto element : accessOrderedSet) {
        seenElements.push_back(element);
      }

      ASSERT_EQ(seenElements.size(), nElements)
          << " assertion failed on thread " << i;

      threadIdToAccessElements.wlock()->emplace(i, std::move(seenElements));
    });
  }

  executor.join();

  auto toOrderedSet = [](const std::vector<int>& vec) {
    return std::set<int>(vec.begin(), vec.end());
  };

  // check that all threads have seen the same elements, although note that
  // there is no guarantee on the order
  EXPECT_EQ(threadIdToAccessElements.rlock()->size(), nFunctions);
  for (const auto i : folly::irange(nFunctions)) {
    auto& seenElements = threadIdToAccessElements.rlock()->at(i);
    EXPECT_EQ(toOrderedSet(seenElements), toOrderedSet(allElements))
        << " assertion failed on thread " << i;
  }
}

} // namespace facebook::algopt::tests
