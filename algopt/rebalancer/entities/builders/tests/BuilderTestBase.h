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

#include "algopt/rebalancer/entities/builders/AssignmentBuilder.h"
#include "algopt/rebalancer/entities/builders/IdBuilder.h"
#include "algopt/rebalancer/entities/Constraint.h"
#include "algopt/rebalancer/entities/Goal.h"

#include <folly/Benchmark.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/Random.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::entities::tests {

inline double fib(std::uint32_t n) {
  if (n <= 2) {
    return n;
  }
  return fib(n - 1) + fib(n - 2);
}

inline void executeRandomDelay() {
  const auto n = folly::Random::rand32(15);
  const auto fibN = fib(n);
  folly::doNotOptimizeAway(fibN);
}

struct ProblemConfig {
  size_t numObjects;
  size_t numContainers;
};

class BuilderTestBase : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    kNumThreads = GetParam();
    executor = std::make_shared<folly::CPUThreadPoolExecutor>(kNumThreads);
  }

  static Map<std::string, std::vector<std::string>> generateInitialAssignment(
      const ProblemConfig& config) {
    // assign object i to container i % numContainers
    Map<std::string, std::vector<std::string>> initialAssignment;
    for (const auto i : folly::irange(config.numObjects)) {
      auto objectName = fmt::format("object_{}", i);
      auto containerName =
          fmt::format("container_{}", i % config.numContainers);
      initialAssignment[std::move(containerName)].push_back(
          std::move(objectName));
    }
    for (const auto i : folly::irange(config.numContainers)) {
      auto containerName = fmt::format("container_{}", i);
      const auto objectsPtr = folly::get_ptr(initialAssignment, containerName);
      if (!objectsPtr) {
        initialAssignment[std::move(containerName)] = {};
      }
    }

    return initialAssignment;
  }

  static Map<std::string, std::vector<std::string>>
  generateScopeItemNameToContainerNames(
      size_t nScopeItems,
      const ContainersData& containers) {
    Map<std::string, std::vector<std::string>> scopeItemNameToContainerNames;
    scopeItemNameToContainerNames.reserve(
        static_cast<decltype(scopeItemNameToContainerNames)::size_type>(
            nScopeItems));
    for (const auto& [containerName, containerId] :
         containers.containerNameToId) {
      const auto scopeItemName = fmt::format(
          "scopeItem_{}", static_cast<size_t>(containerId) % nScopeItems);
      scopeItemNameToContainerNames[scopeItemName].push_back(containerName);
    }

    for (const auto i : folly::irange(nScopeItems)) {
      auto scopeItemName = fmt::format("scopeItem_{}", i);
      const auto containersPtr =
          folly::get_ptr(scopeItemNameToContainerNames, scopeItemName);
      if (!containersPtr) {
        scopeItemNameToContainerNames[std::move(scopeItemName)] = {};
      }
    }

    return scopeItemNameToContainerNames;
  }

  static Map<std::string, std::vector<std::string>>
  generateGroupNameToObjectNames(size_t nGroups, const ObjectsData& objects) {
    Map<std::string, std::vector<std::string>> groupNameToObjectNames;
    groupNameToObjectNames.reserve(
        static_cast<decltype(groupNameToObjectNames)::size_type>(nGroups));
    for (const auto& [objectName, objectId] : objects.objectNameToId) {
      auto groupName =
          fmt::format("group_{}", static_cast<size_t>(objectId) % nGroups);
      groupNameToObjectNames[std::move(groupName)].push_back(objectName);
    }

    for (const auto i : folly::irange(nGroups)) {
      auto groupName = fmt::format("group_{}", i);
      const auto groupsPtr = folly::get_ptr(groupNameToObjectNames, groupName);
      if (!groupsPtr) {
        groupNameToObjectNames[std::move(groupName)] = {};
      }
    }

    return groupNameToObjectNames;
  }

  static Map<std::string, std::string> generateObjectNameToGroupName(
      size_t nGroups,
      const ObjectsData& objects) {
    Map<std::string, std::string> objectNameToGroupName;
    objectNameToGroupName.reserve(objects.objectNameToId.size());
    for (const auto& [objectName, objectId] : objects.objectNameToId) {
      auto groupName =
          fmt::format("group_{}", static_cast<size_t>(objectId) % nGroups);
      objectNameToGroupName.emplace(objectName, std::move(groupName));
    }
    return objectNameToGroupName;
  }

  static std::unique_ptr<Goal> makeGoal(double weight, int tupleIndex) {
    interface::GoalSpecs spec;
    spec.capacitySpec().emplace();
    return std::make_unique<Goal>(std::move(spec), weight, tupleIndex);
  }

  static std::unique_ptr<Constraint>
  makeConstraint(double invalidCost, double invalidState, int tupleIndex) {
    interface::ConstraintSpecs spec;
    spec.capacitySpec().emplace();
    return std::make_unique<Constraint>(
        std::move(spec),
        interface::ConstraintPolicy::HARD,
        invalidCost,
        invalidState,
        tupleIndex);
  }

  IdBuilder idBuilder_;
  std::shared_ptr<folly::CPUThreadPoolExecutor> executor;
  int kNumThreads{1};
};

} // namespace facebook::rebalancer::entities::tests
