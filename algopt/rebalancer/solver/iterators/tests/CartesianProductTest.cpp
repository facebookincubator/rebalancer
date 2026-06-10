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

#include "algopt/rebalancer/solver/iterators/CartesianProduct.h"

#include <gtest/gtest.h>

#include <array>
#include <list>

namespace facebook::rebalancer::packer::tests {

// ==================== ORIGINAL CORE TESTS ====================

TEST(CartesianProductTest, Basic) {
  const std::vector<std::string> v1 = {"a", "b"};
  const std::vector<int> v2 = {1, 2};
  const CartesianProduct<std::vector<std::string>, std::vector<int>> product(
      v1, v2);
  const std::vector<std::pair<std::string, int>> output(
      product.begin(), product.end());
  const std::vector<std::pair<std::string, int>> expected = {
      {"a", 1}, {"a", 2}, {"b", 1}, {"b", 2}};
  EXPECT_EQ(expected, output);
}

TEST(CartesianProductTest, FirstEmpty) {
  const std::vector<std::string> v1 = {};
  const std::vector<int> v2 = {1, 2};
  const CartesianProduct<std::vector<std::string>, std::vector<int>> product(
      v1, v2);
  const std::vector<std::pair<std::string, int>> output(
      product.begin(), product.end());
  ASSERT_EQ(0, output.size());
}

TEST(CartesianProductTest, SecondEmpty) {
  const std::vector<std::string> v1 = {"a", "b"};
  const std::vector<int> v2 = {};
  const CartesianProduct<std::vector<std::string>, std::vector<int>> product(
      v1, v2);
  const std::vector<std::pair<std::string, int>> output(
      product.begin(), product.end());
  ASSERT_EQ(0, output.size());
}

TEST(CartesianProductTest, BothEmpty) {
  const std::vector<std::string> v1 = {};
  const std::vector<int> v2 = {};
  const CartesianProduct<std::vector<std::string>, std::vector<int>> product(
      v1, v2);
  const std::vector<std::pair<std::string, int>> output(
      product.begin(), product.end());
  ASSERT_EQ(0, output.size());
}

TEST(CartesianProductTest, Iterators) {
  std::vector<std::string> v1 = {"a", "b"};
  std::vector<int> v2 = {1};
  const CartesianProduct<std::vector<std::string>, std::vector<int>> product(
      v1.begin(), v1.end(), v2.begin(), v2.end());
  const std::vector<std::pair<std::string, int>> output(
      product.begin(), product.end());
  const std::vector<std::pair<std::string, int>> expected = {
      {"a", 1}, {"b", 1}};
  EXPECT_EQ(expected, output);
}

// ==================== RANDOM ACCESS TESTS ====================

TEST(CartesianProductTest, RandomAccessOperations) {
  const auto v1 = std::vector<int>{10, 20, 30};
  const auto v2 = std::vector<char>{'x', 'y'};
  const auto product = CartesianProduct(v1, v2);

  // Test size
  EXPECT_EQ(6, product.size());

  // Test indexing
  {
    const auto it = product.begin();
    EXPECT_EQ(std::make_pair(10, 'x'), it[0]);
    EXPECT_EQ(std::make_pair(20, 'y'), it[3]);
    EXPECT_EQ(std::make_pair(30, 'y'), it[5]);
  }

  // Test arithmetic operations
  {
    auto it = product.begin();
    it += 2;
    EXPECT_EQ(std::make_pair(20, 'x'), *it);
    EXPECT_EQ(2, (product.begin() + 4) - it);
  }

  // Test comparisons
  {
    const auto it = product.begin();
    const auto it2 = product.begin() + 1;
    EXPECT_TRUE(it < it2);
    EXPECT_TRUE(it2 != it);
  }

  // Test bidirectional movement
  {
    auto it = product.begin();
    it += 2;
    ++it;
    EXPECT_EQ(std::make_pair(20, 'y'), *it);
    --it;
    EXPECT_EQ(std::make_pair(20, 'x'), *it);
  }
}

TEST(CartesianProductTest, RandomAccessWithArrays) {
  const auto a1 = std::array<int, 2>{1, 2};
  const auto a2 = std::array<char, 3>{'a', 'b', 'c'};
  const auto product = CartesianProduct(a1, a2);

  EXPECT_EQ(6, product.size());
  const auto it = product.begin();
  EXPECT_EQ(std::make_pair(1, 'a'), it[0]);
  EXPECT_EQ(std::make_pair(2, 'c'), it[5]);
}

TEST(CartesianProductTest, RandomAccessBoundaryConditions) {
  const auto v1 = std::vector<int>{1, 2};
  const auto v2 = std::vector<char>{'a', 'b'};
  const auto product = CartesianProduct(v1, v2);

  // Test valid range operations
  {
    const auto begin = product.begin();
    const auto end = product.end();
    EXPECT_EQ(4, end - begin);
    EXPECT_TRUE(begin < end);
  }

  // Test arithmetic at boundaries
  {
    const auto begin = product.begin();
    const auto end = product.end();

    // Moving from begin
    const auto it1 = begin + 1;
    EXPECT_EQ(std::make_pair(1, 'b'), *it1);

    // Moving towards end
    const auto it2 = end - 1;
    EXPECT_EQ(std::make_pair(2, 'b'), *it2);

    // Test subtraction
    EXPECT_EQ(1, it1 - begin);
    EXPECT_EQ(3, end - it1);
  }

  // Note: Testing begin() - 1 or end() + 1 would be undefined behavior
  // as they would create invalid iterators outside the valid range.
  // These operations are technically possible but should not be dereferenced.
}

// ==================== FORWARD-ONLY TESTS ====================

TEST(CartesianProductTest, ForwardOnlyWithLists) {
  const auto l1 = std::list<int>{1, 2};
  const auto l2 = std::list<char>{'a', 'b'};
  const auto product =
      CartesianProduct<std::list<int>, std::list<char>>(l1, l2);

  const auto output =
      std::vector<std::pair<int, char>>(product.begin(), product.end());
  const auto expected =
      std::vector<std::pair<int, char>>{{1, 'a'}, {1, 'b'}, {2, 'a'}, {2, 'b'}};

  EXPECT_EQ(expected, output);
}

TEST(CartesianProductTest, MixedAccessTypes) {
  // Mix vector (random access) with list (forward only) -> should use forward
  // iterator
  const auto v1 = std::vector<int>{1, 2};
  const auto l2 = std::list<char>{'x', 'y'};
  const auto product = CartesianProduct(v1, l2);

  const auto output =
      std::vector<std::pair<int, char>>(product.begin(), product.end());
  const auto expected =
      std::vector<std::pair<int, char>>{{1, 'x'}, {1, 'y'}, {2, 'x'}, {2, 'y'}};

  EXPECT_EQ(expected, output);
}

// ==================== ITERATOR CATEGORY VERIFICATION ====================

TEST(CartesianProductTest, VerifyIteratorCategories) {
  {
    // Random access collections -> random access iterator
    const auto v1 = std::vector<int>{1, 2};
    const auto v2 = std::vector<int>{3, 4};
    const auto ra_product = CartesianProduct(v1, v2);

    const auto ra_it = ra_product.begin();
    static_assert(std::is_same_v<
                  typename decltype(ra_it)::iterator_category,
                  std::random_access_iterator_tag>);
  }

  {
    // Forward-only collections -> input iterator
    const auto l1 = std::list<int>{1, 2};
    const auto l2 = std::list<int>{3, 4};
    const auto fo_product = CartesianProduct(l1, l2);

    const auto fo_it = fo_product.begin();
    static_assert(std::is_same_v<
                  typename decltype(fo_it)::iterator_category,
                  std::input_iterator_tag>);
  }
}

} // namespace facebook::rebalancer::packer::tests
