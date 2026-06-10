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

#include <folly/container/irange.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::interface::tests {

class ColocateGroupsWithHintStrategiesTest
    : public ::testing::TestWithParam<int> {
 protected:
  static entities::Map<std::string, std::vector<std::string>>
  generateInitialAssignment(
      int numObjects,
      int numContainers,
      int numShardTypes,
      bool swap) {
    entities::Map<std::string, std::vector<std::string>> initialAssignment;
    for (const auto i : folly::irange(numContainers)) {
      initialAssignment.emplace(
          fmt::format("rank{}", i), std::vector<std::string>{});
    }
    initialAssignment.emplace(DUMMY_RANK, std::vector<std::string>{});
    if (swap) {
      int counter = 0;
      for (const auto i : folly::irange(numObjects)) {
        if (i % numShardTypes == 0 && counter < 2) {
          initialAssignment["rank0"].push_back(fmt::format("shard{}", i));
          counter++;
        } else {
          initialAssignment[DUMMY_RANK].push_back(fmt::format("shard{}", i));
        }
      }
    } else {
      for (const auto i : folly::irange(numObjects)) {
        initialAssignment[DUMMY_RANK].emplace_back(fmt::format("shard{}", i));
      }
    }
    return initialAssignment;
  }

  void populateShardToTable(int numTables, int numShardsPerTable) {
    for (const auto i : folly::irange(numTables)) {
      for (const auto j : folly::irange(numShardsPerTable)) {
        tableNameToShards[fmt::format("table{}", i)].emplace_back(
            fmt::format("shard{}", i * numShardsPerTable + j));
      }
    }
  }
  void populateShardTypeToShards(
      int numTables,
      int numShardsPerTable,
      int numShardTypes) {
    for (const auto t : folly::irange(numTables)) {
      for (const auto s : folly::irange(numShardsPerTable)) {
        const auto shardType = fmt::format("shardType{}", s % numShardTypes);
        const auto shard = fmt::format("shard{}", t * numShardsPerTable + s);
        shardTypeToShards[shardType].emplace_back(shard);
        shardToShardType[shard] = shardType;
      }
    }
  }

  static entities::Map<std::string, std::vector<std::string>>
  generateNodeToRanks(int numNodes, int numRanksPerNode) {
    entities::Map<std::string, std::vector<std::string>> nodeToRanks;
    for (const auto i : folly::irange(numNodes)) {
      for (const auto j : folly::irange(numRanksPerNode)) {
        nodeToRanks[fmt::format("node{}", i)].emplace_back(
            fmt::format("rank{}", i * numRanksPerNode + j));
      }
    }
    nodeToRanks.emplace("dummyNode", std::vector<std::string>{DUMMY_RANK});
    return nodeToRanks;
  }

  static entities::Map<std::string, std::vector<std::string>>
  generateWorldToRanks(int numRanks) {
    entities::Map<std::string, std::vector<std::string>> worldToRanks;
    worldToRanks["fakeWorld"].emplace_back(DUMMY_RANK);
    for (const auto i : folly::irange(numRanks)) {
      worldToRanks["realWorld"].emplace_back(fmt::format("rank{}", i));
    }
    return worldToRanks;
  }

  static entities::Map<std::string, std::vector<std::string>>
  generateShardTypeInTable(
      int numTables,
      int numShardsPerTable,
      int numShardTypes) {
    entities::Map<std::string, std::vector<std::string>> shardTypeInTable;
    for (const auto t : folly::irange(numTables)) {
      for (const auto s : folly::irange(numShardsPerTable)) {
        auto shardName = fmt::format("shard{}", t * numShardsPerTable + s);
        auto shardType = s % numShardTypes;
        shardTypeInTable[fmt::format("table{}_shardType{}", t, shardType)]
            .emplace_back(shardName);
      }
    }
    return shardTypeInTable;
  }

  static entities::Map<std::string, double> generateShardTypeInTableDim(
      int numTables,
      int numShardsPerTable,
      int numShardTypes) {
    entities::Map<std::string, double> shardTypeInTableDim;
    for (const auto t : folly::irange(numTables)) {
      for (const auto s : folly::irange(numShardsPerTable)) {
        auto shardName = fmt::format("shard{}", t * numShardsPerTable + s);
        auto leader = s / numShardTypes;
        if (leader == 0) {
          shardTypeInTableDim[shardName] = 1;
        } else {
          shardTypeInTableDim[shardName] = 0;
        }
      }
    }
    return shardTypeInTableDim;
  }

  static entities::Map<std::string, double> generateHBMdim(
      int numTables,
      int numShardsPerTable,
      int numShardTypes,
      bool swap) {
    facebook::rebalancer::entities::Map<std::string, double> HBMdim;
    if (swap) {
      int counter = 0;
      for (const auto i : folly::irange(numTables * numShardsPerTable)) {
        if (i % numShardTypes == 0 && counter < 2) {
          HBMdim[fmt::format("shard{}", i)] = 100;
          counter++;
        } else {
          HBMdim[fmt::format("shard{}", i)] = 1;
        }
      }
    } else {
      for (const auto t : folly::irange(numTables)) {
        for (const auto s : folly::irange(numShardsPerTable)) {
          auto shardName = fmt::format("shard{}", t * numShardsPerTable + s);
          auto shardType = s % numShardTypes + 1;
          HBMdim[shardName] = shardType;
        }
      }
    }
    return HBMdim;
  }

  static entities::Map<std::string, double> generateRuntimeDim(
      int numTables,
      int numShardsPerTable,
      int numShardTypes,
      int numShardsInShardType) {
    entities::Map<std::string, double> runtimeDim;
    for (const auto t : folly::irange(numTables)) {
      for (const auto s : folly::irange(numShardsPerTable)) {
        auto shardName = fmt::format("shard{}", t * numShardsPerTable + s);
        auto shardType = ((s + numShardsInShardType) % numShardTypes) + 1;
        runtimeDim[shardName] = shardType;
      }
    }
    return runtimeDim;
  }

  struct Config {
    int numShards = 0;
    int numTables = 0;
    int numShardsPerTable = 0;
    int numShardTypes = 0;
    int numNodes = 0;
    int numRanksPerNode = 0;
    int numRanks = 0;
    bool swap = false;
  };
  std::unique_ptr<ProblemSolver> createBasicScenario(const Config& config) {
    auto solver =
        initializeTestProblemSolver({.executorThreadCount = GetParam()});

    solver->setObjectName("shards");
    solver->setContainerName("ranks");

    // 8 shards in each table
    // 3 tables
    // 4 shard types
    // 2 shards per shard type in each table

    // shards in table 1: 1-8
    // shards in table 2: 9-16
    // shards in table 3: 17-24

    // 4 nodes, each node has 2 ranks
    // ranks in node 1: 0, 1
    // ranks in node 2: 2, 3
    // ranks in node 3: 4, 5
    // ranks in node 4: 6, 7

    const auto initialAssignment = generateInitialAssignment(
        config.numShards, config.numRanks, config.numShardTypes, config.swap);

    solver->setAssignment(initialAssignment);

    populateShardToTable(config.numTables, config.numShardsPerTable);
    solver->addPartition("tables", tableNameToShards);

    populateShardTypeToShards(
        config.numTables, config.numShardsPerTable, config.numShardTypes);
    solver->addPartition("shardTypes", shardToShardType);

    const auto nodeToRanks =
        generateNodeToRanks(config.numNodes, config.numRanksPerNode);
    solver->addScope("nodes", nodeToRanks);

    const auto worldToRanks = generateWorldToRanks(config.numRanks);
    solver->addScope("worlds", worldToRanks);

    // create partition for shardtype in each table
    const auto shardTypeInTable = generateShardTypeInTable(
        config.numTables, config.numShardsPerTable, config.numShardTypes);
    solver->addPartition("shardTypesInTable", shardTypeInTable);

    const auto shardTypeInTableDim = generateShardTypeInTableDim(
        config.numTables, config.numShardsPerTable, config.numShardTypes);
    solver->addObjectDimension("shardTypeInTableDim", shardTypeInTableDim);

    // each shard type in the same table had the same dimension
    const auto HBMdim = generateHBMdim(
        config.numTables,
        config.numShardsPerTable,
        config.numShardTypes,
        config.swap);
    solver->addObjectDimension("HBM", HBMdim);

    if (config.swap) {
      solver->addContainerDimension(
          "HBM",
          facebook::rebalancer::entities::Map<std::string, double>{
              {"rank0", 100}, {"rank1", 100}});
      // add capacity spec for hbm
      CapacitySpec capacitySpec;
      capacitySpec.scope() = "ranks";
      capacitySpec.dimension() = "HBM";
      capacitySpec.bound() = CapacitySpecBound::MAX;
      capacitySpec.filter()->itemsWhitelist() = {"rank0", "rank1"};
      capacitySpec.limit()->globalLimit() = 50;
      capacitySpec.limit()->type() = LimitType::ABSOLUTE;
      solver->addConstraint(
          capacitySpec, ConstraintPolicy::SOFT, std::nullopt, std::nullopt, 0);
    }

    // each shard has a runtime dimension, dimension value is between 1 to 10
    if (config.numShardsPerTable % config.numShardTypes != 0) {
      throw std::runtime_error(
          fmt::format(
              "expected numShardsPerTable % numShardTypes != 0, but got numShardsPerTable={}, numShardTypes={}",
              config.numShardsPerTable,
              config.numShardTypes));
    }
    const auto numShardsInShardType =
        config.numShardsPerTable / config.numShardTypes;
    const entities::Map<std::string, double> runtimeDim = generateRuntimeDim(
        config.numTables,
        config.numShardsPerTable,
        config.numShardTypes,
        numShardsInShardType);
    solver->addObjectDimension("runtimeDim", runtimeDim);

    return solver;
  }

  static void addExactlyOneShardTypePerTableSpec(ProblemSolver& solver) {
    GroupCountSpec atLeastOneShardGroupCountSpec;
    atLeastOneShardGroupCountSpec.scope() = "worlds";
    atLeastOneShardGroupCountSpec.partitionName() = "tables";
    atLeastOneShardGroupCountSpec.dimension() = "shardTypeInTableDim";
    atLeastOneShardGroupCountSpec.name() = "exactly-one-shard-type-per-table";
    atLeastOneShardGroupCountSpec.filter()->itemsBlacklist() = {"fakeWorld"};
    Limit atLeastOneShardGroupCountSpecLimit;
    atLeastOneShardGroupCountSpecLimit.type() = LimitType::ABSOLUTE;
    atLeastOneShardGroupCountSpecLimit.scopeItemLimits() = {{"realWorld", 1}};
    atLeastOneShardGroupCountSpec.limit() = atLeastOneShardGroupCountSpecLimit;
    atLeastOneShardGroupCountSpec.bound() = GroupCountSpecBound::EXACT;
    solver.addConstraint(atLeastOneShardGroupCountSpec);
  }

  static void addColcoateAllShardsOfSameType(ProblemSolver& solver) {
    ColocateGroupsSpec colocateGroupSpec;
    colocateGroupSpec.scope() = "worlds";
    colocateGroupSpec.partitionName() = "shardTypesInTable";
    colocateGroupSpec.name() = "colocate-all-shards-of-same-table-and-type";
    solver.addConstraint(colocateGroupSpec);
  }

  // check if at least one shard is allocated from the table and all the
  // allocated shards are of the same type
  bool isAllocated(
      const std::string& tableName,
      const AssignmentSolution& solution) {
    entities::Set<std::string> allocatedFromTable;
    const auto& shards = tableNameToShards.at(tableName);
    for (const auto& shard : shards) { // table1 shards
      if (solution.assignment()->at(shard) != DUMMY_RANK) {
        allocatedFromTable.insert(shard);
      }
    }

    if (allocatedFromTable.empty()) {
      return false;
    }

    entities::Set<std::string> allocatedShardTypes;
    for (const auto& shard : allocatedFromTable) {
      const auto& shardType = shardToShardType.at(shard);
      allocatedShardTypes.insert(shardType);
    }

    if (allocatedShardTypes.size() > 1) {
      throw std::runtime_error(
          fmt::format(
              "table {} has more than one shard type allocated in the final solution",
              tableName));
    }

    return true;
  }

 protected:
  entities::Map<std::string, std::vector<std::string>> tableNameToShards;
  entities::Map<std::string, std::vector<std::string>> shardTypeToShards;
  entities::Map<std::string, std::string> shardToShardType;
  const inline static std::string DUMMY_RANK = "dummyRank";
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    ColocateGroupsWithHintStrategiesTest,
    testThreadCounts());

