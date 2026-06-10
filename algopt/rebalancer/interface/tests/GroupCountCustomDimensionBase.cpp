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

#include "algopt/rebalancer/interface/tests/GroupCountCustomDimensionBase.h"

#include "algopt/rebalancer/interface/tests/utils.h"

#include <fmt/core.h>
#include <folly/container/irange.h>

using namespace facebook::rebalancer::interface;

std::unique_ptr<ProblemSolver> buildProblem(
    int objectCount,
    int containerCount,
    int groupCount,
    bool absoluteLimit,
    int executorThreadCount) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = executorThreadCount});
  solver->setObjectName("task");
  solver->setContainerName("host");

  std::map<std::string, std::vector<std::string>> initialAssignment;
  for (const auto i : folly::irange(containerCount)) {
    initialAssignment[fmt::format("host{}", i)] = {};
  }

  auto initialHostId = [&](int objectId) {
    return (objectId / 3) % containerCount;
  };

  for (const auto i : folly::irange(objectCount)) {
    auto host = fmt::format("host{}", initialHostId(i));
    auto task = fmt::format("task{}", i);
    initialAssignment[host].push_back(task);
  }
  solver->setAssignment(initialAssignment);

  std::map<std::string, std::string> taskToJob;
  std::unordered_map<std::string, std::vector<int>> jobToTasksIds;
  for (const auto i : folly::irange(objectCount)) {
    auto task = fmt::format("task{}", i);
    auto job = fmt::format("job{}", i * groupCount / objectCount);
    taskToJob[task] = job;
    jobToTasksIds[job].push_back(i);
  }
  solver->addPartition("job", taskToJob);

  std::map<std::string, std::map<std::string, double>> dynamicObjectDimension;
  // the dynamicDimension below is defined in a such a way that for all the
  // test cases in GroupCountDimensionTest, the dynamic dimension defined below
  // is equivalent to static dimension where each task has dimension value ((10
  // * taskId) / double(objectCount - 1));

  // Skipping task0 on purpose which would have a weight of 0 to test the edge
  // where some of the objects don't have an explicit dimension value and they
  // are expected to get the default value of 0.
  for (int objId = 1 /* skip objId=0 */; objId < objectCount; ++objId) {
    // all tasks that are part of the same job have the same value in any host
    // that has at least one of them
    auto task = fmt::format("task{}", objId);
    auto initialHost = fmt::format("host{}", initialHostId(objId));
    auto& job = taskToJob.at(task);
    for (auto relatedTaskId : jobToTasksIds.at(job)) {
      auto relatedTask = fmt::format("task{}", relatedTaskId);
      dynamicObjectDimension[initialHost][relatedTask] =
          (10 * relatedTaskId) / double(objectCount - 1);
    }
  }
  solver->addDynamicObjectDimension(
      "dim", "host", std::move(dynamicObjectDimension));

  auto spec = GroupCountSpec();
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.dimension() = "dim";
  spec.limit()->type() =
      absoluteLimit ? LimitType::ABSOLUTE : LimitType::RELATIVE;
  spec.limit()->globalLimit() = 0.1;

  solver->addGoal(std::move(spec));

  return solver;
}
