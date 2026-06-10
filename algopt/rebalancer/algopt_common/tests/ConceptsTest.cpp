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

#include "algopt/rebalancer/algopt_common/Concepts.h"
#include "algopt/rebalancer/algopt_common/tests/gen-cpp2/testdata_types.h"

#include "folly/concurrency/ConcurrentHashMap.h"
#include "folly/container/F14Map.h"
#include <gtest/gtest.h>

using namespace facebook::algopt::common::tests;

TEST(ConceptsTest, BasicIsIterableOverPairs) {
  static_assert(
      IsIterableOverPairs<std::vector<std::pair<int, int>>, int, int>);
  static_assert(
      IsIterableOverPairs<std::unordered_set<std::pair<int, int>>, int, int>);
  static_assert(IsIterableOverPairs<
                std::set<std::pair<std::string, std::string>>,
                std::string,
                std::string>);

  static_assert(IsIterableOverPairs<std::unordered_map<int, int>, int, int>);
  static_assert(
      IsIterableOverPairs<std::map<int, std::string>, int, std::string>);
  static_assert(IsIterableOverPairs<
                folly::F14FastMap<std::string, int>,
                std::string,
                int>);

  // errors
  static_assert(!IsIterableOverPairs<std::vector<int>, int, int>);
  static_assert(
      !IsIterableOverPairs<std::vector<std::pair<int, int>>, std::string, int>);
  static_assert(!IsIterableOverPairs<std::map<int, int>, int, std::string>);
  // concurrentHashMap returns an error because it does not have
  // Container::iterator
  static_assert(
      !IsIterableOverPairs<folly::ConcurrentHashMap<int, int>, int, int>);
}

TEST(ConceptsTest, BasicIsMap) {
  static_assert(IsMap<std::unordered_map<int, int>, int, int>);
  static_assert(IsMap<std::map<int, std::string>, int, std::string>);
  static_assert(
      IsMap<folly::F14FastMap<std::string, double>, std::string, double>);

  // errors
  static_assert(!IsMap<folly::F14FastMap<double, double>, double, std::string>);
  static_assert(!IsMap<folly::F14FastMap<double, double>, std::string, double>);
  // concurrentHashMap returns an error because it does not have
  // Container::iterator
  static_assert(!IsMap<folly::ConcurrentHashMap<int, int>, int, int>);
}

TEST(ConceptsTest, BasicIsMapOfMap) {
  static_assert(IsMapOfMap<
                folly::F14FastMap<std::string, std::map<std::string, double>>,
                std::string,
                std::string,
                double>);
  static_assert(
      IsMapOfMap<
          std::unordered_map<int, std::unordered_map<std::string, double>>,
          int,
          std::string,
          double>);

  // errors
  static_assert(
      !IsMapOfMap<
          folly::
              F14FastMap<std::string, folly::F14FastMap<std::string, double>>,
          std::string,
          std::string,
          int>);
  static_assert(!IsMapOfMap<
                folly::F14FastMap<std::string, std::pair<std::string, double>>,
                std::string,
                std::string,
                double>);
}

template <typename T>
void testThriftDataType() {
  struct NonExistentType {};
  static_assert(FieldTypeAssignableToThriftStructOrUnion<T, TestFieldOne>);
  static_assert(FieldTypeExistsInThriftStructOrUnion<T, TestFieldOne>);
  static_assert(FieldTypeAssignableToThriftStructOrUnion<T, TestFieldTwo>);
  static_assert(FieldTypeExistsInThriftStructOrUnion<T, TestFieldTwo>);
  // Struct contains field which is a thrift string, both std::string and
  // const char* are valid for assignment, only std::string is valid for exact
  static_assert(FieldTypeAssignableToThriftStructOrUnion<T, std::string>);
  static_assert(FieldTypeExistsInThriftStructOrUnion<T, std::string>);
  static_assert(FieldTypeAssignableToThriftStructOrUnion<T, const char*>);
  static_assert(!FieldTypeExistsInThriftStructOrUnion<T, const char*>);
  static_assert(!FieldTypeAssignableToThriftStructOrUnion<T, NonExistentType>);
}

TEST(ConceptsTest, ThriftTypeContainsField) {
  testThriftDataType<TestUnion>();
  testThriftDataType<TestStruct>();
}