TEST_P(ColocateGroupsWithHintStrategiesTest, OnlyWithReplacement) {
  auto solver = createBasicScenario(
      {.numShards = 24,
       .numTables = 3,
       .numShardsPerTable = 8,
       .numShardTypes = 4,
       .numNodes = 4,
       .numRanksPerNode = 2,
       .numRanks = 8});

  // exactly-one-shard-type-per-table
  addExactlyOneShardTypePerTableSpec(*solver);

  // colocate all the shards of the same (table, group)
  addColcoateAllShardsOfSameType(*solver);

  LocalSearchSolverSpec localSearchSpec;
  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveWithHintStrategiesSpec;

  groupMoveWithHintStrategiesSpec.primaryPartition() = "tables";
  groupMoveWithHintStrategiesSpec.secondaryPartition() = "shardTypes";

  std::map<std::string, MoveStrategy> hintMap;
  MoveStrategy option1;
  option1.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  MoveToScopeItemsSpec moveToScopeItemsSpec;
  ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "worlds";
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;
  option1.moveToScopeItems() = moveToScopeItemsSpec;

  hintMap.emplace("shardType0", option1);
  hintMap.emplace("shardType1", option1);
  hintMap.emplace("shardType2", option1);
  hintMap.emplace("shardType3", option1);

  MoveStrategies groupToMoveStrategy;
  groupToMoveStrategy.groupToMoveStrategy() = hintMap;

  groupMoveWithHintStrategiesSpec.moveStrategies() = groupToMoveStrategy;

  localSearchSpec.moveTypeList()->emplace_back(
      ProblemSolver::makeMoveTypeSpec(groupMoveWithHintStrategiesSpec));

  localSearchSpec.exploreMovesFromContainersNotInObjective() = true;
  solver->addSolver(localSearchSpec);
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // Exact shard-to-rank mapping depends on solver iteration order.
  // Verify structural properties: correct number allocated and constraints met.
  int numAssigned = 0;
  for (const auto i : folly::irange(24)) {
    if (assignment[fmt::format("shard{}", i)] != DUMMY_RANK) {
      ++numAssigned;
    }
  }
  EXPECT_EQ(6, numAssigned);
  // Verify colocation: each table's allocated shards are the same type.
  for (const auto t : folly::irange(3)) {
    EXPECT_TRUE(isAllocated(fmt::format("table{}", t), solution));
  }
}

