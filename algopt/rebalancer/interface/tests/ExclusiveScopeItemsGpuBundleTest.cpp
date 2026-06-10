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

#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/interface/tests/utils.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/Types_types.h"
#include "algopt/rebalancer/solver/utils/Util.h"

#include <gtest/gtest.h>

namespace entities = facebook::rebalancer::entities;
namespace facebook::rebalancer::interface::tests {

class ExclusiveScopeItemsGpuBundleTest : public ::testing::TestWithParam<int> {
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    ExclusiveScopeItemsGpuBundleTest,
    testThreadCounts());

std::vector<ScopeItemConflictInfo> makeConflictInfoList(
    const std::vector<
        std::pair<std::string, std::vector<std::pair<std::string, double>>>>&
        scopeItems) {
  std::vector<ScopeItemConflictInfo> result;
  for (auto& [scopeItem, conflictingScopeItems] : scopeItems) {
    std::vector<ConflictingScopeItemInfo> conflictingScopeItemInfoList;
    for (auto& [conflictingScopeItem, overlap] : conflictingScopeItems) {
      auto conflictingScopeItemInfo = ConflictingScopeItemInfo();
      conflictingScopeItemInfo.conflictingScopeItem() = conflictingScopeItem;
      conflictingScopeItemInfo.overlap() = overlap;
      conflictingScopeItemInfoList.push_back(
          std::move(conflictingScopeItemInfo));
    }
    auto info = ScopeItemConflictInfo();
    info.scopeItem() = scopeItem;
    info.conflictingScopeItemsWithOverlap() = conflictingScopeItemInfoList;
    result.push_back(std::move(info));
  }
  return result;
}

std::unique_ptr<ProblemSolver> getSolver(
    int executorThreadCount,
    std::map<std::string, std::vector<std::string>>& assignment,
    std::map<std::string, std::map<std::string, double>>& avoidItems,
    std::vector<ScopeItemConflictInfo>& conflictInfoList,
    std::map<std::string, double>& scopeItemWeights,
    ExclusiveScopeItemsFormula formula) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = executorThreadCount});
  solver->setObjectName("task");
  solver->setContainerName("bundle");
  solver->setAssignment(assignment);
  solver->addDynamicObjectDimension("avoid", "bundle", avoidItems, 1.0);

  { // prevent overlapping bundles
    auto spec = ExclusiveScopeItemsSpec();
    spec.name() = "prevent overlapping";
    spec.scope() = "bundle";
    spec.dimension() = "task_count";
    spec.conflictInfoList() = conflictInfoList;
    solver->addConstraint(std::move(spec));
  }

  { // minimize invalidated bundles
    auto spec = ExclusiveScopeItemsSpec();
    spec.name() = "goal";
    spec.scope() = "bundle";
    spec.dimension() = "task_count";
    spec.conflictInfoList() = conflictInfoList;
    spec.scopeItemWeights() = scopeItemWeights;
    spec.formula() = formula;
    solver->addGoal(std::move(spec));
  }

  { // limit tasks per bundle to 1
    auto spec = CapacitySpec();
    spec.scope() = "bundle";
    spec.dimension() = "task_count";
    Limit limit;
    limit.type() = LimitType::ABSOLUTE;
    limit.globalLimit() = 1.0;
    spec.limit() = limit;
    solver->addConstraint(std::move(spec));
  }

  { // prevent tasks from being assigned to wrong bundles
    auto spec = CapacitySpec();
    spec.scope() = "bundle";
    spec.dimension() = "avoid";
    Limit limit;
    limit.type() = LimitType::ABSOLUTE;
    limit.globalLimit() = 0.0;
    spec.limit() = limit;
    solver->addConstraint(std::move(spec));
  }

  return solver;
}

