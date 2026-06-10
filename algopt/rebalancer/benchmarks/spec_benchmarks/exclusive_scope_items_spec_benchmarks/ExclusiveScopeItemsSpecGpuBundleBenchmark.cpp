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

#include <map>

namespace entities = facebook::rebalancer::entities;
using namespace facebook::rebalancer::interface;

const std::vector<std::string> g8Bundles = {"g8a"};
const std::vector<std::string> g4Bundles = {"g4a", "g4b"};
const std::vector<std::string> g2Bundles = {"g2a", "g2b", "g2c", "g2d"};
const std::vector<std::string> g1Bundles =
    {"g1a", "g1b", "g1c", "g1d", "g1e", "g1f", "g1g", "g1h"};
const std::vector<std::vector<std::string>> availableBundles = {
    g8Bundles,
    g4Bundles,
    g2Bundles,
    g1Bundles};

std::vector<ScopeItemConflictInfo> makeConflictInfoList(
    const std::vector<
        std::pair<std::string, std::vector<std::pair<std::string, double>>>>&
        scopeItems);

std::string machineBundleSize(int machine);
std::string bundle(int machine, std::string bundle);
std::string task(int machine, int size, int taskNum);

std::unique_ptr<ProblemSolver> getSolver(
    const std::map<std::string, std::vector<std::string>>& assignment,
    const std::map<std::string, std::map<std::string, double>>& avoidItems,
    const std::vector<ScopeItemConflictInfo>& conflictInfoList,
    const std::map<std::string, double>& scopeItemWeights,
    const ExclusiveScopeItemsFormula formula);

bool isPacked(entities::Map<std::string, int>& bundleToTaskCount, int m);

std::unique_ptr<ProblemSolver> getSolverForMachines(
    const int machinesCount,
    const int size2TaskCountPerMachine,
    const int size1TaskCountPerMachine,
    const ExclusiveScopeItemsFormula formula);