TEST_P(ColocateGroupsWithHintStrategiesTest, OnlyWithoutReplacement) {
  auto solver = createBasicScenario(
      {.numShards = 24,
       .numTables = 3,
       .numShardsPerTable = 8,
       .numShardTypes = 4,
       .numNodes = 4,
       .numRanksPerNode = 2,
       .numRanks = 8});

  // exactly-one-shard-type-per-table
  addExactlyOneShardTypePerTableSpec(*solver);

  // colocate all the shards of the same (table, group)
  addColcoateAllShardsOfSameType(*solver);

  // one shard per rank
  entities::Map<std::string, double> oneShardPerRankDimension;
  for (const auto i : folly::irange(24)) {
    oneShardPerRankDimension[fmt::format("shard{}", i)] = 1;
  }
  solver->addObjectDimension("oneShardPerRank", oneShardPerRankDimension);
  Limit rankLimit;
  rankLimit.type() = LimitType::ABSOLUTE;
  rankLimit.globalLimit() = 24;
  std::map<std::string, double> rankLimitMap;
  for (const auto i : folly::irange(8)) {
    rankLimitMap[fmt::format("rank{}", i)] = 1;
  }
  rankLimit.scopeItemLimits() = rankLimitMap;

  GroupCountSpec oneShardPerRankGroupCountSpec;
  oneShardPerRankGroupCountSpec.scope() = "ranks";
  oneShardPerRankGroupCountSpec.partitionName() = "tables";
  oneShardPerRankGroupCountSpec.dimension() = "oneShardPerRank";
  oneShardPerRankGroupCountSpec.limit() = rankLimit;
  oneShardPerRankGroupCountSpec.name() = "dp-shards-at-most-one-per-rank";

  solver->addConstraint(oneShardPerRankGroupCountSpec);

  // one node used for shards

  LocalSearchSolverSpec localSearchSpec;
  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveWithHintStrategiesSpec;

  groupMoveWithHintStrategiesSpec.primaryPartition() = "tables";
  groupMoveWithHintStrategiesSpec.secondaryPartition() = "shardTypes";

  std::map<std::string, MoveStrategy> hintMap;
  MoveStrategy option1;
  option1.type() = MoveStrategyType::RANDOM_SAMPLING_WITHOUT_REPLACEMENT;
  MoveToScopeItemsSpec moveToScopeItemsSpec;
  ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "nodes";
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;
  option1.moveToScopeItems() = moveToScopeItemsSpec;

  hintMap.emplace("shardType0", option1);
  hintMap.emplace("shardType1", option1);
  hintMap.emplace("shardType2", option1);
  hintMap.emplace("shardType3", option1);

  MoveStrategies groupToMoveStrategy;
  groupToMoveStrategy.groupToMoveStrategy() = hintMap;

  groupMoveWithHintStrategiesSpec.moveStrategies() = groupToMoveStrategy;

  localSearchSpec.moveTypeList()->emplace_back(
      ProblemSolver::makeMoveTypeSpec(groupMoveWithHintStrategiesSpec));

  localSearchSpec.exploreMovesFromContainersNotInObjective() = true;
  solver->addSolver(localSearchSpec);
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // Exact shard-to-rank mapping depends on solver iteration order.
  // Verify structural properties: correct number allocated and constraints met.
  int numAssigned = 0;
  for (const auto i : folly::irange(24)) {
    if (assignment[fmt::format("shard{}", i)] != DUMMY_RANK) {
      ++numAssigned;
    }
  }
  EXPECT_EQ(6, numAssigned);
  for (const auto t : folly::irange(3)) {
    EXPECT_TRUE(isAllocated(fmt::format("table{}", t), solution));
  }
}