std::unique_ptr<ProblemSolver> getSolverFor8GPU(
    int executorThreadCount,
    std::vector<std::pair<std::string, std::string>>& initialAssignment,
    ExclusiveScopeItemsFormula formula =
        ExclusiveScopeItemsFormula::MINIMIZE_INVALIDATED_SCOPE_ITEMS_COUNT) {
  std::map<std::string, std::map<std::string, double>> avoidItems;
  std::map<std::string, std::vector<std::string>> assignment = {
      {"g8a", {}},
      {"g4a", {}},
      {"g4b", {}},
      {"g2a", {}},
      {"g2b", {}},
      {"g2c", {}},
      {"g2d", {}},
      {"g1a", {}},
      {"g1b", {}},
      {"g1c", {}},
      {"g1d", {}},
      {"g1e", {}},
      {"g1f", {}},
      {"g1g", {}},
      {"g1h", {}},
  };

  // asssign tasks and specify the bundles that each task can be assigned to
  for (const auto& [bundle, task] : initialAssignment) {
    assignment[bundle].push_back(task);
    if (bundle.starts_with("g8")) {
      continue;
    } else if (bundle.starts_with("g4")) {
      avoidItems["g4a"][task] = 0.0;
      avoidItems["g4b"][task] = 0.0;
    } else if (bundle.starts_with("g2")) {
      avoidItems["g2a"][task] = 0.0;
      avoidItems["g2b"][task] = 0.0;
      avoidItems["g2c"][task] = 0.0;
      avoidItems["g2d"][task] = 0.0;
    } else if (bundle.starts_with("g1")) {
      avoidItems["g1a"][task] = 0.0;
      avoidItems["g1b"][task] = 0.0;
      avoidItems["g1c"][task] = 0.0;
      avoidItems["g1d"][task] = 0.0;
      avoidItems["g1e"][task] = 0.0;
      avoidItems["g1f"][task] = 0.0;
      avoidItems["g1g"][task] = 0.0;
      avoidItems["g1h"][task] = 0.0;
    }
  }

  const std::vector<
      std::pair<std::string, std::vector<std::pair<std::string, double>>>>
      conflictList = {
          {"g8a",
           {{"g4a", 4},
            {"g4b", 4},
            {"g2a", 2},
            {"g2b", 2},
            {"g2c", 2},
            {"g2d", 2},
            {"g1a", 1},
            {"g1b", 1},
            {"g1c", 1},
            {"g1d", 1},
            {"g1e", 1},
            {"g1f", 1},
            {"g1g", 1},
            {"g1h", 1}}},
          {"g4a",
           {{"g2a", 2},
            {"g2b", 2},
            {"g1a", 1},
            {"g1b", 1},
            {"g1c", 1},
            {"g1d", 1}}},
          {"g4b",
           {{"g2c", 2},
            {"g2d", 2},
            {"g1e", 1},
            {"g1f", 1},
            {"g1g", 1},
            {"g1h", 1}}},
          {"g2a", {{"g1a", 1}, {"g1b", 1}}},
          {"g2b", {{"g1c", 1}, {"g1d", 1}}},
          {"g2c", {{"g1e", 1}, {"g1f", 1}}},
          {"g2d", {{"g1g", 1}, {"g1h", 1}}}};
  auto conflictInfoList = makeConflictInfoList(conflictList);

  std::map<std::string, double> scopeItemWeights = {
      {"g8a", 8},
      {"g4a", 4},
      {"g4b", 4},
      {"g2a", 2},
      {"g2b", 2},
      {"g2c", 2},
      {"g2d", 2}};

  return getSolver(
      executorThreadCount,
      assignment,
      avoidItems,
      conflictInfoList,
      scopeItemWeights,
      formula);
}

