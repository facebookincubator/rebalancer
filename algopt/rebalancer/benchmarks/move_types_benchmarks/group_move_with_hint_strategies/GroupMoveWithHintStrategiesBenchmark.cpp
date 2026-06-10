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

using namespace facebook::rebalancer;
using namespace facebook::rebalancer::interface;

void createBasicScenario(
    int numTables,
    int numShardTypes,
    int numNodes,
    int moveSetsGenerated,
    int numRanks,
    int numShards);

void createBasicScenario(
    int numTables,
    int numShardTypes,
    int numNodes,
    int moveSetsGenerated,
    int numRanks,
    int numShards) {
  folly::BenchmarkSuspender suspend;
  auto solver = initializeTestProblemSolver();

  solver->setObjectName("shards");
  solver->setContainerName("ranks");

  // create initial assignment
  facebook::rebalancer::entities::Map<std::string, std::vector<std::string>>
      initialAssignment;
  for (const auto i : folly::irange(numRanks)) {
    initialAssignment[fmt::format("rank{}", i)] = {};
  }

  // create dummy rank with all the shards
  for (const auto i : folly::irange(numShards)) {
    initialAssignment["dummyRank"].push_back(fmt::format("shard{}", i));
  }
  solver->setAssignment(initialAssignment);

  // create tables
  // each shard type will have 10 shards
  // each table will have numShardTypes shards
  facebook::rebalancer::entities::Map<std::string, std::vector<std::string>>
      shardToTable;
  for (const auto i : folly::irange(numTables)) {
    const int numShardsInTable = numShards / numTables;
    for (const auto j : folly::irange(numShardsInTable)) {
      shardToTable[fmt::format("table{}", i)].push_back(
          fmt::format("shard{}", i * numShardsInTable + j));
    }
  }
  solver->addPartition("tables", std::move(shardToTable));

  // create shard types
  // each shard type will have 10 shards
  facebook::rebalancer::entities::Map<std::string, std::vector<std::string>>
      shardToShardType;
  for (const auto i : folly::irange(numTables)) {
    const int numShardsInTable = numShards / numTables;
    for (const auto j : folly::irange(numShardsInTable)) {
      shardToShardType[fmt::format("shardType{}", j % numShardTypes)].push_back(
          fmt::format("shard{}", i * numShardsInTable + j));
    }
  }

  solver->addPartition("shardTypes", std::move(shardToShardType));

  // create nodes
  facebook::rebalancer::entities::Map<std::string, std::vector<std::string>>
      nodeToRanks;
  for (const auto i : folly::irange(numNodes)) {
    const int numRanksInNode = numRanks / numNodes;
    for (const auto j : folly::irange(numRanksInNode)) {
      nodeToRanks[fmt::format("node{}", i)].push_back(
          fmt::format("rank{}", i * numRanksInNode + j));
    }
  }
  nodeToRanks.emplace("dummyNode", std::vector<std::string>{"dummyRank"});
  solver->addScope("nodes", nodeToRanks);

  // create worlds
  facebook::rebalancer::entities::Map<std::string, std::vector<std::string>>
      worldToRanks;
  worldToRanks["fakeWorld"].emplace_back("dummyRank");
  for (const auto i : folly::irange(numRanks)) {
    worldToRanks["realWorld"].push_back(fmt::format("rank{}", i));
  }
  solver->addScope("worlds", worldToRanks);

  // create partition for shardtype in each table
  facebook::rebalancer::entities::Map<std::string, std::vector<std::string>>
      shardTypesInTable;
  for (const auto t : folly::irange(numTables)) {
    const int numShardsInTable = numShards / numTables;
    for (const auto j : folly::irange(numShardsInTable)) {
      auto shardName = fmt::format("shard{}", t * numShardsInTable + j);
      auto shardType = j % numShardTypes;
      shardTypesInTable[fmt::format("table{}_shardType{}", t, shardType)]
          .push_back(shardName);
    }
  }
  solver->addPartition("shardTypesInTable", std::move(shardTypesInTable));

  facebook::rebalancer::entities::Map<std::string, double> shardTypeInTableDim;
  for (const auto t : folly::irange(numTables)) {
    const int numShardsInTable = numShards / numTables;
    for (const auto j : folly::irange(numShardTypes)) {
      auto shardName = fmt::format("shard{}", t * numShardsInTable + j);
      shardTypeInTableDim[shardName] = 1;
    }
    for (int j = numShardTypes; j < numShardsInTable; j++) {
      auto shardName = fmt::format("shard{}", t * numShardsInTable + j);
      shardTypeInTableDim[shardName] = 0;
    }
  }
  solver->addObjectDimension(
      "shardTypeInTableDim", std::move(shardTypeInTableDim));

  // hbm and runtime dimension
  facebook::rebalancer::entities::Map<std::string, double> HBMdim;
  for (const auto i : folly::irange(numTables)) {
    const int numShardsInTable = numShards / numTables;
    for (const auto j : folly::irange(numShardsInTable)) {
      HBMdim[fmt::format("shard{}", i * numShardsInTable + j)] =
          j % numShardTypes + 1;
    }
  }

  solver->addObjectDimension("HBM", std::move(HBMdim));

  facebook::rebalancer::entities::Map<std::string, double> runtimeDim;
  for (const auto i : folly::irange(numTables)) {
    const int numShardsInTable = numShards / numTables;
    for (const auto j : folly::irange(numShardsInTable)) {
      runtimeDim[fmt::format("shard{}", i * numShardsInTable + j)] =
          (j + 2) % numShardTypes + 1;
    }
  }
  solver->addObjectDimension("runtimeDim", std::move(runtimeDim));

  // at_most_one_shard_type_constraint
  GroupCountSpec atMostOneShardGroupCountSpec;
  atMostOneShardGroupCountSpec.scope() = "worlds";
  atMostOneShardGroupCountSpec.partitionName() = "tables";
  atMostOneShardGroupCountSpec.dimension() = "shardTypeInTableDim";
  atMostOneShardGroupCountSpec.name() = "at-most-one-shard-type-per-table";
  atMostOneShardGroupCountSpec.filter()->itemsBlacklist() = {"fakeWorld"};
  Limit atMostOneShardGroupCountSpecLimit;
  atMostOneShardGroupCountSpecLimit.type() = LimitType::ABSOLUTE;
  atMostOneShardGroupCountSpecLimit.globalLimit() = numShards;
  atMostOneShardGroupCountSpecLimit.scopeItemLimits() = {
      {"realWorld", 1}, {"fakeWorld", numShards}};
  atMostOneShardGroupCountSpec.limit() =
      std::move(atMostOneShardGroupCountSpecLimit);
  solver->addConstraint(std::move(atMostOneShardGroupCountSpec));

  // at-least-one-shard-type-per-table
  GroupCountSpec atLeastOneShardGroupCountSpec;
  atLeastOneShardGroupCountSpec.scope() = "worlds";
  atLeastOneShardGroupCountSpec.partitionName() = "tables";
  atLeastOneShardGroupCountSpec.dimension() = "shardTypeInTableDim";
  atLeastOneShardGroupCountSpec.name() = "at-least-one-shard-type-per-table";
  atLeastOneShardGroupCountSpec.filter()->itemsBlacklist() = {"fakeWorld"};
  Limit atLeastOneShardGroupCountSpecLimit;
  atLeastOneShardGroupCountSpecLimit.type() = LimitType::ABSOLUTE;
  atLeastOneShardGroupCountSpecLimit.globalLimit() = 0;
  atLeastOneShardGroupCountSpecLimit.scopeItemLimits() = {
      {"realWorld", 1}, {"fakeWorld", 0}};
  atLeastOneShardGroupCountSpec.limit() =
      std::move(atLeastOneShardGroupCountSpecLimit);
  atLeastOneShardGroupCountSpec.bound() = GroupCountSpecBound::MIN;
  solver->addConstraint(std::move(atLeastOneShardGroupCountSpec));

  // colocate same shard type in the same
  ColocateGroupsSpec sameShardTypeInSameTableColocateGroupSpec;
  sameShardTypeInSameTableColocateGroupSpec.scope() = "worlds";
  sameShardTypeInSameTableColocateGroupSpec.partitionName() =
      "shardTypesInTable";
  sameShardTypeInSameTableColocateGroupSpec.name() = "assign-all-shards";
  solver->addConstraint(std::move(sameShardTypeInSameTableColocateGroupSpec));

  // one shard per rank for even shard types
  for (int i = 0; i < numShardTypes; i += 2) {
    std::unordered_map<std::string, double> oneShardPerRankDimension;
    for (const auto j : folly::irange(numTables)) {
      const int numShardsInTable = numShards / numTables;
      for (const auto k : folly::irange(numShardsInTable)) {
        if (k % numShardTypes == i) {
          oneShardPerRankDimension[fmt::format(
              "shard{}", j * numShardsInTable + k)] = 1;
        } else {
          oneShardPerRankDimension[fmt::format(
              "shard{}", j * numShardsInTable + k)] = 0;
        }
      }
    }
    solver->addObjectDimension(
        fmt::format("oneShardPerRank_{}", i), oneShardPerRankDimension);
    Limit rankLimit;
    rankLimit.type() = LimitType::ABSOLUTE;
    rankLimit.globalLimit() = numShards;
    std::map<std::string, double> rankLimitMap;
    for (const auto j : folly::irange(numRanks)) {
      rankLimitMap[fmt::format("rank{}", j)] = 1;
    }
    rankLimit.scopeItemLimits() = rankLimitMap;

    GroupCountSpec oneShardPerRankGroupCountSpec;
    oneShardPerRankGroupCountSpec.scope() = "ranks";
    oneShardPerRankGroupCountSpec.partitionName() = "tables";
    oneShardPerRankGroupCountSpec.dimension() =
        fmt::format("oneShardPerRank_{}", i);
    oneShardPerRankGroupCountSpec.limit() = rankLimit;
    oneShardPerRankGroupCountSpec.name() =
        fmt::format("dp-shards-at-most-one-per-rank_{}", i);
    solver->addConstraint(oneShardPerRankGroupCountSpec);
  }

  LocalSearchSolverSpec localSearchSpec;
  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveWithHintStrategiesSpec;
  groupMoveWithHintStrategiesSpec.primaryPartition() = "tables";
  groupMoveWithHintStrategiesSpec.secondaryPartition() = "shardTypes";

  std::map<std::string, MoveStrategy> hintMap;
  MoveStrategy option1;
  option1.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  option1.moveSetsGeneratedPerScopeItem() = moveSetsGenerated;
  MoveToScopeItemsSpec moveToScopeItemsSpec1;
  ScopeItemList defaultScopeItems1;
  defaultScopeItems1.scopeName() = "worlds";
  moveToScopeItemsSpec1.defaultScopeItems() = std::move(defaultScopeItems1);
  option1.moveToScopeItems() = std::move(moveToScopeItemsSpec1);

  MoveStrategy option2;
  option2.type() = MoveStrategyType::RANDOM_SAMPLING_WITHOUT_REPLACEMENT;
  option2.moveSetsGeneratedPerScopeItem() = moveSetsGenerated;
  MoveToScopeItemsSpec moveToScopeItemsSpec2;
  ScopeItemList defaultScopeItems2;
  defaultScopeItems2.scopeName() = "nodes";
  moveToScopeItemsSpec2.defaultScopeItems() = std::move(defaultScopeItems2);
  option2.moveToScopeItems() = std::move(moveToScopeItemsSpec2);

  // even shard types will get option 2 cause all even shards can only have 1
  // rank per shard
  for (const auto i : folly::irange(numShardTypes)) {
    if (i % 2 == 0) {
      hintMap.emplace(fmt::format("shardType{}", i), option2);
    } else {
      hintMap.emplace(fmt::format("shardType{}", i), option1);
    }
  }

  MoveStrategies groupToMoveStrategy;
  groupToMoveStrategy.groupToMoveStrategy() = std::move(hintMap);

  groupMoveWithHintStrategiesSpec.moveStrategies() =
      std::move(groupToMoveStrategy);

  localSearchSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(
          std::move(groupMoveWithHintStrategiesSpec)));

  localSearchSpec.exploreMovesFromContainersNotInObjective() = true;
  solver->addSolver(localSearchSpec);
  suspend.dismiss();
  auto solution = solver->solve();
}

BENCHMARK(BasicScenario) {
  /*
     int numTables = 3
     int numShardTypes = 4
     int numNodes = 4
     int moveSetsGenerated = 1
     int numRanks = 8
     int numShards = 24
  */
  createBasicScenario(3, 4, 4, 1, 8, 24);
}

BENCHMARK(InitialTorchRecScenario) {
  /*
     int numTables = 2500
     int numShardTypes = 10
     int numNodes = 125
     int moveSetsGenerated = 1
     int numRanks = 1000
     int numShards = 25e5
  */
  createBasicScenario(2500, 10, 125, 1, 1000, 25e5);
}

BENCHMARK(WorstCaseScenario) {
  /*
     int numTables = 5000
     int numShardTypes = 10
     int numNodes = 125
     int moveSetsGenerated = 1
     int numRanks = 1250
     int numShards = 1e7
  */
  createBasicScenario(5000, 10, 125, 1, 1250, 1e7);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