TEST_P(ColocateGroupsWithHintStrategiesTest, AllHintOptions) {
  auto solver = createBasicScenario(
      {.numShards = 24,
       .numTables = 3,
       .numShardsPerTable = 8,
       .numShardTypes = 4,
       .numNodes = 4,
       .numRanksPerNode = 2,
       .numRanks = 8});

  // exactly-one-shard-type-per-table
  addExactlyOneShardTypePerTableSpec(*solver);

  // colocate all the shards of the same (table, group)
  addColcoateAllShardsOfSameType(*solver);

  // one shard per rank for shardType0
  entities::Map<std::string, double> oneShardPerRankDimension;
  for (const auto i : folly::irange(24)) {
    if (i % 4 == 0) {
      oneShardPerRankDimension[fmt::format("shard{}", i)] = 1;
    } else {
      oneShardPerRankDimension[fmt::format("shard{}", i)] = 0;
    }
  }
  solver->addObjectDimension("oneShardPerRank", oneShardPerRankDimension);
  Limit rankLimit;
  rankLimit.type() = LimitType::ABSOLUTE;
  rankLimit.globalLimit() = 24;
  std::map<std::string, double> rankLimitMap;
  for (const auto i : folly::irange(8)) {
    rankLimitMap[fmt::format("rank{}", i)] = 1;
  }
  rankLimit.scopeItemLimits() = rankLimitMap;

  GroupCountSpec oneShardPerRankGroupCountSpec;
  oneShardPerRankGroupCountSpec.scope() = "ranks";
  oneShardPerRankGroupCountSpec.partitionName() = "tables";
  oneShardPerRankGroupCountSpec.dimension() = "oneShardPerRank";
  oneShardPerRankGroupCountSpec.limit() = rankLimit;
  oneShardPerRankGroupCountSpec.name() = "dp-shards-at-most-one-per-rank";

  solver->addConstraint(oneShardPerRankGroupCountSpec);

  // one node used for shardType1
  entities::Map<std::string, std::vector<std::string>> twrwPartition;
  for (const auto t : folly::irange(3)) {
    for (const auto s : folly::irange(8)) {
      int shardIndex = t * 8 + s;
      auto shard = fmt::format("shard{}", shardIndex);
      if (shardIndex % 4 == 1) {
        twrwPartition[fmt::format("table{}", t)].emplace_back(shard);
      }
    }
  }
  solver->addPartition("twrwPartition", twrwPartition);

  ColocateGroupsSpec oneNodePerShardType1ColocateGroupSpec;
  oneNodePerShardType1ColocateGroupSpec.scope() = "nodes";
  oneNodePerShardType1ColocateGroupSpec.partitionName() = "twrwPartition";
  oneNodePerShardType1ColocateGroupSpec.name() = "twrw-twcw-colocate-on-node";
  solver->addConstraint(oneNodePerShardType1ColocateGroupSpec);

  LocalSearchSolverSpec localSearchSpec;
  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveWithHintStrategiesSpec;

  groupMoveWithHintStrategiesSpec.primaryPartition() = "tables";
  groupMoveWithHintStrategiesSpec.secondaryPartition() = "shardTypes";

  std::map<std::string, MoveStrategy> hintMap;
  MoveStrategy option1;
  option1.type() = MoveStrategyType::RANDOM_SAMPLING_WITHOUT_REPLACEMENT;
  MoveToScopeItemsSpec moveToScopeItemsSpec1;
  ScopeItemList defaultScopeItems1;
  defaultScopeItems1.scopeName() = "nodes";
  moveToScopeItemsSpec1.defaultScopeItems() = defaultScopeItems1;
  option1.moveToScopeItems() = moveToScopeItemsSpec1;

  MoveStrategy option2;
  option2.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  MoveToScopeItemsSpec moveToScopeItemsSpec2;
  ScopeItemList defaultScopeItems2;
  defaultScopeItems2.scopeName() = "nodes";
  moveToScopeItemsSpec2.defaultScopeItems() = defaultScopeItems2;
  option2.moveToScopeItems() = moveToScopeItemsSpec2;

  MoveStrategy option3;
  option3.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  MoveToScopeItemsSpec moveToScopeItemsSpec3;
  ScopeItemList defaultScopeItems3;
  defaultScopeItems3.scopeName() = "worlds";
  moveToScopeItemsSpec3.defaultScopeItems() = defaultScopeItems3;
  option3.moveToScopeItems() = moveToScopeItemsSpec3;

  hintMap.emplace("shardType0", option1); // can only have one shard per rank
  hintMap.emplace("shardType1", option2); // all shards on the same node
  hintMap.emplace("shardType2", option3); // can be in any rank
  hintMap.emplace("shardType3", option3); // can be in any rank

  MoveStrategies groupToMoveStrategy;
  groupToMoveStrategy.groupToMoveStrategy() = hintMap;

  groupMoveWithHintStrategiesSpec.moveStrategies() = groupToMoveStrategy;

  localSearchSpec.moveTypeList()->emplace_back(
      ProblemSolver::makeMoveTypeSpec(groupMoveWithHintStrategiesSpec));

  localSearchSpec.exploreMovesFromContainersNotInObjective() = true;
  solver->addSolver(localSearchSpec);
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // Exact shard-to-rank mapping depends on solver iteration order.
  // Verify structural properties: correct number allocated and constraints met.
  int numAssigned = 0;
  for (const auto i : folly::irange(24)) {
    if (assignment[fmt::format("shard{}", i)] != DUMMY_RANK) {
      ++numAssigned;
    }
  }
  EXPECT_EQ(6, numAssigned);
  for (const auto t : folly::irange(3)) {
    EXPECT_TRUE(isAllocated(fmt::format("table{}", t), solution));
  }
}

