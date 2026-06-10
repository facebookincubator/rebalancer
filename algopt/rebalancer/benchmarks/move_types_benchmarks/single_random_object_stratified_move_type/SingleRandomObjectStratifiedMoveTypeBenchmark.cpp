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
#include <folly/Random.h>

using namespace facebook::rebalancer::interface;

// Each host has 1000 memory, each task has random[0, 99] memory and is
// assigned to hosts except one host. The host0 is whitelisted by min and
// max capacity goals. Tasks are grouped into 10 groups by memory.
static void run(int numHosts, int numTasks, int numSamples) {
  std::unordered_map<std::string, std::vector<std::string>> assignment;
  folly::F14FastMap<std::string, double> hostMemory;
  std::unordered_map<std::string, std::vector<std::string>> sameMemoryTaskLists;
  folly::F14FastMap<std::string, double> taskMemory;

  auto solver = initializeTestProblemSolver();
  solver->setObjectName("task");
  solver->setContainerName("host");

  // create numContainers+1 hosts with 1000 memory
  for (const auto i : folly::irange(numHosts + 1)) {
    std::string hostI = fmt::format("host{}", i);
    assignment[hostI] = {};
    hostMemory[hostI] = 1000;
  }

  // create 10 groups
  for (const auto i : folly::irange(10)) {
    sameMemoryTaskLists[fmt::format("group{}", i)] = {};
  }

  // create numTasks tasks and group by memory
  for (const auto i : folly::irange(numTasks)) {
    std::string taskI = fmt::format("task{}", i);
    double memory = folly::Random::secureRandDouble(0.1, 10);
    assignment[fmt::format("host{}", i % numHosts)].push_back(taskI);
    taskMemory[taskI] = memory;
    sameMemoryTaskLists[fmt::format("group{}", (int)memory)].push_back(taskI);
  }
  solver->setAssignment(assignment);
  solver->addObjectDimension("memory", std::move(taskMemory));
  solver->addContainerDimension("memory", hostMemory);
  solver->addPartition("group", std::move(sameMemoryTaskLists));

  // setup min goal for 1 host
  CapacitySpec minCapacitySpec;
  minCapacitySpec.scope() = "host";
  minCapacitySpec.filter()->itemsWhitelist() = {
      fmt::format("host{}", numHosts)};
  minCapacitySpec.dimension() = "memory";
  Limit limit;
  limit.type() = LimitType::RELATIVE;
  limit.globalLimit() = 0.5;
  minCapacitySpec.limit() = std::move(limit);
  minCapacitySpec.definition() = CapacitySpecDefinition::AFTER;
  minCapacitySpec.bound() = CapacitySpecBound::MIN;
  solver->addGoal(std::move(minCapacitySpec));
  solver->addGoalBoundary();

  // setup max goal for 1 host
  CapacitySpec maxCapacitySpec;
  maxCapacitySpec.scope() = "host";
  maxCapacitySpec.dimension() = "memory";
  maxCapacitySpec.filter()->itemsWhitelist() = {
      fmt::format("host{}", numHosts)};
  Limit limit2;
  limit2.type() = LimitType::RELATIVE;
  limit2.globalLimit() = 0.5;
  maxCapacitySpec.limit() = std::move(limit2);
  maxCapacitySpec.definition() = CapacitySpecDefinition::AFTER;
  maxCapacitySpec.bound() = CapacitySpecBound::MAX;
  solver->addGoal(std::move(maxCapacitySpec));

  // setup move type
  SingleRandomObjectStratifiedMoveTypeSpec
      singleRandomObjectStratifiedMoveTypeSpec;
  GroupList groupList;
  groupList.partitionName() = "group";
  ObjectsFromGroupsSpec objectsFromGroupsSpec;
  objectsFromGroupsSpec.groupList() = std::move(groupList);
  ObjectsToExploreOptions objectsToExploreOptions;
  objectsToExploreOptions.set_objectsFromGroupsSpec(objectsFromGroupsSpec);
  singleRandomObjectStratifiedMoveTypeSpec.objectsToExploreOptions() =
      std::move(objectsToExploreOptions);
  SampleSize sampleSize;
  sampleSize.defaultSampleSize() = numSamples;
  singleRandomObjectStratifiedMoveTypeSpec.stratifiedSampleSize() =
      std::move(sampleSize);
  MoveTypeSpec moveTypeSpec;
  moveTypeSpec.set_singleRandomObjectStratifiedMoveTypeSpec(
      singleRandomObjectStratifiedMoveTypeSpec);

  // setup local search
  LocalSearchSolverSpec localSearchSpec;
  localSearchSpec.stopAfterMoves() = numTasks * 2;
  localSearchSpec.solveTime() = 3600;
  localSearchSpec.moveTypeList()->push_back(std::move(moveTypeSpec));
  std::vector<LocalSearchStageSpec> stages;
  {
    LocalSearchStageSpec stage;
    stage.name() = "stage1";
    stage.begin() = 0;
    stage.end() = 1;
    stage.solverSpec() = localSearchSpec;
    stages.push_back(stage);
  }
  {
    LocalSearchStageSpec stage;
    stage.name() = "stage2";
    stage.begin() = 1;
    stage.end() = 2;
    stage.solverSpec() = std::move(localSearchSpec);
    stages.push_back(stage);
  }
  LocalSearchStageSolverSpec stageSolverSpec;
  stageSolverSpec.stageSpecs() = std::move(stages);
  stageSolverSpec.exploreMovesFromContainersNotInObjective() = false;
  solver->addSolver(stageSolverSpec);

  solver->solve();
}

// 25K host, 3M task, 300 samples.
BENCHMARK(single_random_object_stratified_move_type_benchmark) {
  run(25000, 3000000, 300);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
