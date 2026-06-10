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

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/tests/utils.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer::interface;

static void run(int numContainers, int numTasks) {
  auto solver = initializeTestProblemSolver();

  // set up the problem
  solver->setObjectName("task");
  solver->setContainerName("host");

  std::unordered_map<std::string, std::vector<std::string>> assignment;
  std::vector<std::string> allServers;
  // create numContainers hosts
  for (const auto i : folly::irange(numContainers)) {
    auto host = fmt::format("host{}", i);
    assignment[host] = {};
    allServers.push_back(host);
  }

  // create fake container to store all tasks
  assignment["fake_container"] = {};
  allServers.emplace_back("fake_container");

  for (const auto i : folly::irange(numTasks)) {
    assignment["fake_container"].push_back(fmt::format("task{}", i));
  }

  solver->setAssignment(assignment);
  solver->addScope(
      "allservers",
      std::map<std::string, std::vector<std::string>>{{"all", allServers}});

  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "to_free";
  toFreeSpec.containers() = {"fake_container"};

  solver->addConstraint(std::move(toFreeSpec));

  LocalSearchSolverSpec solverSpec;
  solverSpec.stopAfterMoves() = numTasks * 2;
  solverSpec.solveTime() = 3600;
  solverSpec.enableConstrainedBoundsOptimization() = true;
  solverSpec.constrainedBoundsCheckMs() = 2000;
  solverSpec.minHotObjects() = 1;
  solverSpec.recomputeContainerOrderingAfterEveryMove() = false;
  solverSpec.exploreMovesFromContainersNotInObjective() = false;

  SingleRandomStratifiedMoveTypeSpec singleRandomStratifiedSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 10;

  ScopeItemList scopeItems;
  scopeItems.scopeName() = "allservers";

  MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.defaultScopeItems() = std::move(scopeItems);

  DestinationsToExploreOptions destinationsToExploreOptions;
  destinationsToExploreOptions.moveToScopeItems() =
      std::move(moveToScopeItemsSpec);

  singleRandomStratifiedSpec.destinationsToExplore() =
      std::move(destinationsToExploreOptions);
  singleRandomStratifiedSpec.stratifiedSampleSize() = std::move(sampleSize);

  solverSpec.singleRandomStratifiedMoveTypeSpec() = singleRandomStratifiedSpec;
  solverSpec.moveTypeList() = {
      ProblemSolver::makeMoveTypeSpec("SINGLE_RANDOM_STRATIFIED")};

  solver->addSolver(solverSpec);
  solver->solve();
}

static void run_with_destinations_to_explore_in_universe(
    int numContainers,
    int numTasks) {
  auto solver = initializeTestProblemSolver();

  // set up the problem
  solver->setObjectName("task");
  solver->setContainerName("host");

  std::unordered_map<std::string, std::vector<std::string>> assignment;
  std::vector<std::string> allServers;
  // create numContainers hosts
  for (const auto i : folly::irange(numContainers)) {
    auto host = fmt::format("host{}", i);
    assignment[host] = {};
    allServers.push_back(host);
  }

  // create fake container to store all tasks
  assignment["fake_container"] = {};
  allServers.emplace_back("fake_container");

  for (const auto i : folly::irange(numTasks)) {
    assignment["fake_container"].push_back(fmt::format("task{}", i));
  }

  solver->setAssignment(assignment);
  solver->addScope(
      "allservers",
      std::map<std::string, std::vector<std::string>>{{"all", allServers}});

  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "to_free";
  toFreeSpec.containers() = {"fake_container"};

  solver->addConstraint(std::move(toFreeSpec));

  LocalSearchSolverSpec solverSpec;
  solverSpec.stopAfterMoves() = numTasks * 2;
  solverSpec.solveTime() = 3600;
  solverSpec.enableConstrainedBoundsOptimization() = true;
  solverSpec.constrainedBoundsCheckMs() = 2000;
  solverSpec.minHotObjects() = 1;
  solverSpec.recomputeContainerOrderingAfterEveryMove() = false;
  solverSpec.exploreMovesFromContainersNotInObjective() = false;

  SingleRandomStratifiedMoveTypeSpec singleRandomStratifiedSpec;
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = 10;

  ScopeItemList scopeItems;
  scopeItems.scopeName() = "allservers";

  MoveToScopeItemsSpec moveToScopeItemsSpec;
  moveToScopeItemsSpec.defaultScopeItems() = std::move(scopeItems);

  DestinationsToExploreOptions destinationsToExploreOptions;
  destinationsToExploreOptions.moveToScopeItems() =
      std::move(moveToScopeItemsSpec);

  solver->addDestinationsToExploreOptions(
      "myHint", std::move(destinationsToExploreOptions));

  DestinationsToExploreOptions destinationName;
  destinationName.set_destinationToExploreName("myHint");

  singleRandomStratifiedSpec.stratifiedSampleSize() = std::move(sampleSize);
  singleRandomStratifiedSpec.destinationsToExplore() = destinationName;

  solverSpec.singleRandomStratifiedMoveTypeSpec() = singleRandomStratifiedSpec;
  solverSpec.moveTypeList() = {
      ProblemSolver::makeMoveTypeSpec("SINGLE_RANDOM_STRATIFIED")};

  solver->addSolver(solverSpec);
  solver->solve();
}

BENCHMARK(sampling_benchmark) {
  run(1000000, 10000);
}

BENCHMARK(sampling_with_destinations_to_explore_in_universe) {
  run_with_destinations_to_explore_in_universe(1000000, 10000);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
