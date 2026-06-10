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

#include "algopt/rebalancer/algopt_common/AccessOrderedSortedSet.h"

#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <gtest/gtest.h>

namespace facebook::algopt::tests {

class AccessOrderedSortedSetTest : public ::testing::Test {
 protected:
  class Comparator {
   public:
    bool operator()(int a, int b) const {
      return a > b;
    }
  };

  template <typename T>
  void iterateOverSetAndCheck(
      int lineNumber, /*line number from which this function was called; for
                        printing purposes*/
      const AccessOrderedSortedSet<T, Comparator>& accessOrderedSortedSet,
      const std::vector<T>& expectedIterOrder,
      std::optional<int> stopIterationAfterCollectingNthElement =
          std::nullopt) {
    std::vector<T> iteratedValues;
    for (auto i : accessOrderedSortedSet) {
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
      AccessOrderedSortedSet<T, Comparator>& accessOrderedSortedSet,
      const std::vector<T>& values) {
    for (auto& i : values) {
      accessOrderedSortedSet.insert(i);
    }
  }

  Comparator comparator;
};

TEST_F(AccessOrderedSortedSetTest, BasicIterationWithJustInsert) {
  AccessOrderedSortedSet<int, Comparator> accessOrderedSortedSet(comparator);

  EXPECT_EQ(accessOrderedSortedSet.size(), 0);
  EXPECT_EQ(accessOrderedSortedSet.begin(), accessOrderedSortedSet.end());

  // NOLINTNEXTLINE(clang-diagnostic-unreachable-code-loop-increment)
  for ([[maybe_unused]] auto _ : accessOrderedSortedSet) {
    throw std::runtime_error("Should not iterate over empty set");
  }

  insertToSet(accessOrderedSortedSet, {2, 1, 3});

  ASSERT_EQ(accessOrderedSortedSet.size(), 3);

  // we expect begin to point to the first element, which should be 3 since it
  // is the largest element and Comparator is in descending order
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 3);

  // perform a full iteration over the set and check that the order is
  // correct; also after full iteration, we expect begin() to point to the
  // first element again
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {3, 2, 1});
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 3);

  // access the first element; after that we now want begin() to point to the
  // second element (the next starting point)
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {3}, 1);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 2);

  // insert more elements
  insertToSet(accessOrderedSortedSet, {43, 5, 8, 7, 9});

  EXPECT_EQ(accessOrderedSortedSet.size(), 8);

  // repeat insert; will not be inserted
  accessOrderedSortedSet.insert(43);
  EXPECT_EQ(accessOrderedSortedSet.size(), 8);

  // perform a full iteration over the set and check that the order is
  // correct;
  // NOTE: begin is reset because there was an insertion, so we will strat from
  // 43
  iterateOverSetAndCheck<int>(
      __LINE__, accessOrderedSortedSet, {43, 9, 8, 7, 5, 3, 2, 1}, 8);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 43);

  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {43, 9, 8}, 3);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 7);

  // repeat insert; will not be inserted AND we expect begin() to NOT be reset
  accessOrderedSortedSet.insert(9);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 7);

  iterateOverSetAndCheck<int>(
      __LINE__, accessOrderedSortedSet, {7, 5, 3, 2, 1, 43}, 6);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 9);

  iterateOverSetAndCheck<int>(
      __LINE__, accessOrderedSortedSet, {9, 8, 7, 5, 3, 2, 1}, 7);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 43);

  iterateOverSetAndCheck<int>(
      __LINE__, accessOrderedSortedSet, {43, 9, 8, 7}, 4);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 5);

  // new insertion; we expect begin() to be reset
  accessOrderedSortedSet.insert(4);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 43);

  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {43}, 1);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 9);
}