TEST_P(ColocateGroupsWithHintStrategiesTest, TertiaryPartition) {
  auto solver = createBasicScenario(
      {.numShards = 24,
       .numTables = 3,
       .numShardsPerTable = 8,
       .numShardTypes = 1,
       .numNodes = 4,
       .numRanksPerNode = 2,
       .numRanks = 8});
  // add tertiary partition
  entities::Map<std::string, std::string> tertiaryPartition;
  for (const auto t : folly::irange(3)) {
    for (const auto s : folly::irange(8)) {
      int shardIndex = t * 8 + s;
      auto shardName = fmt::format("shard{}", shardIndex);
      auto tertiaryGroup = s % 4;
      tertiaryPartition[fmt::format("shard{}", shardIndex)] =
          fmt::format("table{}_tertiaryPartition{}", t, tertiaryGroup);
    }
  }
  solver->addPartition("tertiaryPartition", tertiaryPartition);

  // exactly-one-shard-type-per-table
  addExactlyOneShardTypePerTableSpec(*solver);

  // colocate all the shards of the same (table, group)
  addColcoateAllShardsOfSameType(*solver);

  // colocate tertiary partition to the same node
  ColocateGroupsSpec sameTertiaryPartitionInSameNodeColocateGroupSpec;
  sameTertiaryPartitionInSameNodeColocateGroupSpec.scope() = "nodes";
  sameTertiaryPartitionInSameNodeColocateGroupSpec.partitionName() =
      "tertiaryPartition";
  sameTertiaryPartitionInSameNodeColocateGroupSpec.name() =
      "gs-colocate-on-node";
  solver->addConstraint(sameTertiaryPartitionInSameNodeColocateGroupSpec);

  LocalSearchSolverSpec localSearchSpec;
  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveWithHintStrategiesSpec;

  groupMoveWithHintStrategiesSpec.primaryPartition() = "tables";
  groupMoveWithHintStrategiesSpec.secondaryPartition() = "shardTypes";

  std::map<std::string, MoveStrategy> hintMap;
  MoveStrategy option1;
  option1.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  MoveToScopeItemsSpec moveToScopeItemsSpec;
  ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "nodes";
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;
  option1.moveToScopeItems() = moveToScopeItemsSpec;
  option1.tertiaryPartition() = "tertiaryPartition";
  option1.numScopeItemsToExplorePerTertiaryGroup() = 100;

  hintMap.emplace("shardType0", option1);

  MoveStrategies groupToMoveStrategy;
  groupToMoveStrategy.groupToMoveStrategy() = hintMap;

  groupMoveWithHintStrategiesSpec.moveStrategies() = groupToMoveStrategy;

  localSearchSpec.moveTypeList()->emplace_back(
      ProblemSolver::makeMoveTypeSpec(groupMoveWithHintStrategiesSpec));

  localSearchSpec.exploreMovesFromContainersNotInObjective() = true;

  solver->addSolver(localSearchSpec);
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  entities::Map<std::string, std::string> nodeMap;
  for (const auto i : folly::irange(8)) {
    nodeMap[fmt::format("rank{}", i)] = fmt::format("node{}", i / 2);
  }
  entities::Map<std::string, entities::Map<std::string, int>>
      expectedTertiaryPartitionAssignment;

  for (const auto t : folly::irange(3)) {
    for (const auto s : folly::irange(8)) {
      int shardIndex = t * 8 + s;
      auto shardName = fmt::format("shard{}", shardIndex);
      auto tertiaryGroup = s % 4;
      auto tertiaryPartitionName =
          fmt::format("table{}_tertiaryPartition{}", t, tertiaryGroup);
      ASSERT_NE(assignment[shardName], DUMMY_RANK);
      expectedTertiaryPartitionAssignment[tertiaryPartitionName]
                                         [nodeMap[assignment[shardName]]] += 1;
    }
  }

  for (const auto& [g, m] : expectedTertiaryPartitionAssignment) {
    EXPECT_EQ(m.size(), 1);
    for (const auto& [node, count] : m) {
      EXPECT_EQ(count, 2);
    }
  }
}

