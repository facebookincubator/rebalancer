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

/**
 * GroupMoveWithHintStrategies Move Type Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
 * */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

// quick_example_start
void quickExample() {
  // Apply different strategies for different shard types
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("rank");

  // Setup problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"node1", {}},
          {"node2", {"shard1", "shard2"}},
      });

  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"shard1", 3.0},
          {"shard2", 3.0},
      });

  // Define partitions
  std::unordered_map<std::string, std::vector<std::string>> tables = {
      {"table1", {"shard1", "shard2"}},
  };
  solver.addPartition("table", std::move(tables));

  std::unordered_map<std::string, std::vector<std::string>> shardTypes = {
      {"row_wise", {"shard1"}},
      {"column_wise", {"shard2"}},
  };
  solver.addPartition("shard_type", std::move(shardTypes));

  LocalSearchSolverSpec solverSpec;

  // Create strategies for each shard type
  MoveStrategy rowWiseStrategy;
  rowWiseStrategy.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  MoveToScopeItemsSpec rowWiseDest;
  rowWiseDest.defaultScopeItems() = ScopeItemList();
  rowWiseDest.defaultScopeItems()->scopeName() = "rank";
  rowWiseDest.defaultScopeItems()->scopeItems() = {"node1", "node2"};
  rowWiseStrategy.moveToScopeItems() = std::move(rowWiseDest);

  MoveStrategy columnWiseStrategy;
  columnWiseStrategy.type() =
      MoveStrategyType::RANDOM_SAMPLING_WITHOUT_REPLACEMENT;
  MoveToScopeItemsSpec columnWiseDest;
  columnWiseDest.defaultScopeItems() = ScopeItemList();
  columnWiseDest.defaultScopeItems()->scopeName() = "rank";
  columnWiseDest.defaultScopeItems()->scopeItems() = {"node1", "node2"};
  columnWiseStrategy.moveToScopeItems() = std::move(columnWiseDest);

  MoveStrategies moveStrategies;
  moveStrategies.groupToMoveStrategy() = {
      {"row_wise", rowWiseStrategy},
      {"column_wise", columnWiseStrategy},
  };

  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveSpec;
  groupMoveSpec.primaryPartition() = "table";
  groupMoveSpec.secondaryPartition() = "shard_type";
  groupMoveSpec.moveStrategies() = std::move(moveStrategies);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().groupMoveWithHintStrategiesMoveTypeSpec() =
      groupMoveSpec;

  solver.addSolver(solverSpec);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "rank";
  balanceSpec.dimension() = "memory";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial objective: " << std::fixed << std::setprecision(4)
            << *solution.initialObjective()->value() << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// quick_example_end

// torchrec_start
void torchrec() {
  // TorchRec table sharding with different strategies per shard type
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("rank");

  // Setup imbalanced problem
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"node1", {"shard1", "shard2", "shard3"}},
          {"node2", {}},
          {"node3", {}},
      });

  solver.addObjectDimension(
      "compute",
      std::map<std::string, double>{
          {"shard1", 2.0},
          {"shard2", 2.0},
          {"shard3", 2.0},
      });

  std::unordered_map<std::string, std::vector<std::string>> tables = {
      {"embeddings", {}},
      {"features", {}},
  };
  solver.addPartition("table", std::move(tables));

  std::unordered_map<std::string, std::vector<std::string>> shardTypes = {
      {"row_wise", {}},
      {"column_wise", {}},
      {"data_parallel", {}},
  };
  solver.addPartition("shard_type", std::move(shardTypes));

  LocalSearchSolverSpec solverSpec;

  MoveToScopeItemsSpec defaultDest;
  defaultDest.defaultScopeItems() = ScopeItemList();
  defaultDest.defaultScopeItems()->scopeName() = "rank";
  defaultDest.defaultScopeItems()->scopeItems() = {"node1", "node2", "node3"};

  MoveStrategy rowWiseStrategy;
  rowWiseStrategy.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  rowWiseStrategy.moveToScopeItems() = defaultDest;

  MoveStrategy columnWiseStrategy;
  columnWiseStrategy.type() =
      MoveStrategyType::RANDOM_SAMPLING_WITHOUT_REPLACEMENT;
  columnWiseStrategy.moveToScopeItems() = defaultDest;

  MoveStrategy dataParallelStrategy;
  dataParallelStrategy.type() =
      MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  dataParallelStrategy.moveToScopeItems() = std::move(defaultDest);

  MoveStrategies moveStrategies;
  moveStrategies.groupToMoveStrategy() = {
      {"row_wise", rowWiseStrategy},
      {"column_wise", columnWiseStrategy},
      {"data_parallel", dataParallelStrategy},
  };

  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveSpec;
  groupMoveSpec.primaryPartition() = "table";
  groupMoveSpec.secondaryPartition() = "shard_type";
  groupMoveSpec.moveStrategies() = std::move(moveStrategies);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().groupMoveWithHintStrategiesMoveTypeSpec() =
      groupMoveSpec;

  solver.addSolver(solverSpec);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-compute";
  balanceSpec.scope() = "rank";
  balanceSpec.dimension() = "compute";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Solve and print results
  auto solution = solver.solve();
  std::cout << "  Initial objective: " << std::fixed << std::setprecision(4)
            << *solution.initialObjective()->value() << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}
// torchrec_end