TEST_F(AccessOrderedSortedSetTest, BasicIterationWithInsertAndErase) {
  AccessOrderedSortedSet<int, Comparator> accessOrderedSortedSet(comparator);

  EXPECT_EQ(accessOrderedSortedSet.size(), 0);
  EXPECT_EQ(accessOrderedSortedSet.begin(), accessOrderedSortedSet.end());

  // NOLINTNEXTLINE(clang-diagnostic-unreachable-code-loop-increment)
  for ([[maybe_unused]] auto _ : accessOrderedSortedSet) {
    throw std::runtime_error("Should not iterate over empty set");
  }

  accessOrderedSortedSet.insert(2);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {2}, 1);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 2);

  accessOrderedSortedSet.erase(2);
  EXPECT_EQ(accessOrderedSortedSet.size(), 0);
  EXPECT_EQ(accessOrderedSortedSet.begin(), accessOrderedSortedSet.end());

  insertToSet(accessOrderedSortedSet, {2, 1, 3, 4, 5});

  EXPECT_EQ(accessOrderedSortedSet.size(), 5);
  iterateOverSetAndCheck<int>(
      __LINE__, accessOrderedSortedSet, {5, 4, 3, 2, 1});
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 5);
  iterateOverSetAndCheck<int>(
      __LINE__, accessOrderedSortedSet, {5, 4, 3, 2}, 4);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 1);

  // erase the last accessed element from the previous iteration
  accessOrderedSortedSet.erase(3);
  EXPECT_FALSE(accessOrderedSortedSet.contains(3));
  EXPECT_EQ(accessOrderedSortedSet.size(), 4);
  iterateOverSetAndCheck<int>(
      __LINE__, accessOrderedSortedSet, {1, 5, 4, 2}, 4);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 1);

  // erase non-existent element
  accessOrderedSortedSet.erase(3);
  EXPECT_FALSE(accessOrderedSortedSet.contains(3));
  EXPECT_EQ(accessOrderedSortedSet.size(), 4);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {1, 5}, 2);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 4);

  // erase the first unaccessed element from the previous iteration
  accessOrderedSortedSet.erase(4);
  EXPECT_FALSE(accessOrderedSortedSet.contains(4));
  EXPECT_EQ(accessOrderedSortedSet.size(), 3);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {2, 1}, 2);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 5);

  accessOrderedSortedSet.erase(2);
  EXPECT_FALSE(accessOrderedSortedSet.contains(2));
  EXPECT_EQ(accessOrderedSortedSet.size(), 2);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {5, 1});
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 5);

  accessOrderedSortedSet.erase(1);
  EXPECT_FALSE(accessOrderedSortedSet.contains(1));
  EXPECT_EQ(accessOrderedSortedSet.size(), 1);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {5});
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 5);

  accessOrderedSortedSet.erase(5);
  EXPECT_FALSE(accessOrderedSortedSet.contains(5));
  EXPECT_TRUE(accessOrderedSortedSet.empty());
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {});
  EXPECT_TRUE(accessOrderedSortedSet.begin() == accessOrderedSortedSet.end());
}

TEST_F(AccessOrderedSortedSetTest, CopyAndMoveConstructors) {
  AccessOrderedSortedSet<int, Comparator> accessOrderedSortedSet(comparator);
  insertToSet(accessOrderedSortedSet, {2, 1, 3, 4, 5});

  AccessOrderedSortedSet<int, Comparator> copiedSet(accessOrderedSortedSet);
  // begin() should be different between the two sets, since there is a new
  // elementList after copying
  EXPECT_FALSE(accessOrderedSortedSet.begin() == copiedSet.begin());
  EXPECT_EQ(copiedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, copiedSet, {5, 4, 3, 2, 1});
  EXPECT_EQ(*copiedSet.begin(), 5);

  // inserting into the copied set should not affect the original set
  copiedSet.insert(6);
  EXPECT_EQ(copiedSet.size(), 6);
  EXPECT_EQ(accessOrderedSortedSet.size(), 5);

  // check move constructor
  const AccessOrderedSortedSet<int, Comparator> movedSet(
      std::move(accessOrderedSortedSet));
  EXPECT_EQ(movedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, movedSet, {5, 4, 3}, 3);
  EXPECT_EQ(*movedSet.begin(), 2);
}

TEST_F(AccessOrderedSortedSetTest, CopyAndMoveConstructorPartialIteration) {
  AccessOrderedSortedSet<int, Comparator> accessOrderedSortedSet(comparator);
  insertToSet(accessOrderedSortedSet, {2, 1, 3, 4, 5});

  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {5, 4, 3}, 3);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 2);
  const AccessOrderedSortedSet<int, Comparator> copiedSet(
      accessOrderedSortedSet);
  EXPECT_FALSE(*accessOrderedSortedSet.begin() == *copiedSet.begin());
  EXPECT_EQ(copiedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, copiedSet, {5, 4, 3, 2, 1});

  const AccessOrderedSortedSet<int, Comparator> movedSet(
      std::move(accessOrderedSortedSet));
  EXPECT_EQ(movedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, movedSet, {5, 4}, 2);
}