TEST_P(
    ColocateGroupsWithHintStrategiesTest,
    OneTableNotAllocatableOthersAllocated) {
  auto solver = createBasicScenario(
      {.numShards = 24,
       .numTables = 3,
       .numShardsPerTable = 8,
       .numShardTypes = 1,
       .numNodes = 4,
       .numRanksPerNode = 2,
       .numRanks = 8});

  // exactly-one-shard-type-per-table
  addExactlyOneShardTypePerTableSpec(*solver);

  // colocate all the shards of the same (table, group)
  addColcoateAllShardsOfSameType(*solver);

  { // Add constraint that makes table0 impossible to allocate
    // Create a dimension that only applies to table0 shards
    entities::Map<std::string, double> table0OnlyDimension;
    for (const auto& shard : tableNameToShards["table0"]) {
      table0OnlyDimension[shard] = 1;
    }
    solver->addObjectDimension(
        "table0Only", table0OnlyDimension, /*defaultValue=*/0);

    GroupCountSpec impossibleConstraint;
    impossibleConstraint.scope() = "ranks";
    impossibleConstraint.partitionName() = "tables";
    impossibleConstraint.dimension() = "table0Only";
    impossibleConstraint.limit()->globalLimit() = 0;
    impossibleConstraint.name() = "impossible-table0-constraint";
    solver->addConstraint(impossibleConstraint);
  }

  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveWithHintStrategiesSpec;
  groupMoveWithHintStrategiesSpec.primaryPartition() = "tables";
  groupMoveWithHintStrategiesSpec.secondaryPartition() = "shardTypes";

  MoveStrategy strategy;
  strategy.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  MoveToScopeItemsSpec moveToScopeItemsSpec;
  ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "worlds";
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;
  strategy.moveToScopeItems() = moveToScopeItemsSpec;

  MoveStrategies groupToMoveStrategy;
  auto& thriftGroupToMoveStrategy = *groupToMoveStrategy.groupToMoveStrategy();
  thriftGroupToMoveStrategy.emplace("shardType0", strategy);
  groupMoveWithHintStrategiesSpec.moveStrategies() = groupToMoveStrategy;

  LocalSearchSolverSpec localSearchSpec;
  localSearchSpec.moveTypeList()->emplace_back(
      ProblemSolver::makeMoveTypeSpec(groupMoveWithHintStrategiesSpec));

  // forces table0 to be allocated first since those shards have the highest
  // dimension value
  localSearchSpec.objectOrderingDimension() = "table0Only";

  solver->addSolver(localSearchSpec);

  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // expect tables1 and 2 to be allocated, table0 to be unallocated
  EXPECT_FALSE(isAllocated("table0", solution));
  EXPECT_TRUE(isAllocated("table1", solution));
  EXPECT_TRUE(isAllocated("table2", solution));
}