std::unique_ptr<ProblemSolver> getSolverFor12GPU(
    int executorThreadCount,
    std::vector<std::pair<std::string, std::string>>& initialAssignment,
    ExclusiveScopeItemsFormula formula =
        ExclusiveScopeItemsFormula::MINIMIZE_INVALIDATED_SCOPE_ITEMS_COUNT) {
  std::map<std::string, std::map<std::string, double>> avoidItems;
  std::map<std::string, std::vector<std::string>> assignment = {
      {"g12a", {}}, {"g6a", {}}, {"g6b", {}}, {"g4a", {}}, {"g4b", {}},
      {"g4c", {}},  {"g3a", {}}, {"g3b", {}}, {"g3c", {}}, {"g3d", {}},
      {"g2a", {}},  {"g2b", {}}, {"g2c", {}}, {"g2d", {}}, {"g2e", {}},
      {"g2f", {}},  {"g1a", {}}, {"g1b", {}}, {"g1c", {}}, {"g1d", {}},
      {"g1e", {}},  {"g1f", {}}, {"g1g", {}}, {"g1h", {}}, {"g1i", {}},
      {"g1j", {}},  {"g1k", {}}, {"g1l", {}},
  };

  // asssign tasks and specify the bundles that each task can be assigned to
  for (const auto& [bundle, task] : initialAssignment) {
    assignment[bundle].push_back(task);
    if (bundle.starts_with("g12")) {
      continue;
    } else if (bundle.starts_with("g6")) {
      avoidItems["g6a"][task] = 0.0;
      avoidItems["g6b"][task] = 0.0;
    } else if (bundle.starts_with("g4")) {
      avoidItems["g4a"][task] = 0.0;
      avoidItems["g4b"][task] = 0.0;
      avoidItems["g4c"][task] = 0.0;
    } else if (bundle.starts_with("g3")) {
      avoidItems["g3a"][task] = 0.0;
      avoidItems["g3b"][task] = 0.0;
      avoidItems["g3c"][task] = 0.0;
      avoidItems["g3d"][task] = 0.0;
    } else if (bundle.starts_with("g2")) {
      avoidItems["g2a"][task] = 0.0;
      avoidItems["g2b"][task] = 0.0;
      avoidItems["g2c"][task] = 0.0;
      avoidItems["g2d"][task] = 0.0;
      avoidItems["g2e"][task] = 0.0;
      avoidItems["g2f"][task] = 0.0;
    } else if (bundle.starts_with("g1")) {
      avoidItems["g1a"][task] = 0.0;
      avoidItems["g1b"][task] = 0.0;
      avoidItems["g1c"][task] = 0.0;
      avoidItems["g1d"][task] = 0.0;
      avoidItems["g1e"][task] = 0.0;
      avoidItems["g1f"][task] = 0.0;
      avoidItems["g1g"][task] = 0.0;
      avoidItems["g1h"][task] = 0.0;
      avoidItems["g1i"][task] = 0.0;
      avoidItems["g1j"][task] = 0.0;
      avoidItems["g1k"][task] = 0.0;
      avoidItems["g1l"][task] = 0.0;
    }
  }

  const std::vector<
      std::pair<std::string, std::vector<std::pair<std::string, double>>>>
      conflictList = {
          {"g12a", {{"g6a", 6}, {"g6b", 6}, {"g4a", 4}, {"g4b", 4}, {"g4c", 4},
                    {"g3a", 3}, {"g3b", 3}, {"g3c", 3}, {"g3d", 3}, {"g2a", 2},
                    {"g2b", 2}, {"g2c", 2}, {"g2d", 2}, {"g2e", 2}, {"g2f", 2},
                    {"g1a", 1}, {"g1b", 1}, {"g1c", 1}, {"g1d", 1}, {"g1e", 1},
                    {"g1f", 1}, {"g1g", 1}, {"g1h", 1}, {"g1i", 1}, {"g1j", 1},
                    {"g1k", 1}, {"g1l", 1}}},
          {"g6a",
           {{"g4a", 4},
            {"g4b", 2},
            {"g3a", 3},
            {"g3b", 3},
            {"g2a", 2},
            {"g2b", 2},
            {"g2c", 2},
            {"g1a", 1},
            {"g1b", 1},
            {"g1c", 1},
            {"g1d", 1},
            {"g1e", 1},
            {"g1f", 1}}},
          {"g6b",
           {{"g4b", 2},
            {"g4c", 4},
            {"g3c", 3},
            {"g3d", 3},
            {"g2d", 2},
            {"g2e", 2},
            {"g2f", 2},
            {"g1g", 1},
            {"g1h", 1},
            {"g1i", 1},
            {"g1j", 1},
            {"g1k", 1},
            {"g1l", 1}}},
          {"g4a",
           {{"g3a", 3},
            {"g3b", 1},
            {"g2a", 2},
            {"g2b", 2},
            {"g1a", 1},
            {"g1b", 1},
            {"g1c", 1},
            {"g1d", 1}}},
          {"g4b",
           {{"g3b", 2},
            {"g3c", 2},
            {"g2c", 2},
            {"g2d", 2},
            {"g1e", 1},
            {"g1f", 1},
            {"g1g", 1},
            {"g1h", 1}}},
          {"g4c",
           {{"g3c", 1},
            {"g3d", 3},
            {"g2e", 2},
            {"g2f", 2},
            {"g1i", 1},
            {"g1j", 1},
            {"g1k", 1},
            {"g1l", 1}}},
          {"g3a", {{"g2a", 2}, {"g2b", 1}, {"g1a", 1}, {"g1b", 1}, {"g1c", 1}}},
          {"g3b", {{"g2b", 1}, {"g2c", 2}, {"g1d", 1}, {"g1e", 1}, {"g1f", 1}}},
          {"g3c", {{"g2d", 2}, {"g2e", 1}, {"g1g", 1}, {"g1h", 1}, {"g1i", 1}}},
          {"g3d", {{"g2e", 1}, {"g2f", 2}, {"g1j", 1}, {"g1k", 1}, {"g1l", 1}}},
          {"g2a", {{"g1a", 1}, {"g1b", 1}}},
          {"g2b", {{"g1c", 1}, {"g1d", 1}}},
          {"g2c", {{"g1e", 1}, {"g1f", 1}}},
          {"g2d", {{"g1g", 1}, {"g1h", 1}}},
          {"g2e", {{"g1i", 1}, {"g1j", 1}}},
          {"g2f", {{"g1k", 1}, {"g1l", 1}}}};
  auto conflictInfoList = makeConflictInfoList(conflictList);

  std::map<std::string, double> scopeItemWeights = {
      {"g12a", 12},
      {"g6a", 6},
      {"g6b", 6},
      {"g4a", 4},
      {"g4b", 4},
      {"g4c", 4},
      {"g3a", 3},
      {"g3b", 3},
      {"g3c", 3},
      {"g3d", 3},
      {"g2a", 2},
      {"g2b", 2},
      {"g2c", 2},
      {"g2d", 2},
      {"g2e", 2},
      {"g2f", 2}};

  return getSolver(
      executorThreadCount,
      assignment,
      avoidItems,
      conflictInfoList,
      scopeItemWeights,
      formula);
}