TEST_F(AccessOrderedSortedSetTest, CopyAndMoveAssignmentBasic) {
  AccessOrderedSortedSet<int, Comparator> accessOrderedSortedSet(comparator);
  insertToSet(accessOrderedSortedSet, {6, 1, 3, 9, 11});

  AccessOrderedSortedSet<int, Comparator> copiedSet(comparator);
  copiedSet = accessOrderedSortedSet;
  // begin() should be different between the two sets, since there is a new
  // elementList after copying
  EXPECT_FALSE(accessOrderedSortedSet.begin() == copiedSet.begin());
  EXPECT_EQ(copiedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, copiedSet, {11, 9, 6, 3}, 4);
  EXPECT_EQ(*copiedSet.begin(), 1);

  AccessOrderedSortedSet<int, Comparator> movedSet(comparator);
  movedSet = std::move(accessOrderedSortedSet);
  EXPECT_EQ(movedSet.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, movedSet, {11}, 1);
  EXPECT_EQ(*movedSet.begin(), 9);
}

TEST_F(AccessOrderedSortedSetTest, CopyAndMoveAssignmentAfterPartialIteration) {
  AccessOrderedSortedSet<int, Comparator> accessOrderedSortedSet(comparator);
  insertToSet(accessOrderedSortedSet, {6, 1, 3, 9, 11});

  // if accessOrderedSortedSet is copied after partial iteration, begin() of the
  // new set will start from the first element in the original set
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {11, 9}, 2);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 6);
  AccessOrderedSortedSet<int, Comparator> copiedSet2(comparator);
  copiedSet2 = accessOrderedSortedSet;
  EXPECT_EQ(copiedSet2.size(), 5);
  iterateOverSetAndCheck<int>(__LINE__, copiedSet2, {11, 9}, 2);

  // changes to the original set should not affect the copied set
  accessOrderedSortedSet.insert(2);
  EXPECT_EQ(accessOrderedSortedSet.size(), 6);
  EXPECT_EQ(copiedSet2.size(), 5);
  EXPECT_TRUE(accessOrderedSortedSet.contains(2));
  EXPECT_FALSE(copiedSet2.contains(2));

  // if accessOrderedSortedSet is moved after partial iteration, begin() of the
  // new set will start from the first element of the set
  // note that we had an insertion, so begin() of accessOrderedSortedSet is
  // reset
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 11);
  iterateOverSetAndCheck<int>(__LINE__, accessOrderedSortedSet, {11}, 1);
  EXPECT_EQ(*accessOrderedSortedSet.begin(), 9);
  AccessOrderedSortedSet<int, Comparator> movedSet2(comparator);
  movedSet2 = std::move(accessOrderedSortedSet);
  EXPECT_EQ(movedSet2.size(), 6);
  iterateOverSetAndCheck<int>(__LINE__, movedSet2, {11, 9, 6}, 3);
}

TEST_F(AccessOrderedSortedSetTest, ParallelIteration) {
  constexpr int numThreads = 200;
  int nElements = 10e3;
  auto executor = folly::CPUThreadPoolExecutor(numThreads);

  folly::Synchronized<folly::F14FastMap<int, std::vector<int>>>
      threadIdToAccessElements;

  std::vector<int> allElements;
  allElements.reserve(nElements);
  for (const auto i : folly::irange(nElements)) {
    allElements.push_back(i);
  }

  AccessOrderedSortedSet<int, Comparator> accessOrderedSortedSet(comparator);
  insertToSet(accessOrderedSortedSet, allElements);

  for (const auto i : folly::irange(numThreads)) {
    executor.add([&, i]() {
      // each thread just iterates over the set collects all seen elements
      std::vector<int> seenElements;
      for (auto element : accessOrderedSortedSet) {
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
  EXPECT_EQ(threadIdToAccessElements.rlock()->size(), numThreads);
  for (const auto i : folly::irange(numThreads)) {
    auto& seenElements = threadIdToAccessElements.rlock()->at(i);
    EXPECT_EQ(toOrderedSet(seenElements), toOrderedSet(allElements))
        << " assertion failed on thread " << i;
  }
}

} // namespace facebook::algopt::tests
