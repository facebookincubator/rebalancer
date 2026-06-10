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

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/ScopeDimension.h"

#include <gtest/gtest.h>

using namespace facebook::rebalancer::entities;

TEST(ScopeDimensionTest, Basic) {
  const auto item1 = ScopeItemId(1);
  const auto item2 = ScopeItemId(2);
  const auto item3 = ScopeItemId(3);

  const ScopeDimension dimension({{item1, 10}, {item2, 20}}, 30);

  EXPECT_EQ(10, dimension.getValue(item1));
  EXPECT_EQ(20, dimension.getValue(item2));
  EXPECT_EQ(30, dimension.getValue(item3));

  auto& nonDefaultValues = dimension.getNonDefaultValues();
  EXPECT_EQ(2, nonDefaultValues.size());
  EXPECT_EQ(10, nonDefaultValues.at(item1));
  EXPECT_EQ(20, nonDefaultValues.at(item2));

  EXPECT_EQ(30, dimension.getDefaultValue());
}

TEST(ScopeDimensionTest, ToThrift) {
  const ScopeDimension dimension({{ScopeItemId(0), 10}}, 20);

  auto thrift = dimension.toThrift();

  EXPECT_EQ(1, thrift.values()->size());
  EXPECT_EQ(10, thrift.values()->at(0));
  EXPECT_EQ(20, *thrift.defaultValue());
}

TEST(ScopeDimensionTest, FromThrift) {
  thrift::ScopeDimension thrift;
  thrift.values() = {{0, 10}};
  thrift.defaultValue() = 20;

  const ScopeDimension dimension(thrift);

  EXPECT_EQ(1, dimension.getNonDefaultValues().size());
  EXPECT_EQ(10, dimension.getNonDefaultValues().at(ScopeItemId(0)));
  EXPECT_EQ(20, dimension.getDefaultValue());
}