TEST_P(
    ExclusiveScopeItemsGpuBundleTest,
    ExclusiveScopeItemsConstraintWithMinimizeInvalidatedGoalFor8GPU) {
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T0┃g1a   ┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2a┃┃*T1┃g1b   ┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃*T2┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g4a┃┃g2b┃┃   ┃g1d   ┃   ┃┃g4a┃┃g2b┃┃*T3┃g1d
    // ┃   ┃┗━━━┛┗━━━┛┗━━━┛      ┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2c┃┃   ┃g1f   ┃   ┃┃   ┃┃g2c┃┃   ┃g1f
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛   -> ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T2┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃g8a┃┃g4b┃┃g2d┃┃*T3┃g1h   ┃g8a┃┃g4b┃┃g2d┃┃   ┃g1h
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g1a", "task0"}, {"g1b", "task1"}, {"g1g", "task2"}, {"g1h", "task3"}};
    auto solver = getSolverFor8GPU(GetParam(), initialAssignment);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    // Moving task2 and task3 to g1c and g1d, OR moving task0 and task1 to g1e
    // and g1f gives the optimal solution.
    int numAssigned = 0;
    for (auto& bundle :
         {"g1a", "g1b", "g1c", "g1d", "g1e", "g1f", "g1g", "g1h"}) {
      numAssigned += bundleToTaskCount[bundle];
    }

    // ensures all tasks are assigned
    EXPECT_EQ(4, numAssigned);

    // The MinimizeInvalidatedScopeItems goal does not move any of the
    // tasks and fails to reach the optimal solution.
    EXPECT_EQ(1, bundleToTaskCount["g1a"]);
    EXPECT_EQ(1, bundleToTaskCount["g1b"]);
    EXPECT_EQ(1, bundleToTaskCount["g1g"]);
    EXPECT_EQ(1, bundleToTaskCount["g1h"]);
  }
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1a   ┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃*T0┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2a┃┃   ┃g1b   ┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃*T2┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g4a┃┃g2b┃┃   ┃g1d   ┃   ┃┃g4a┃┃g2b┃┃*T3┃g1d
    // ┃   ┃┗━━━┛┗━━━┛┗━━━┛      ┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T1┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2c┃┃*T2┃g1f   ┃   ┃┃   ┃┃g2c┃┃   ┃g1f
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛   -> ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃g8a┃┃g4b┃┃g2d┃┃   ┃g1h   ┃g8a┃┃g4b┃┃g2d┃┃   ┃g1h
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g2a", "task0"}, {"g1e", "task1"}, {"g1f", "task2"}};
    auto solver = getSolverFor8GPU(GetParam(), initialAssignment);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    int numAssigned = 0;
    for (auto& bundle :
         {"g2a",
          "g2b",
          "g2c",
          "g2d",
          "g1a",
          "g1b",
          "g1c",
          "g1d",
          "g1e",
          "g1f",
          "g1g",
          "g1h"}) {
      numAssigned += bundleToTaskCount[bundle];
    }

    // ensures all tasks are assigned
    EXPECT_EQ(3, numAssigned);

    // Moving task0 to g2d gives the optimal solution.
    EXPECT_EQ(1, bundleToTaskCount["g2d"]);
    EXPECT_EQ(1, bundleToTaskCount["g1e"]);
    EXPECT_EQ(1, bundleToTaskCount["g1f"]);
  }
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1a   ┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃*T0┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2a┃┃   ┃g1b   ┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃*T2┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g4a┃┃g2b┃┃   ┃g1d   ┃   ┃┃g4a┃┃g2b┃┃*T3┃g1d
    // ┃   ┃┗━━━┛┗━━━┛┗━━━┛      ┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T1┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2c┃┃   ┃g1f   ┃   ┃┃   ┃┃g2c┃┃   ┃g1f
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛   -> ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T2┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃g8a┃┃g4b┃┃g2d┃┃   ┃g1h   ┃g8a┃┃g4b┃┃g2d┃┃   ┃g1h
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g2a", "task0"}, {"g1e", "task1"}, {"g1g", "task2"}};
    auto solver = getSolverFor8GPU(GetParam(), initialAssignment);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    int numAssigned = 0;
    for (auto& bundle :
         {"g2a",
          "g2b",
          "g2c",
          "g2d",
          "g1a",
          "g1b",
          "g1c",
          "g1d",
          "g1e",
          "g1f",
          "g1g",
          "g1h"}) {
      numAssigned += bundleToTaskCount[bundle];
    }

    // ensures all tasks are assigned
    EXPECT_EQ(3, numAssigned);

    const int sum1 = bundleToTaskCount["g2a"] + bundleToTaskCount["g1c"] +
        bundleToTaskCount["g1d"];
    const int sum2 = bundleToTaskCount["g2c"] + bundleToTaskCount["g1g"] +
        bundleToTaskCount["g1h"];
    const int sum3 = bundleToTaskCount["g2d"] + bundleToTaskCount["g1e"] +
        bundleToTaskCount["g1f"];
    EXPECT_TRUE(sum1 == 3 || sum2 == 3 || sum3 == 3);
  }
}