TEST_P(ColocateGroupsWithHintStrategiesTest, GroupMoveSwapIsNecessary) {
  auto solver = createBasicScenario(
      {.numShards = 24,
       .numTables = 3,
       .numShardsPerTable = 8,
       .numShardTypes = 4,
       .numNodes = 1,
       .numRanksPerNode = 2,
       .numRanks = 8,
       .swap = true});

  // exactly-one-shard-type-per-table
  addExactlyOneShardTypePerTableSpec(*solver);

  // colocate all the shards of the same (table, group)
  addColcoateAllShardsOfSameType(*solver);

  std::vector<LocalSearchStageSpec> stageSpecs = {};

  {
    LocalSearchSolverSpec localSearchSpec;
    GroupMoveWithHintStrategiesMoveTypeSpec
        groupMoveWithHintStrategiesMoveTypeSpec;

    groupMoveWithHintStrategiesMoveTypeSpec.primaryPartition() = "tables";
    groupMoveWithHintStrategiesMoveTypeSpec.secondaryPartition() = "shardTypes";
    groupMoveWithHintStrategiesMoveTypeSpec.unassignedContainer() = "dummyRank";

    std::map<std::string, MoveStrategy> hintMap;

    MoveStrategy option3;
    option3.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
    MoveToScopeItemsSpec moveToScopeItemsSpec3;
    ScopeItemList defaultScopeItems3;
    defaultScopeItems3.scopeName() = "worlds";
    moveToScopeItemsSpec3.defaultScopeItems() = defaultScopeItems3;
    option3.moveToScopeItems() = moveToScopeItemsSpec3;

    hintMap.emplace("shardType0", option3); // can be in any rank
    hintMap.emplace("shardType1", option3); // can be in any rank
    hintMap.emplace("shardType2", option3); // can be in any rank
    hintMap.emplace("shardType3", option3); // can be in any rank

    MoveStrategies groupToMoveStrategy;
    groupToMoveStrategy.groupToMoveStrategy() = hintMap;

    groupMoveWithHintStrategiesMoveTypeSpec.moveStrategies() =
        groupToMoveStrategy;

    localSearchSpec.moveTypeList()->emplace_back(
        ProblemSolver::makeMoveTypeSpec(
            groupMoveWithHintStrategiesMoveTypeSpec));

    localSearchSpec.exploreMovesFromContainersNotInObjective() = true;

    LocalSearchStageSpec localSearchStageSpec;
    localSearchStageSpec.solverSpec() = localSearchSpec;
    localSearchStageSpec.begin() = 0;
    localSearchStageSpec.end() = 2;
    localSearchStageSpec.solverSpec()
        ->exploreMovesFromContainersNotInObjective() = true;
    localSearchStageSpec.name() = "Swapping";

    stageSpecs.push_back(localSearchStageSpec);
  }

  LocalSearchStageSolverSpec localSearchStageSolverSpec;
  localSearchStageSolverSpec.stageSpecs() = stageSpecs;
  solver->addSolver(localSearchStageSolverSpec);
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // Exact shard-to-rank mapping depends on solver iteration order.
  // Verify structural properties: correct number allocated and constraints met.
  int numAssigned = 0;
  for (const auto i : folly::irange(24)) {
    if (assignment[fmt::format("shard{}", i)] != DUMMY_RANK) {
      ++numAssigned;
    }
  }
  EXPECT_EQ(6, numAssigned);
  for (const auto t : folly::irange(3)) {
    EXPECT_TRUE(isAllocated(fmt::format("table{}", t), solution));
  }
}