std::vector<ScopeItemConflictInfo> makeConflictInfoList(
    const std::vector<
        std::pair<std::string, std::vector<std::pair<std::string, double>>>>&
        scopeItems) {
  std::vector<ScopeItemConflictInfo> result;
  for (const auto& [scopeItem, conflictingScopeItems] : scopeItems) {
    std::vector<ConflictingScopeItemInfo> conflictingScopeItemInfoList;
    for (const auto& [conflictingScopeItem, overlap] : conflictingScopeItems) {
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

std::string machineBundleSize(int machine, int size) {
  return fmt::format("zion{}gpu{}bundles", machine, size);
}

std::string bundle(int machine, std::string bundle) {
  return fmt::format("zion{}{}", machine, bundle);
}

std::string task(int machine, int size, int taskNum) {
  return fmt::format("zion{}-gpu{}task{}", machine, size, taskNum);
}

std::unique_ptr<ProblemSolver> getSolver(
    const std::map<std::string, std::vector<std::string>>& assignment,
    const std::map<std::string, std::map<std::string, double>>& avoidItems,
    const std::vector<ScopeItemConflictInfo>& conflictInfoList,
    const std::map<std::string, double>& scopeItemWeights,
    const ExclusiveScopeItemsFormula formula) {
  auto solver = initializeTestProblemSolver();
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

bool isPacked(entities::Map<std::string, int>& bundleToTaskCount, int m) {
  const int sum1 = bundleToTaskCount[bundle(m, "g2a")] +
      bundleToTaskCount[bundle(m, "g1b")] + bundleToTaskCount[bundle(m, "g1d")];
  const int sum2 = bundleToTaskCount[bundle(m, "g2b")] +
      bundleToTaskCount[bundle(m, "g1a")] + bundleToTaskCount[bundle(m, "g1c")];
  const int sum3 = bundleToTaskCount[bundle(m, "g2c")] +
      bundleToTaskCount[bundle(m, "g1g")] + bundleToTaskCount[bundle(m, "g1h")];
  const int sum4 = bundleToTaskCount[bundle(m, "g2d")] +
      bundleToTaskCount[bundle(m, "g1e")] + bundleToTaskCount[bundle(m, "g1f")];
  const int sum5 = bundleToTaskCount[bundle(m, "g1a")] +
      bundleToTaskCount[bundle(m, "g1b")] +
      bundleToTaskCount[bundle(m, "g1c")] + bundleToTaskCount[bundle(m, "g1d")];
  const int sum6 = bundleToTaskCount[bundle(m, "g1e")] +
      bundleToTaskCount[bundle(m, "g1f")] +
      bundleToTaskCount[bundle(m, "g1g")] + bundleToTaskCount[bundle(m, "g1h")];
  return sum1 == 3 || sum2 == 3 || sum3 == 3 || sum4 == 3 || sum5 == 4 ||
      sum6 == 4;
}

std::unique_ptr<ProblemSolver> getSolverForMachines(
    const int machinesCount,
    const int size2TaskCountPerMachine,
    const int size1TaskCountPerMachine,
    const ExclusiveScopeItemsFormula formula) {
  EXPECT_LE(2 * size2TaskCountPerMachine + size1TaskCountPerMachine, 8);
  EXPECT_LE(size2TaskCountPerMachine, 4);
  EXPECT_LE(size1TaskCountPerMachine, 8);

  std::map<std::string, std::vector<std::string>> assignment;
  std::map<std::string, std::map<std::string, double>> avoidItems;
  std::vector<
      std::pair<std::string, std::vector<std::pair<std::string, double>>>>
      conflictList;
  std::map<std::string, double> scopeItemWeights;
  std::mt19937 rng(0);
  for (const auto m : folly::irange(machinesCount)) {
    for (const auto& c : availableBundles) {
      for (const auto& b : c) {
        assignment[bundle(m, b)] = {};
      }
    }

    int taskCount = 0;

    // Assign size2 tasks
    std::vector<std::string> size2Bundles;
    size2Bundles.reserve(size2TaskCountPerMachine);
    std::sample(
        g2Bundles.begin(),
        g2Bundles.end(),
        std::back_inserter(size2Bundles),
        size2TaskCountPerMachine,
        rng);
    for (const auto& b : size2Bundles) {
      auto t = task(m, 2, taskCount++);
      assignment[bundle(m, b)].push_back(t);
      for (const auto& g2bundle : g2Bundles) {
        avoidItems[bundle(m, g2bundle)][t] = 0.0;
      }
    }

    taskCount = 0;

    // Assign size1 tasks
    std::vector<std::string> size1Bundles;
    std::map<std::string, std::vector<std::string>> g2BundleConflicts = {
        {"g2a", {"g1a", "g1c"}},
        {"g2b", {"g1b", "g1d"}},
        {"g2c", {"g1e", "g1f"}},
        {"g2d", {"g1g", "g1h"}}};
    std::vector<std::string> filteredG1Bundles;
    for (const auto& g2Bundle : g2Bundles) {
      if (count(size2Bundles.begin(), size2Bundles.end(), g2Bundle) == 0) {
        for (const auto& g1bundle : g2BundleConflicts[g2Bundle]) {
          filteredG1Bundles.push_back(g1bundle);
        }
      }
    }
    size1Bundles.reserve(size1TaskCountPerMachine);
    std::sample(
        filteredG1Bundles.begin(),
        filteredG1Bundles.end(),
        std::back_inserter(size1Bundles),
        size1TaskCountPerMachine,
        rng);
    for (const auto& b : size1Bundles) {
      auto t = task(m, 1, taskCount++);
      assignment[bundle(m, b)].push_back(t);
      for (const auto& g1Bundle : g1Bundles) {
        avoidItems[bundle(m, g1Bundle)][t] = 0.0;
      }
    }

    // Add conflicts
    conflictList.emplace_back(
        bundle(m, "g8a"),
        std::vector<std::pair<std::string, double>>{
            {bundle(m, "g4a"), 4},
            {bundle(m, "g4b"), 4},
            {bundle(m, "g2a"), 2},
            {bundle(m, "g2b"), 2},
            {bundle(m, "g2c"), 2},
            {bundle(m, "g2d"), 2},
            {bundle(m, "g1a"), 1},
            {bundle(m, "g1b"), 1},
            {bundle(m, "g1c"), 1},
            {bundle(m, "g1d"), 1},
            {bundle(m, "g1e"), 1},
            {bundle(m, "g1f"), 1},
            {bundle(m, "g1g"), 1},
            {bundle(m, "g1h"), 1}});
    conflictList.emplace_back(
        bundle(m, "g4a"),
        std::vector<std::pair<std::string, double>>{
            {bundle(m, "g2a"), 2},
            {bundle(m, "g2b"), 2},
            {bundle(m, "g1a"), 1},
            {bundle(m, "g1b"), 1},
            {bundle(m, "g1c"), 1},
            {bundle(m, "g1d"), 1}});
    conflictList.emplace_back(
        bundle(m, "g4b"),
        std::vector<std::pair<std::string, double>>{
            {bundle(m, "g2c"), 2},
            {bundle(m, "g2d"), 2},
            {bundle(m, "g1e"), 1},
            {bundle(m, "g1f"), 1},
            {bundle(m, "g1g"), 1},
            {bundle(m, "g1h"), 1}});
    conflictList.emplace_back(
        bundle(m, "g2a"),
        std::vector<std::pair<std::string, double>>{
            {bundle(m, "g1a"), 1}, {bundle(m, "g1c"), 1}});
    conflictList.emplace_back(
        bundle(m, "g2b"),
        std::vector<std::pair<std::string, double>>{
            {bundle(m, "g1b"), 1}, {bundle(m, "g1d"), 1}});
    conflictList.emplace_back(
        bundle(m, "g2c"),
        std::vector<std::pair<std::string, double>>{
            {bundle(m, "g1e"), 1}, {bundle(m, "g1f"), 1}});
    conflictList.emplace_back(
        bundle(m, "g2d"),
        std::vector<std::pair<std::string, double>>{
            {bundle(m, "g1g"), 1}, {bundle(m, "g1h"), 1}});

    // Add weights
    scopeItemWeights[bundle(m, "g8a")] = 8;
    scopeItemWeights[bundle(m, "g4a")] = 4;
    scopeItemWeights[bundle(m, "g4b")] = 4;
    scopeItemWeights[bundle(m, "g2a")] = 2;
    scopeItemWeights[bundle(m, "g2b")] = 2;
    scopeItemWeights[bundle(m, "g2c")] = 2;
    scopeItemWeights[bundle(m, "g2d")] = 2;
  }

  const auto conflictInfoList = makeConflictInfoList(conflictList);
  return getSolver(
      assignment, avoidItems, conflictInfoList, scopeItemWeights, formula);
}

static void run(
    int machinesCount,
    int size2TaskCountPerMachine,
    int size1TaskCountPerMachine,
    ExclusiveScopeItemsFormula formula) {
  auto solver = getSolverForMachines(
      machinesCount,
      size2TaskCountPerMachine,
      size1TaskCountPerMachine,
      formula);

  // Set up DestinationsToExplore for SINGLE_FAST
  std::map<std::string, std::vector<std::string>> scopeItemToContainers;
  for (const auto i : folly::irange(machinesCount)) {
    for (auto& g1Bundle : g1Bundles) {
      scopeItemToContainers[machineBundleSize(i, 1)].push_back(
          bundle(i, g1Bundle));
    }
    for (auto& g2Bundle : g2Bundles) {
      scopeItemToContainers[machineBundleSize(i, 2)].push_back(
          bundle(i, g2Bundle));
    }
    for (auto& g4Bundle : g4Bundles) {
      scopeItemToContainers[machineBundleSize(i, 4)].push_back(
          bundle(i, g4Bundle));
    }
    for (auto& g8Bundle : g8Bundles) {
      scopeItemToContainers[machineBundleSize(i, 8)].push_back(
          bundle(i, g8Bundle));
    }
  }
  solver->addScope("machineBundle", scopeItemToContainers);

  MoveToCurrentScopeItemSpec moveToCurrentScopeItemsSpec;
  moveToCurrentScopeItemsSpec.scopeNameForExploringMovesToCurrentScopeItem() =
      "machineBundle";

  DestinationsToExploreOptions destinationsToExplore;
  destinationsToExplore.set_moveToCurrentScopeItem() =
      moveToCurrentScopeItemsSpec;

  SingleFastMoveTypeSpec moveTypeSpec;
  moveTypeSpec.destinationsToExplore() = destinationsToExplore;

  LocalSearchSolverSpec localSearchSpec;
  localSearchSpec.stopAfterMoves() = std::numeric_limits<int>::max();
  localSearchSpec.solveTime() = std::numeric_limits<int>::max();
  localSearchSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(moveTypeSpec));
  localSearchSpec.recomputeContainerOrderingAfterEveryMove() = false;
  solver->addSolver(localSearchSpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  int packedCount = 0;
  entities::Map<std::string, int> bundleToTaskCount;
  for (auto& [task, bundle] : assignment) {
    bundleToTaskCount[bundle] += 1;
    EXPECT_EQ(bundleToTaskCount[bundle], 1);
  }
  for (const auto i : folly::irange(machinesCount)) {
    if (isPacked(bundleToTaskCount, i)) {
      packedCount++;
    }
  }

  XLOG(INFO) << "Packed percentage: " << packedCount / (double)machinesCount;
}

BENCHMARK(GpuBundles1Gpu2TaskAnd2Gpu1TasksWithMinimizeInvalidatedBenchmark) {
  run(1000, // machinesCount
      1, // size2TaskCountPerMachine
      2, // size1TaskCountPerMachine
      ExclusiveScopeItemsFormula::MINIMIZE_INVALIDATED_SCOPE_ITEMS_COUNT);
}

BENCHMARK(GpuBundles1Gpu2TaskAnd2Gpu1TasksWithAggressivePackingBenchmark) {
  run(1000, // machinesCount
      1, // size2TaskCountPerMachine
      2, // size1TaskCountPerMachine
      ExclusiveScopeItemsFormula::AGGRESSIVE_PACKING);
}

BENCHMARK(GpuBundles4Gpu1TasksWithMinimizeInvalidatedBenchmark) {
  run(1000, // machinesCount
      0, // size2TaskCountPerMachine
      4, // size1TaskCountPerMachine
      ExclusiveScopeItemsFormula::MINIMIZE_INVALIDATED_SCOPE_ITEMS_COUNT);
}

BENCHMARK(GpuBundles4Gpu1TasksWithAggressivePackingBenchmark) {
  run(1000, // machinesCount
      0, // size2TaskCountPerMachine
      4, // size1TaskCountPerMachine
      ExclusiveScopeItemsFormula::AGGRESSIVE_PACKING);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
