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
 * ColocateGroups Move Type Reference Examples
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
  // Colocate primary and replicas in same region
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  // Setup problem with replicas scattered across regions
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server1", {"shard1_p", "shard2_r2"}},
          {"server2", {"shard1_r2"}},
          {"server3", {"shard1_r1", "shard2_p"}},
          {"server4", {"shard2_r1"}},
      });

  solver.addScope(
      "region",
      std::map<std::string, std::string>{
          {"server1", "us-east"},
          {"server2", "us-east"},
          {"server3", "us-west"},
          {"server4", "us-west"},
      });

  solver.addObjectDimension(
      "size",
      std::map<std::string, double>{
          {"shard1_p", 1.0},
          {"shard1_r1", 1.0},
          {"shard1_r2", 1.0},
          {"shard2_p", 1.0},
          {"shard2_r1", 1.0},
          {"shard2_r2", 1.0},
      });

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-size";
  balanceSpec.scope() = "server";
  balanceSpec.dimension() = "size";
  solver.addGoal(std::move(balanceSpec), 1.0);

  // Define partition with replica groups
  const std::unordered_map<std::string, std::vector<std::string>>
      replicaGroups = {
          {"primary", {"shard1_p", "shard2_p"}},
          {"replica1", {"shard1_r1", "shard2_r1"}},
          {"replica2", {"shard1_r2", "shard2_r2"}},
      };
  solver.addPartition("replica_group", replicaGroups);

  LocalSearchSolverSpec solverSpec;

  ColocateGroupsMoveTypeRelatedGroupsInfo relatedInfo;
  relatedInfo.relatedGroups() = {"primary", "replica1", "replica2"};

  ColocateGroupsMoveTypeSpec colocateSpec;
  colocateSpec.partitionName() = "replica_group";
  colocateSpec.relatedGroupsList() = {std::move(relatedInfo)};
  colocateSpec.colocationScopeName() = "region";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().colocateGroupsMoveTypeSpec() = colocateSpec;

  solver.addSolver(solverSpec);

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

// replica_colocation_start
void replicaColocation() {
  // Ensure all replicas of a shard are in the same region
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("server");

  // Define replica groups
  std::unordered_map<std::string, std::vector<std::string>> replicaTypes = {
      {"primary", {"shard1_primary", "shard2_primary"}},
      {"replica_1", {"shard1_replica1", "shard2_replica1"}},
      {"replica_2", {"shard1_replica2", "shard2_replica2"}},
  };
  solver.addPartition("replica_type", std::move(replicaTypes));

  LocalSearchSolverSpec solverSpec;

  ColocateGroupsMoveTypeRelatedGroupsInfo relatedInfo;
  relatedInfo.relatedGroups() = {"primary", "replica_1", "replica_2"};

  ColocateGroupsMoveTypeSpec colocateSpec;
  colocateSpec.partitionName() = "replica_type";
  colocateSpec.relatedGroupsList() = {std::move(relatedInfo)};
  colocateSpec.colocationScopeName() = "region";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().colocateGroupsMoveTypeSpec() = colocateSpec;

  solver.addSolver(solverSpec);

  // Setup with misaligned replicas
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"server1", {"shard1_primary"}},
          {"server2", {"shard2_replica1"}},
          {"server3", {"shard1_replica1", "shard2_primary"}},
          {"server4", {"shard1_replica2", "shard2_replica2"}},
      });

  solver.addScope(
      "region",
      std::map<std::string, std::string>{
          {"server1", "region_a"},
          {"server2", "region_a"},
          {"server3", "region_b"},
          {"server4", "region_b"},
      });

  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"shard1_primary", 2.0},
          {"shard1_replica1", 2.0},
          {"shard1_replica2", 2.0},
          {"shard2_primary", 2.0},
          {"shard2_replica1", 2.0},
          {"shard2_replica2", 2.0},
      });

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-memory";
  balanceSpec.scope() = "server";
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
// replica_colocation_end

// sampling_start
void sampling() {
  // Use sampling to limit complexity
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> replicaGroups = {
      {"primary", {"obj1"}},
      {"replica1", {"obj2"}},
      {"replica2", {"obj3"}},
  };
  solver.addPartition("replica_group", std::move(replicaGroups));

  LocalSearchSolverSpec solverSpec;

  ColocateGroupsMoveTypeRelatedGroupsInfo relatedInfo;
  relatedInfo.relatedGroups() = {"primary", "replica1", "replica2"};

  ColocateGroupsMoveTypeSpec colocateSpec;
  colocateSpec.partitionName() = "replica_group";
  colocateSpec.relatedGroupsList() = {std::move(relatedInfo)};
  colocateSpec.colocationScopeName() = "region";
  colocateSpec.defaultSampleSize() = 5; // Sample 5 containers per group

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().colocateGroupsMoveTypeSpec() = colocateSpec;

  solver.addSolver(solverSpec);
}
// sampling_end

// restricted_start
void restricted() {
  // Restrict which containers each group can use
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> replicaGroups = {
      {"primary", {"obj1"}},
      {"replica", {"obj2"}},
  };
  solver.addPartition("replica_group", std::move(replicaGroups));

  LocalSearchSolverSpec solverSpec;

  ColocateGroupsMoveTypeRelatedGroupsInfo relatedInfo;
  relatedInfo.relatedGroups() = {"primary", "replica"};

  ColocateGroupsMoveTypeSpec colocateSpec;
  colocateSpec.partitionName() = "replica_group";
  colocateSpec.relatedGroupsList() = {std::move(relatedInfo)};
  colocateSpec.colocationScopeName() = "region";

  // Restrict valid containers per group per region
  StringToStringSetMap region1Map;
  region1Map["primary"] = {"server1", "server2"};
  region1Map["replica"] = {"server3", "server4"};

  StringToStringSetMap region2Map;
  region2Map["primary"] = {"server5", "server6"};
  region2Map["replica"] = {"server7", "server8"};

  colocateSpec.colocationScopeItemToGroupToContainers() = {
      {"region1", region1Map},
      {"region2", region2Map},
  };

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().colocateGroupsMoveTypeSpec() = colocateSpec;

  solver.addSolver(solverSpec);
}
// restricted_end

// multiple_sets_start
void multipleSets() {
  // Multiple independent related group sets
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");

  std::unordered_map<std::string, std::vector<std::string>> serviceTypes = {
      {"web", {"web1"}},
      {"web_cache", {"cache1"}},
      {"db", {"db1"}},
      {"db_replica", {"db2"}},
  };
  solver.addPartition("service_type", std::move(serviceTypes));

  LocalSearchSolverSpec solverSpec;

  ColocateGroupsMoveTypeRelatedGroupsInfo webTier;
  webTier.relatedGroups() = {"web", "web_cache"}; // Web tier together

  ColocateGroupsMoveTypeRelatedGroupsInfo dbTier;
  dbTier.relatedGroups() = {"db", "db_replica"}; // DB tier together

  ColocateGroupsMoveTypeSpec colocateSpec;
  colocateSpec.partitionName() = "service_type";
  colocateSpec.relatedGroupsList() = {std::move(webTier), std::move(dbTier)};
  colocateSpec.colocationScopeName() = "datacenter";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().colocateGroupsMoveTypeSpec() = colocateSpec;

  solver.addSolver(solverSpec);
}
// multiple_sets_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "=== Quick example showing basic ColocateGroups usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