TEST_P(
    ExclusiveScopeItemsGpuBundleTest,
    ExclusiveScopeItemsConstraintWithAggressivePackingGoalFor8GPU) {
  const auto formula = ExclusiveScopeItemsFormula::AGGRESSIVE_PACKING;
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T0┃g1a   ┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2a┃┃*T1┃g1b   ┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃*T2┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g4a┃┃g2b┃┃   ┃g1d   ┃   ┃┃g4a┃┃g2b┃┃*T3┃g1d
    // ┃   ┃┗━━━┛┗━━━┛┗━━━┛      ┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2c┃┃   ┃g1f   ┃   ┃┃   ┃┃g2c┃┃   ┃g1f
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛   -> ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T2┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃g8a┃┃g4b┃┃g2d┃┃*T3┃g1h   ┃g8a┃┃g4b┃┃g2d┃┃   ┃g1h
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g1a", "task0"}, {"g1b", "task1"}, {"g1g", "task2"}, {"g1h", "task3"}};
    auto solver = getSolverFor8GPU(GetParam(), initialAssignment, formula);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    // Moving task2 and task3 to g1c and g1b, OR moving task0 and task1 to g1e
    // and g1f gives the optimal solution.
    int numAssigned = 0;
    for (auto& bundle :
         {"g1a", "g1b", "g1c", "g1d", "g1e", "g1f", "g1g", "g1h"}) {
      numAssigned += bundleToTaskCount[bundle];
    }

    // ensures all tasks are assigned
    EXPECT_EQ(4, numAssigned);

    const int sum1 = bundleToTaskCount["g1a"] + bundleToTaskCount["g1b"] +
        bundleToTaskCount["g1c"] + bundleToTaskCount["g1d"];
    const int sum2 = bundleToTaskCount["g1e"] + bundleToTaskCount["g1f"] +
        bundleToTaskCount["g1g"] + bundleToTaskCount["g1h"];
    EXPECT_TRUE(sum1 == 0 || sum1 == 4);
    EXPECT_TRUE(sum2 == 0 || sum2 == 4);
  }
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1a   ┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃*T0┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2a┃┃   ┃g1b   ┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃*T2┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g4a┃┃g2b┃┃   ┃g1d   ┃   ┃┃g4a┃┃g2b┃┃*T3┃g1d
    // ┃   ┃┗━━━┛┗━━━┛┗━━━┛      ┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T1┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2c┃┃*T2┃g1f   ┃   ┃┃   ┃┃g2c┃┃   ┃g1f
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛   -> ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃g8a┃┃g4b┃┃g2d┃┃   ┃g1h   ┃g8a┃┃g4b┃┃g2d┃┃   ┃g1h
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g2a", "task0"}, {"g1e", "task1"}, {"g1f", "task2"}};
    auto solver = getSolverFor8GPU(GetParam(), initialAssignment);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    int numAssigned = 0;
    for (auto& bundle :
         {"g2a",
          "g2b",
          "g2c",
          "g2d",
          "g1a",
          "g1b",
          "g1c",
          "g1d",
          "g1e",
          "g1f",
          "g1g",
          "g1h"}) {
      numAssigned += bundleToTaskCount[bundle];
    }

    // ensures all tasks are assigned
    EXPECT_EQ(3, numAssigned);

    // Moving task0 to g2d gives the optimal solution.
    EXPECT_EQ(1, bundleToTaskCount["g2d"]);
    EXPECT_EQ(1, bundleToTaskCount["g1e"]);
    EXPECT_EQ(1, bundleToTaskCount["g1f"]);
  }
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1a   ┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃*T0┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2a┃┃   ┃g1b   ┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃*T2┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g4a┃┃g2b┃┃   ┃g1d   ┃   ┃┃g4a┃┃g2b┃┃*T3┃g1d
    // ┃   ┃┗━━━┛┗━━━┛┗━━━┛      ┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T1┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g2c┃┃   ┃g1f   ┃   ┃┃   ┃┃g2c┃┃   ┃g1f
    // ┃   ┃┃   ┃┗━━━┛┗━━━┛   -> ┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T2┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃g8a┃┃g4b┃┃g2d┃┃   ┃g1h   ┃g8a┃┃g4b┃┃g2d┃┃   ┃g1h
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g2a", "task0"}, {"g1e", "task1"}, {"g1g", "task2"}};
    auto solver = getSolverFor8GPU(GetParam(), initialAssignment);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    int numAssigned = 0;
    for (auto& bundle :
         {"g2a",
          "g2b",
          "g2c",
          "g2d",
          "g1a",
          "g1b",
          "g1c",
          "g1d",
          "g1e",
          "g1f",
          "g1g",
          "g1h"}) {
      numAssigned += bundleToTaskCount[bundle];
    }

    // ensures all tasks are assigned
    EXPECT_EQ(3, numAssigned);

    const int sum1 = bundleToTaskCount["g2a"] + bundleToTaskCount["g1c"] +
        bundleToTaskCount["g1d"];
    const int sum2 = bundleToTaskCount["g2c"] + bundleToTaskCount["g1g"] +
        bundleToTaskCount["g1h"];
    const int sum3 = bundleToTaskCount["g2d"] + bundleToTaskCount["g1e"] +
        bundleToTaskCount["g1f"];
    EXPECT_TRUE(sum1 == 3 || sum2 == 3 || sum3 == 3);
  }
}

