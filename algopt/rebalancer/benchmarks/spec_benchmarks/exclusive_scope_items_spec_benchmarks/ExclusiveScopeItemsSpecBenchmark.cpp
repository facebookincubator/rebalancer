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

#include "algopt/rebalancer/interface/tests/utils.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>
#include <folly/Random.h>

#include <map>

namespace interface = facebook::rebalancer::interface;

static void run(int objectsCount, int containersCount, int objectsPerGroup) {
  auto solver = interface::initializeTestProblemSolver();
  solver->setObjectName("task");
  solver->setContainerName("host");

  std::map<std::string, std::vector<std::string>> initialAssignment;
  for (const auto i : folly::irange(containersCount)) {
    initialAssignment.emplace(
        fmt::format("host{}", i), std::vector<std::string>());
  }
  // Tasks are randomly assigned to hosts in the initial assignment.
  for (const auto i : folly::irange(objectsCount)) {
    auto task = fmt::format("task{}", i);
    auto host =
        fmt::format("host{}", folly::Random::rand32(0, containersCount));
    initialAssignment[host].push_back(task);
  }
  solver->setAssignment(initialAssignment);

  // each group has {objectsPerGroup} tasks
  std::map<std::string, std::string> objectToGroup;
  for (const auto i : folly::irange(objectsCount)) {
    auto task = fmt::format("task{}", i);
    auto group = fmt::format("group{}", i / objectsPerGroup);
    objectToGroup.emplace(task, group);
  }
  solver->addPartition("job", std::move(objectToGroup));

  std::vector<interface::ScopeItemConflictInfo> conflictInfoList;
  for (const auto i : folly::irange(containersCount - 1)) {
    auto conflictInfo = interface::ScopeItemConflictInfo();
    conflictInfo.scopeItem() = fmt::format("host{}", i);
    auto conflictingScopeItemInfo = interface::ConflictingScopeItemInfo();
    conflictingScopeItemInfo.conflictingScopeItem() =
        fmt::format("host{}", i + 1);
    conflictInfo.conflictingScopeItemsWithOverlap() = {
        std::move(conflictingScopeItemInfo)};
    conflictInfoList.push_back(std::move(conflictInfo));
  }

  auto spec = interface::ExclusiveScopeItemsSpec();
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "task_count";
  spec.conflictInfoList() = std::move(conflictInfoList);
  spec.partitionName() = "job";
  solver->addConstraint(std::move(spec));

  interface::OptimalSolverSpec solverSpec;
  solverSpec.solverPackage() = interface::OptimalSolverPackage::GUROBI;
  solver->addSolver(solverSpec);

  solver->solve();
}

BENCHMARK(ExclusiveScopeItemsSpec) {
  run(50, 5000, 100);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