TEST_P(
    ColocateGroupsWithHintStrategiesTest,
    ZeroMoveSetGeneratedForSecondaryGroup) {
  auto solver = createBasicScenario(
      {.numShards = 24,
       .numTables = 3,
       .numShardsPerTable = 8,
       .numShardTypes = 4,
       .numNodes = 4,
       .numRanksPerNode = 2,
       .numRanks = 8});

  // exactly-one-shard-type-per-table
  addExactlyOneShardTypePerTableSpec(*solver);

  // colocate all the shards of the same (table, group)
  addColcoateAllShardsOfSameType(*solver);

  LocalSearchSolverSpec localSearchSpec;
  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveWithHintStrategiesSpec;

  groupMoveWithHintStrategiesSpec.primaryPartition() = "tables";
  groupMoveWithHintStrategiesSpec.secondaryPartition() = "shardTypes";

  std::map<std::string, MoveStrategy> hintMap;
  MoveStrategy option1;
  option1.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  MoveToScopeItemsSpec moveToScopeItemsSpec;
  ScopeItemList defaultScopeItems;
  defaultScopeItems.scopeName() = "worlds";
  moveToScopeItemsSpec.defaultScopeItems() = defaultScopeItems;
  option1.moveToScopeItems() = moveToScopeItemsSpec;

  MoveStrategy option2;
  option2.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  option2.moveToScopeItems() = moveToScopeItemsSpec;
  option2.moveSetsGeneratedPerScopeItem() = 0;

  hintMap.emplace("shardType0", option1);
  hintMap.emplace("shardType1", option2);
  hintMap.emplace("shardType2", option1);
  hintMap.emplace("shardType3", option2);

  MoveStrategies groupToMoveStrategy;
  groupToMoveStrategy.groupToMoveStrategy() = hintMap;

  groupMoveWithHintStrategiesSpec.moveStrategies() = groupToMoveStrategy;

  localSearchSpec.moveTypeList()->emplace_back(
      ProblemSolver::makeMoveTypeSpec(groupMoveWithHintStrategiesSpec));

  localSearchSpec.exploreMovesFromContainersNotInObjective() = true;
  solver->addSolver(localSearchSpec);
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // we expect shardType1 and shardType3 to be unallocated
  int numShardsAllocated = 0;
  for (const auto i : folly::irange(24)) {
    if (i % 4 == 1 || i % 4 == 3) {
      EXPECT_EQ(assignment[fmt::format("shard{}", i)], DUMMY_RANK);
    }
    if (assignment[fmt::format("shard{}", i)] != DUMMY_RANK) {
      numShardsAllocated++;
    }
  }
  EXPECT_EQ(numShardsAllocated, 6);
}

} // namespace facebook::rebalancer::interface::tests