TEST_P(
    ExclusiveScopeItemsGpuBundleTest,
    ExclusiveScopeItemsConstraintWithMinimizeInvalidatedGoalFor12GPU) {
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃   ┃g1b   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d   ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f   ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f
    // ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛   -> ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h   ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i   ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1k   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1k
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃*T1┃g1l   ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g1a", "task0"}, {"g1l", "task1"}};
    auto solver = getSolverFor12GPU(GetParam(), initialAssignment);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    // Moving task1 to g1b or task0 to g1k gives the optimal solution.
    int numAssigned = 0;
    for (auto& bundle : {"g1a", "g1b", "g1k", "g1l"}) {
      numAssigned += bundleToTaskCount[bundle];
    }

    // ensures all tasks are assigned
    EXPECT_EQ(2, numAssigned);

    const int sum1 = bundleToTaskCount["g1a"] + bundleToTaskCount["g1b"];
    const int sum2 = bundleToTaskCount["g1k"] + bundleToTaskCount["g1l"];
    EXPECT_TRUE(sum1 == 0 || sum1 == 2);
    EXPECT_TRUE(sum2 == 0 || sum2 == 2);
  }
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃*T1┃g1b   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃*T2┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d   ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f   ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f
    // ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛   -> ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h   ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i   ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1k   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1k
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃*T2┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l   ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g1a", "task0"}, {"g1b", "task1"}, {"g2f", "task2"}};
    auto solver = getSolverFor12GPU(GetParam(), initialAssignment);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    // Moving task2 to g2b gives the optimal solution.
    EXPECT_EQ(1, bundleToTaskCount["g1a"]);
    EXPECT_EQ(1, bundleToTaskCount["g1b"]);
    EXPECT_EQ(1, bundleToTaskCount["g2b"]);
  }
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃*T1┃g1b   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃*T2┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d   ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃*T3┃g1d
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f   ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f
    // ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛   -> ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h   ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i   ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T2┃g1k   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1k
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃*T3┃g1l   ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g1a", "task0"}, {"g1b", "task1"}, {"g1k", "task2"}, {"g1l", "task3"}};
    auto solver = getSolverFor12GPU(GetParam(), initialAssignment);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    // The MinimizeInvalidatedScopeItems goal does not move any of the
    // tasks and fails to reach the optimal solution.
    EXPECT_EQ(1, bundleToTaskCount["g1a"]);
    EXPECT_EQ(1, bundleToTaskCount["g1b"]);
    EXPECT_EQ(1, bundleToTaskCount["g1k"]);
    EXPECT_EQ(1, bundleToTaskCount["g1l"]);
  }
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1a
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃   ┃g1b   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃   ┃g1b
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃*T1┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d   ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f   ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f
    // ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛   -> ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃*T1┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h   ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i   ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃*T0┃g1i
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T2┃┃   ┃┃   ┃g1k   ┃   ┃┃   ┃┃   ┃┃*T2┃┃   ┃┃   ┃g1k
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l   ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g1a", "task0"}, {"g2b", "task1"}, {"g3d", "task2"}};
    auto solver = getSolverFor12GPU(GetParam(), initialAssignment);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    // The MinimizeInvalidatedScopeItems goal does not move any of the
    // tasks and fails to reach the optimal solution.
    EXPECT_EQ(1, bundleToTaskCount["g1a"]);
    EXPECT_EQ(1, bundleToTaskCount["g2b"]);
    EXPECT_EQ(1, bundleToTaskCount["g3d"]);
  }
}

