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

#include "algopt/rebalancer/algopt_common/alias.h"
#include "algopt/rebalancer/algopt_common/tests/gen-cpp2/testdata_types.h"
#include "algopt/rebalancer/algopt_common/TestUtils.h"
#include "algopt/rebalancer/algopt_common/Utils.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"

#include <folly/container/F14Set.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

using namespace facebook::algopt::utils;
namespace facebook::algopt::common::tests {

TEST(UtilsTest, testStruct) {
  TestStruct testStruct;
  TestFieldOne fieldOne;
  TestFieldTwo fieldTwo;
  assignThriftStruct(testStruct, fieldOne);
  assignThriftStruct(testStruct, fieldTwo);
  assignThriftStruct(testStruct, "test_struct");
}

TEST(UtilsTest, setStringFieldToValueIfEmptyOrUnset) {
  // string field is empty
  {
    TestStruct testStruct;
    EXPECT_TRUE(testStruct.field3()->empty());

    setStringFieldToValueIfEmptyOrUnset(
        testStruct, "field3", "test_field_value");
    EXPECT_EQ(testStruct.field3(), "test_field_value");
  }

  // string field is assigned, so expect no change
  {
    TestStruct testStruct;
    testStruct.field3() = "test_field_value";
    EXPECT_FALSE(testStruct.field3()->empty());

    setStringFieldToValueIfEmptyOrUnset(
        testStruct, "field3", "new_field_value");
    EXPECT_EQ(testStruct.field3(), "test_field_value");
  }

  // optional string field is unseyt
  {
    TestStruct testStruct;
    EXPECT_FALSE(testStruct.field4().has_value());

    setStringFieldToValueIfEmptyOrUnset(
        testStruct, "field4", "test_field_value");
    EXPECT_EQ(testStruct.field4(), "test_field_value");
  }

  // optional string field is set, but empty
  {
    TestStruct testStruct;
    testStruct.field4() = "";
    EXPECT_TRUE(testStruct.field4()->empty());

    setStringFieldToValueIfEmptyOrUnset(
        testStruct, "field4", "test_field_value");
    EXPECT_EQ(testStruct.field4(), "test_field_value");
  }

  // optional string field is assigned, so expect no change
  {
    TestStruct testStruct;
    testStruct.field4() = "test_field_value";
    EXPECT_TRUE(testStruct.field4().has_value());

    setStringFieldToValueIfEmptyOrUnset(testStruct, "field4", "new_test");
    EXPECT_EQ(testStruct.field4(), "test_field_value");
  }
}

TEST(UtilsTest, testUnion) {
  TestUnion testUnion;
  TestFieldOne fieldOne;
  assignThriftStruct(testUnion, fieldOne);
  EXPECT_EQ(testUnion.getType(), TestUnion::Type::field1);
  TestFieldTwo fieldTwo;
  auto u = createThriftUnionByField<TestUnion>(fieldTwo);
  EXPECT_EQ(u.getType(), TestUnion::Type::field2);
  const TestUnion empty;
  REBALANCER_EXPECT_RUNTIME_ERROR(
      throwIfUnionIsEmpty(empty),
      "Thrift union of type 'TestUnion' is empty. Please initialize it with a valid type.");
}

TEST(UtilsTest, testFormatAsTable) {
  const std::vector<std::vector<std::string>> table = {
      {"4", "a", "b", "c"},
      {"34", "d", "e", "f"},
      {"234", "g", "h", "i"},
      {"1234", "j", "k", "l"},
  };
  const std::string top = "\n┌───────────────────────────────────────────┐\n";
  const std::string header =
      "│   Key    │    A     │    B     │    C     │\n"
      "│===========================================│\n";
  const std::string rest =
      "│    4     │    a     │    b     │    c     │\n"
      "│───────────────────────────────────────────│\n"
      "│    34    │    d     │    e     │    f     │\n"
      "│───────────────────────────────────────────│\n"
      "│   234    │    g     │    h     │    i     │\n"
      "│───────────────────────────────────────────│\n"
      "│   1234   │    j     │    k     │    l     │\n"
      "└───────────────────────────────────────────┘\n";

  const auto withHeader =
      formatAsPrettyTable(table, {"Key", "A", "B", "C"}, 10);
  EXPECT_EQ(top + header + rest, withHeader);

  const auto withoutHeader = formatAsPrettyTable(table, {}, 10);
  EXPECT_EQ(top + rest, withoutHeader);

  // try sorted by key
  const std::string restSorted =
      "│   1234   │    j     │    k     │    l     │\n"
      "│───────────────────────────────────────────│\n"
      "│   234    │    g     │    h     │    i     │\n"
      "│───────────────────────────────────────────│\n"
      "│    34    │    d     │    e     │    f     │\n"
      "│───────────────────────────────────────────│\n"
      "│    4     │    a     │    b     │    c     │\n"
      "└───────────────────────────────────────────┘\n";

  const auto sortedWithHeader =
      formatAsPrettyTable(table, {"Key", "A", "B", "C"}, 10, /*sortByCol=*/0);
  EXPECT_EQ(top + header + restSorted, sortedWithHeader);
  XLOG(INFO) << sortedWithHeader;

  const auto sortedWithHeaderInc = formatAsPrettyTable(
      table,
      {"Key", "A", "B", "C"},
      10,
      /*sortByCol=*/0,
      /*sortDescending=*/false);
  EXPECT_EQ(top + header + rest, sortedWithHeaderInc);
  XLOG(INFO) << sortedWithHeader;

  // using a string column should sort by string
  const auto sortedWithHeaderCol1 = formatAsPrettyTable(
      table,
      {"Key", "A", "B", "C"},
      10,
      /*sortByCol=*/1);
  EXPECT_EQ(top + header + restSorted, sortedWithHeaderCol1);
}

TEST(UtilsTest, ToVectorTest) {
  std::vector<int> vec = {1, 2, 3};
  auto vec2 = vec | to<std::vector>;
  EXPECT_EQ(vec, vec2);
}

TEST(UtilsTest, ToFollyF14FastSetTest) {
  folly::F14FastSet<int> set = {1, 2, 3};
  auto set2 = set | to<folly::F14FastSet>;
  EXPECT_EQ(set, set2);
}

TEST(UtilsTest, FromKeysToVectorTest) {
  algopt::MapImpl<int, int> map = {{1, 2}, {3, 4}, {5, 6}};
  auto key_vec = map | std::views::keys | to<std::vector>;
  const std::vector<int> expected = {1, 3, 5};
  EXPECT_THAT(key_vec, ::testing::UnorderedElementsAreArray(expected));
}

TEST(UtilsTest, FromEntityIdMapToVector) {
  using GI = rebalancer::entities::GoalId;
  rebalancer::entities::Map<GI, int> map = {{GI(1), 2}, {GI(3), 4}, {GI(5), 6}};
  auto key_vec = map | std::views::keys | to<std::vector>;
  const std::vector<GI> expected = {GI(1), GI(3), GI(5)};
  EXPECT_THAT(key_vec, ::testing::UnorderedElementsAreArray(expected));
}

} // namespace facebook::algopt::common::tests
