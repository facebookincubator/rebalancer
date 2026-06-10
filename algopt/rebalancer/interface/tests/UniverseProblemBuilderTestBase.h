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

#pragma once

#include "algopt/rebalancer/interface/UniverseProblemBuilder.h"

#include <folly/container/F14Map.h>
#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <gtest/gtest.h>
namespace facebook::rebalancer::interface::tests {

class UniverseProblemBuilderTestBase : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    kNumThreads_ = GetParam();
  }

  UniverseProblemBuilder getBuilder() const {
    auto asyncConfig = (kNumThreads_ >= 1)
        ? std::make_unique<AsyncConfig>(
              std::make_shared<folly::CPUThreadPoolExecutor>(kNumThreads_))
        : nullptr;
    return UniverseProblemBuilder(std::move(asyncConfig));
  }

  // Assigns object_i to container_{i % numContainers}.
  static folly::F14FastMap<std::string, std::vector<std::string>>
  generateInitialAssignment(size_t numObjects, size_t numContainers) {
    folly::F14FastMap<std::string, std::vector<std::string>> initialAssignment;
    for (const auto i : folly::irange(numObjects)) {
      const auto containerName = fmt::format("container_{}", i % numContainers);
      initialAssignment[containerName].push_back(fmt::format("object_{}", i));
    }
    for (const auto i : folly::irange(numContainers)) {
      const auto containerName = fmt::format("container_{}", i);
      if (!folly::get_ptr(initialAssignment, containerName)) {
        initialAssignment[containerName] = {};
      }
    }
    return initialAssignment;
  }

  // Adds container_i to scopeItem_{i % numScopeItems}
  static folly::F14FastMap<std::string, std::vector<std::string>>
  generateScopeItemToContainers(size_t numScopeItems, size_t numContainers) {
    folly::F14FastMap<std::string, std::vector<std::string>>
        scopeItemToContainers;
    for (const auto i : folly::irange(numContainers)) {
      const auto scopeItemName = fmt::format("scopeItem_{}", i % numScopeItems);
      scopeItemToContainers[scopeItemName].push_back(
          fmt::format("container_{}", i));
    }
    for (const auto i : folly::irange(numScopeItems)) {
      const auto scopeItemName = fmt::format("scopeItem_{}", i);
      if (!folly::get_ptr(scopeItemToContainers, scopeItemName)) {
        scopeItemToContainers[scopeItemName] = {};
      }
    }
    return scopeItemToContainers;
  }

  // Adds object_i to group_{i % numGroups}
  static folly::F14FastMap<std::string, std::vector<std::string>>
  generateGroupToObjects(size_t numGroups, size_t numObjects) {
    folly::F14FastMap<std::string, std::vector<std::string>> groupToObjects;
    for (const auto i : folly::irange(numObjects)) {
      const auto groupName = fmt::format("group_{}", i % numGroups);
      groupToObjects[groupName].push_back(fmt::format("object_{}", i));
    }
    for (const auto i : folly::irange(numGroups)) {
      const auto groupName = fmt::format("group_{}", i);
      if (!folly::get_ptr(groupToObjects, groupName)) {
        groupToObjects[groupName] = {};
      }
    }
    return groupToObjects;
  }

  // For each scope item, assigns baseValue + (j % 100) to object_j.
  static folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
  generateScopeItemToObjectToValue(
      size_t numScopeItems,
      size_t numObjects,
      double baseValue) {
    folly::F14FastMap<std::string, double> objectToValue;
    for (const auto j : folly::irange(numObjects)) {
      objectToValue.emplace(
          fmt::format("object_{}", j),
          baseValue + static_cast<double>(j % 100));
    }

    folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
        scopeItemToObjectToValue;
    for (const auto i : folly::irange(numScopeItems)) {
      scopeItemToObjectToValue.emplace(
          fmt::format("scopeItem_{}", i), objectToValue);
    }

    return scopeItemToObjectToValue;
  }

  // For each group_j, it has value 'baseValue' + (j % 50) in each scope item
  static folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
  generateScopeItemToGroupToValue(
      size_t numScopeItems,
      size_t numGroups,
      double baseValue) {
    folly::F14FastMap<std::string, double> groupToValue;
    for (const auto j : folly::irange(numGroups)) {
      groupToValue.emplace(
          fmt::format("group_{}", j), baseValue + static_cast<double>(j % 50));
    }

    folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
        scopeItemToGroupToValue;
    for (const auto i : folly::irange(numScopeItems)) {
      scopeItemToGroupToValue.emplace(
          fmt::format("scopeItem_{}", i), groupToValue);
    }
    return scopeItemToGroupToValue;
  }

  /*
  generates a map of object to value, where the value of object i is baseValue +
  (i % 100)
  */
  static folly::F14FastMap<std::string, double> generateObjectToValue(
      size_t numObjects,
      double baseValue) {
    folly::F14FastMap<std::string, double> objectToValue;
    for (const auto i : folly::irange(numObjects)) {
      objectToValue.emplace(
          fmt::format("object_{}", i),
          baseValue + static_cast<double>(i % 100));
    }

    return objectToValue;
  }

  /*
  generates a map of object to vector of values, where object i is mapped to a
  vector with values with baseValue + (i % 100) and baseValue + (i % 50)
  */
  static folly::F14FastMap<std::string, std::vector<double>>
  generateObjectToValues(size_t numObjects, double baseValue) {
    folly::F14FastMap<std::string, std::vector<double>> objectToValues;
    for (const auto j : folly::irange(numObjects)) {
      objectToValues.emplace(
          fmt::format("object_{}", j),
          std::vector<double>{
              baseValue + static_cast<double>(j % 100),
              baseValue + static_cast<double>(j % 50)});
    }
    return objectToValues;
  }

 private:
  int kNumThreads_{1};
};

} // namespace facebook::rebalancer::interface::tests