TEST_P(
    ExclusiveScopeItemsGpuBundleTest,
    ExclusiveScopeItemsConstraintWithAggressivePackingGoalFor12GPU) {
  const auto formula = ExclusiveScopeItemsFormula::AGGRESSIVE_PACKING;
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃   ┃g1b   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d   ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f   ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f
    // ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛   -> ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h   ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i   ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1k   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1k
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃*T1┃g1l   ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g1a", "task0"}, {"g1l", "task1"}};
    auto solver = getSolverFor12GPU(GetParam(), initialAssignment, formula);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    // Moving task1 to g1b or task0 to g1k gives the optimal solution.
    int numAssigned = 0;
    for (auto& bundle : {"g1a", "g1b", "g1k", "g1l"}) {
      numAssigned += bundleToTaskCount[bundle];
    }

    // ensures all tasks are assigned
    EXPECT_EQ(2, numAssigned);

    const int sum1 = bundleToTaskCount["g1a"] + bundleToTaskCount["g1b"];
    const int sum2 = bundleToTaskCount["g1k"] + bundleToTaskCount["g1l"];
    EXPECT_TRUE(sum1 == 0 || sum1 == 2);
    EXPECT_TRUE(sum2 == 0 || sum2 == 2);
  }
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃*T1┃g1b   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃*T2┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d   ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f   ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f
    // ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛   -> ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h   ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i   ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1k   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1k
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃*T2┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l   ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g1a", "task0"}, {"g1b", "task1"}, {"g2f", "task2"}};
    auto solver = getSolverFor12GPU(GetParam(), initialAssignment, formula);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    // Moving task2 to g2b gives the optimal solution.
    EXPECT_EQ(1, bundleToTaskCount["g1a"]);
    EXPECT_EQ(1, bundleToTaskCount["g1b"]);
    EXPECT_EQ(1, bundleToTaskCount["g2b"]);
  }
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃*T1┃g1b   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃*T1┃g1b
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃*T2┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d   ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃*T3┃g1d
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f   ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f
    // ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛   -> ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h   ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i   ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T2┃g1k   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1k
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃*T3┃g1l   ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g1a", "task0"}, {"g1b", "task1"}, {"g1k", "task2"}, {"g1l", "task3"}};
    auto solver = getSolverFor12GPU(GetParam(), initialAssignment, formula);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    // Moving task0 and task 1 to g1i and g1j or task2 and task 3 to g1c and g1d
    // gives the optimal solution.
    int numAssigned = 0;
    for (auto& bundle :
         {"g1a", "g1b", "g1c", "g1d", "g1i", "g1j", "g1k", "g1l"}) {
      numAssigned += bundleToTaskCount[bundle];
    }

    // ensures all tasks are assigned
    EXPECT_EQ(4, numAssigned);

    const int sum1 = bundleToTaskCount["g1a"] + bundleToTaskCount["g1b"] +
        bundleToTaskCount["g1c"] + bundleToTaskCount["g1d"];
    const int sum2 = bundleToTaskCount["g1i"] + bundleToTaskCount["g1j"] +
        bundleToTaskCount["g1k"] + bundleToTaskCount["g1l"];
    EXPECT_TRUE(sum1 == 0 || sum1 == 4);
    EXPECT_TRUE(sum2 == 0 || sum2 == 4);
  }
  {
    // ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓      ┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃*T0┃g1a   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1a
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃   ┃g1b   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2a┃┃   ┃g1b
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c   ┃   ┃┃   ┃┃   ┃┃g3a┃┃   ┃┃   ┃g1c
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃*T1┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d   ┃   ┃┃   ┃┃g4a┃┃   ┃┃g2b┃┃   ┃g1d
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1e
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f   ┃   ┃┃g6a┃┃   ┃┃g3b┃┃g2c┃┃   ┃g1f
    // ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛   -> ┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛┗━━━┛
    // ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓      ┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g   ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃g1g
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃*T1┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h   ┃   ┃┃   ┃┃g4b┃┃   ┃┃g2d┃┃   ┃g1h
    // ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃   ┃g1i   ┃   ┃┃   ┃┃   ┃┃g3c┃┃   ┃┃*T0┃g1i
    // ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┗━━━┛┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┏━━━┓┃   ┃┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j   ┃   ┃┃   ┃┃   ┃┃   ┃┃g2e┃┃   ┃g1j
    // ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓┏━━━┓
    // ┃   ┃┃   ┃┃   ┃┃*T2┃┃   ┃┃   ┃g1k   ┃   ┃┃   ┃┃   ┃┃*T2┃┃   ┃┃   ┃g1k
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┗━━━┛
    // ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓      ┃   ┃┃   ┃┃   ┃┃   ┃┃   ┃┏━━━┓
    // ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l   ┃12a┃┃g6b┃┃g4c┃┃g3d┃┃g2f┃┃   ┃g1l
    // ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛      ┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛┗━━━┛
    std::vector<std::pair<std::string, std::string>> initialAssignment = {
        {"g1a", "task0"}, {"g2b", "task1"}, {"g3d", "task2"}};
    auto solver = getSolverFor12GPU(GetParam(), initialAssignment, formula);
    solver->addSolver(makeDefaultLocalSearchSolver());

    // Get a solution from Rebalancer.
    auto solution = solver->solve();
    auto assignment = *solution.assignment();

    entities::Map<std::string, int> bundleToTaskCount;
    for (auto& [task, bundle] : assignment) {
      bundleToTaskCount[bundle] += 1;
    }

    // Moving task0 to g1i and task1 to g2d gives the optimal solution.
    EXPECT_EQ(1, bundleToTaskCount["g1i"]);
    EXPECT_EQ(1, bundleToTaskCount["g2d"]);
    EXPECT_EQ(1, bundleToTaskCount["g3d"]);
  }
}
} // namespace facebook::rebalancer::interface::tests
