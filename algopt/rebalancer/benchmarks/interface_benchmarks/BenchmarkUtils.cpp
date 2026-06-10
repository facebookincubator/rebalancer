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

#include "algopt/rebalancer/benchmarks/interface_benchmarks/BenchmarkUtils.h"

#include "algopt/rebalancer/interface/ProblemSolverFactory.h"

#include <fmt/format.h>
#include <folly/container/irange.h>

namespace facebook::rebalancer::interface::benchmarks {

std::unique_ptr<ProblemSolver> createBaseProblemSolver(
    int numContainers,
    int numObjects,
    bool canExecuteAsync) {
  auto solver = ProblemSolverFactory::makeProblemSolver(
      "rebalancer", "benchmark", canExecuteAsync);
  solver->setObjectName("task");
  solver->setContainerName("host");

  const int baseObjectsPerContainer = numObjects / numContainers;
  const int remainder = numObjects % numContainers;

  std::map<std::string, std::vector<std::string>> containerToObjects;
  int objIdx = 0;
  for (const auto containerIdx : folly::irange(numContainers)) {
    const int objectsCount =
        baseObjectsPerContainer + (containerIdx < remainder ? 1 : 0);
    const std::string container = fmt::format("host_{}", containerIdx);

    std::vector<std::string> objects;
    objects.reserve(objectsCount);
    for (const auto i : folly::irange(objectsCount)) {
      objects.emplace_back(fmt::format("task_{}", objIdx + i));
    }
    containerToObjects[container] = std::move(objects);
    objIdx += objectsCount;
  }

  solver->setAssignment(containerToObjects);
  return solver;
}

folly::F14FastMap<std::string, std::vector<std::string>> createPartitionData(
    int numGroups,
    int numObjects) {
  const int baseObjectsPerGroup = numObjects / numGroups;
  const int remainder = numObjects % numGroups;

  folly::F14FastMap<std::string, std::vector<std::string>> groupToObjects;
  int objIdx = 0;
  for (const auto groupIdx : folly::irange(numGroups)) {
    const int objectsCount =
        baseObjectsPerGroup + (groupIdx < remainder ? 1 : 0);
    const std::string group = fmt::format("group_{}", groupIdx);

    std::vector<std::string> objects;
    objects.reserve(objectsCount);
    for (const auto i : folly::irange(objectsCount)) {
      objects.emplace_back(fmt::format("task_{}", objIdx + i));
    }
    groupToObjects[group] = std::move(objects);
    objIdx += objectsCount;
  }

  return groupToObjects;
}

folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
createDynamicDimensionData(
    int numScopeItems,
    int numObjects,
    bool dimensionBasedOnGroups) {
  folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
      scopeItemToObjectToValue;

  for (const auto scopeIdx : folly::irange(numScopeItems)) {
    const std::string scopeItem = fmt::format("scope_{}", scopeIdx);
    folly::F14FastMap<std::string, double> objectToValue;
    objectToValue.reserve(numObjects);
    for (const auto i : folly::irange(numObjects)) {
      auto object = dimensionBasedOnGroups ? fmt::format("group_{}", i)
                                           : fmt::format("task_{}", i);
      objectToValue.emplace(std::move(object), static_cast<double>(i % 100));
    }
    scopeItemToObjectToValue[scopeItem] = std::move(objectToValue);
  }

  return scopeItemToObjectToValue;
}

} // namespace facebook::rebalancer::interface::benchmarks