// sampling_types_start
void samplingTypes() {
  // With replacement for flexible placement, without for exclusive
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> services = {
      {"web", {}},
      {"db", {}},
  };
  solver.addPartition("service", std::move(services));

  std::unordered_map<std::string, std::vector<std::string>> tiers = {
      {"primary", {}},
      {"replica", {}},
  };
  solver.addPartition("tier", std::move(tiers));

  LocalSearchSolverSpec solverSpec;

  MoveToScopeItemsSpec defaultDest;
  defaultDest.defaultScopeItems() = ScopeItemList();
  defaultDest.defaultScopeItems()->scopeItems() = {"rack1", "rack2"};

  MoveStrategy primaryStrategy;
  primaryStrategy.type() =
      MoveStrategyType::RANDOM_SAMPLING_WITHOUT_REPLACEMENT; // Exclusive
  primaryStrategy.moveToScopeItems() = defaultDest;

  MoveStrategy replicaStrategy;
  replicaStrategy.type() =
      MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT; // Flexible
  replicaStrategy.moveToScopeItems() = std::move(defaultDest);

  MoveStrategies moveStrategies;
  moveStrategies.groupToMoveStrategy() = {
      {"primary", primaryStrategy},
      {"replica", replicaStrategy},
  };

  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveSpec;
  groupMoveSpec.primaryPartition() = "service";
  groupMoveSpec.secondaryPartition() = "tier";
  groupMoveSpec.moveStrategies() = std::move(moveStrategies);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().groupMoveWithHintStrategiesMoveTypeSpec() =
      groupMoveSpec;

  solver.addSolver(solverSpec);
}
// sampling_types_end

// multiple_movesets_start
void multipleMovesets() {
  // Generate multiple move sets for better solution quality
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> tables = {
      {"table1", {}},
  };
  solver.addPartition("table", std::move(tables));

  std::unordered_map<std::string, std::vector<std::string>> shardTypes = {
      {"type_a", {}},
      {"type_b", {}},
  };
  solver.addPartition("shard_type", std::move(shardTypes));

  LocalSearchSolverSpec solverSpec;

  MoveToScopeItemsSpec defaultDest;
  defaultDest.defaultScopeItems() = ScopeItemList();
  defaultDest.defaultScopeItems()->scopeItems() = {"node1", "node2"};

  MoveStrategy typeAStrategy;
  typeAStrategy.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  typeAStrategy.moveSetsGeneratedPerScopeItem() = 5; // Generate 5 move sets
  typeAStrategy.moveToScopeItems() = defaultDest;

  MoveStrategy typeBStrategy;
  typeBStrategy.type() = MoveStrategyType::RANDOM_SAMPLING_WITHOUT_REPLACEMENT;
  typeBStrategy.moveSetsGeneratedPerScopeItem() = 3; // Generate 3 move sets
  typeBStrategy.moveToScopeItems() = std::move(defaultDest);

  MoveStrategies moveStrategies;
  moveStrategies.groupToMoveStrategy() = {
      {"type_a", typeAStrategy},
      {"type_b", typeBStrategy},
  };

  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveSpec;
  groupMoveSpec.primaryPartition() = "table";
  groupMoveSpec.secondaryPartition() = "shard_type";
  groupMoveSpec.moveStrategies() = std::move(moveStrategies);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().groupMoveWithHintStrategiesMoveTypeSpec() =
      groupMoveSpec;

  solver.addSolver(solverSpec);
}
// multiple_movesets_end

// unassigned_start
void unassigned() {
  // Use unassigned container to enable group replacement
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> tables = {
      {"table1", {}},
  };
  solver.addPartition("table", std::move(tables));

  std::unordered_map<std::string, std::vector<std::string>> shardTypes = {
      {"type_a", {}},
      {"type_b", {}},
      {"type_c", {}},
  };
  solver.addPartition("shard_type", std::move(shardTypes));

  LocalSearchSolverSpec solverSpec;

  MoveToScopeItemsSpec defaultDest;
  defaultDest.defaultScopeItems() = ScopeItemList();
  defaultDest.defaultScopeItems()->scopeItems() = {"node1", "node2"};

  MoveStrategy sharedStrategy;
  sharedStrategy.type() = MoveStrategyType::RANDOM_SAMPLING_WITH_REPLACEMENT;
  sharedStrategy.moveToScopeItems() = std::move(defaultDest);

  MoveStrategies moveStrategies;
  moveStrategies.groupToMoveStrategy() = {
      {"type_a", sharedStrategy},
      {"type_b", sharedStrategy},
      {"type_c", sharedStrategy},
  };

  SecondaryGroupReplacementConfig replacementConfig;
  replacementConfig.secondaryGroupToAllowedReplacements() = {
      {"type_a",
       {"type_b", "type_c"}}, // type_a can be replaced by type_b or type_c
      {"type_b", {"type_a"}}, // type_b can be replaced by type_a
      // type_c has no entry, can be replaced by any
  };

  GroupMoveWithHintStrategiesMoveTypeSpec groupMoveSpec;
  groupMoveSpec.primaryPartition() = "table";
  groupMoveSpec.secondaryPartition() = "shard_type";
  groupMoveSpec.unassignedContainer() = "unassigned"; // Enable replacement
  groupMoveSpec.secondaryGroupReplacementConfig() =
      std::move(replacementConfig);
  groupMoveSpec.moveStrategies() = std::move(moveStrategies);

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().groupMoveWithHintStrategiesMoveTypeSpec() =
      groupMoveSpec;

  solver.addSolver(solverSpec);
}
// unassigned_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout
      << "=== Quick example showing basic GroupMoveWithHintStrategies usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
