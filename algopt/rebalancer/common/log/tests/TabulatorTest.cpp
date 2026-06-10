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

#include "algopt/rebalancer/common/Tabulator.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <iterator>
#include <vector>

using namespace facebook::rebalancer;
using namespace folly;

TEST(TabulatorTest, Empty) {
  Tabulator tb({});
  tb.setHeader({"one", "two"});
  EXPECT_EQ("", tb.string());
}

TEST(TabulatorTest, Tabulate) {
  fmt::memory_buffer buf;
  fmt::format_to(std::back_inserter(buf), "some string:");
  Tabulator tb({});
  tb.setHeader({"one", "two"})
      .addRow(std::initializer_list<int>({1, 2}))
      .tabulate(buf);
  fmt::format_to(std::back_inserter(buf), "another string\n");
  EXPECT_EQ(
      ("some string:\n"
       "one  two  \n"
       "1    2    \n"
       "another string\n"),
      fmt::to_string(buf));
}

TEST(TabulatorTest, DynamicRows) {
  Tabulator tb({});
  tb.setHeader({"one", "two"})
      .addRow(dynamic::array(12345678, 1234))
      .addRow(dynamic::array(false, dynamic::object()))
      .addRow(
          dynamic::array(
              dynamic::object()("one", 1)("2", false), dynamic::array()));
  EXPECT_EQ(
      ("\n"
       "one                  two   \n"
       "12345678             1234  \n"
       "0                          \n"
       "{\"2\":false,\"one\":1}        \n"),
      tb.string());
}

TEST(TabulatorTest, CommonRows) {
  Tabulator tb({.rowSeparator = std::nullopt, .colSeparator = '|'});
  tb.addRow(std::initializer_list<int>{1, 2})
      .addRow(std::initializer_list<double>{0.001234, 1234.56789})
      .addRow(
          std::initializer_list<std::string>{"first column", "second column"});

  EXPECT_EQ(
      ("\n"
       "|1             |2              |\n"
       "|0.001234      |1234.56789     |\n"
       "|first column  |second column  |\n"),
      tb.string());
}

TEST(TabulatorTest, WithSeparators) {
  Tabulator tb({.rowSeparator = '-', .colSeparator = '|'});
  tb.setHeader({"one", "two"})
      .addRow(dynamic::array(12345.678, 1234))
      .addRow(
          std::initializer_list<std::string>{"a", "b", "another rogue column"});
  EXPECT_EQ(
      ("\n"
       "-------------------------------------------\n"
       "|one        |two   |\n"
       "-------------------------------------------\n"
       "|12345.678  |1234  |\n"
       "-------------------------------------------\n"
       "|a          |b     |another rogue column  |\n"
       "-------------------------------------------\n"),
      tb.string());
}
