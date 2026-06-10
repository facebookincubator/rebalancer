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
 * ReplicaDrop Move Type Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
 * */

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

// quick_example_start
void quickExample() {
  // Reduce job from 4 tasks to 2, dropping worst tasks
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  // Setup assignment so tasks exist before referencing them in partition
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host1", {"task0", "task1"}},
          {"host2", {"task2"}},
          {"host3", {"task3"}},
      });

  // Define jobs with tasks
  std::unordered_map<std::string, std::vector<std::string>> jobs = {
      {"job0", {"task0", "task1", "task2", "task3"}},
  };
  solver.addPartition("job", jobs);

  // Define assigned scope
  std::map<std::string, std::string> assignedScope = {
      {"host1", "assigned"},
      {"host2", "assigned"},
      {"host3", "assigned"},
  };
  solver.addScope("assigned", assignedScope);

  LocalSearchSolverSpec solverSpec;

  ReplicaDropMoveTypeSpec replicaDropSpec;
  replicaDropSpec.replicaDropPartition() = "job";
  replicaDropSpec.replicaDropScope() = "assigned";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().replicaDropMoveTypeSpec() = replicaDropSpec;

  solver.addSolver(solverSpec);
}
// quick_example_end

void quickExampleComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  // Define jobs with tasks
  std::unordered_map<std::string, std::vector<std::string>> jobs = {
      {"job0", {"task0", "task1", "task2", "task3"}},
  };
  solver.addPartition("job", std::move(jobs));

  // Define assigned scope
  std::map<std::string, std::string> assignedScope = {
      {"host1", "assigned"},
      {"host2", "assigned"},
      {"host3", "assigned"},
  };
  solver.addScope("assigned", assignedScope);

  // Setup over-replicated job
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host1", {"task0", "task1"}},
          {"host2", {"task2"}},
          {"host3", {"task3"}},
      });

  // Add quality dimension (lower is better)
  std::map<std::string, double> lagDim = {
      {"task0", 0.0}, // Best
      {"task1", 0.01},
      {"task2", 0.05}, // Worst - should be dropped
      {"task3", 0.03}, // Second worst - should be dropped
  };
  solver.addObjectDimension("lag", std::move(lagDim));

  std::map<std::string, double> cpuDim = {
      {"task0", 1.0},
      {"task1", 1.0},
      {"task2", 1.0},
      {"task3", 1.0},
  };
  solver.addObjectDimension("cpu", std::move(cpuDim));

  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "assigned";
  groupCountSpec.dimension() = "task_count";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.bound() = GroupCountSpecBound::EXACT;
  groupCountSpec.limit()->globalLimit() = 2;
  solver.addConstraint(std::move(groupCountSpec));

  // Minimize total lag (drops high-lag tasks)
  MinimizeSquaresSpec minimizeLagSpec;
  minimizeLagSpec.name() = "minimize-lag";
  minimizeLagSpec.scope() = "assigned";
  minimizeLagSpec.dimension() = "lag";
  solver.addGoal(std::move(minimizeLagSpec), 1.0);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-cpu";
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver.addGoal(std::move(balanceSpec), 0.1);

  LocalSearchSolverSpec solverSpec;
  ReplicaDropMoveTypeSpec replicaDropSpec;
  replicaDropSpec.replicaDropPartition() = "job";
  replicaDropSpec.replicaDropScope() = "assigned";
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().replicaDropMoveTypeSpec() = replicaDropSpec;
  solver.addSolver(solverSpec);

  auto solution = solver.solve();
  std::cout << "  Initial objective: " << *solution.initialObjective()->value()
            << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}

// job_downsizing_start
void jobDownsizing() {
  // Downsize jobs to specific task counts, dropping highest-lag tasks
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  // Define jobs
  std::unordered_map<std::string, std::vector<std::string>> jobs = {
      {"job0", {"job0_task0", "job0_task1", "job0_task2", "job0_task3"}},
      {"job1", {"job1_task0", "job1_task1"}},
  };
  solver.addPartition("job", std::move(jobs));

  // Define assigned scope
  std::map<std::string, std::string> assignedScope = {
      {"host1", "assigned"},
      {"host2", "assigned"},
      {"host3", "assigned"},
      {"host4", "assigned"},
  };
  solver.addScope("assigned", assignedScope);

  // Add lag dimension to distinguish task quality
  std::map<std::string, double> lagDimension = {
      {"job0_task0", 0.0},
      {"job0_task1", 0.01},
      {"job0_task2", 0.03},
      {"job0_task3", 0.02},
      {"job1_task0", 0.06},
      {"job1_task1", 0.0},
  };
  solver.addObjectDimension("lag", std::move(lagDimension));

  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "assigned";
  groupCountSpec.dimension() = "task_count";
  groupCountSpec.partitionName() = "job";
  groupCountSpec.bound() = GroupCountSpecBound::EXACT;
  groupCountSpec.limit()->groupLimits() = {{"job0", 2}, {"job1", 1}};
  solver.addConstraint(std::move(groupCountSpec));

  LocalSearchSolverSpec solverSpec;

  ReplicaDropMoveTypeSpec replicaDropSpec;
  replicaDropSpec.replicaDropPartition() = "job";
  replicaDropSpec.replicaDropScope() = "assigned";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().replicaDropMoveTypeSpec() = replicaDropSpec;

  solver.addSolver(solverSpec);
}
// job_downsizing_end

// shard_reduction_start
void shardReduction() {
  // Reduce from 5 replicas to 3 replicas per shard
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("replica");
  solver.setContainerName("node");

  // Define shards with 5 replicas each
  std::unordered_map<std::string, std::vector<std::string>> shards = {
      {"shard1",
       {"shard1_r1", "shard1_r2", "shard1_r3", "shard1_r4", "shard1_r5"}},
      {"shard2",
       {"shard2_r1", "shard2_r2", "shard2_r3", "shard2_r4", "shard2_r5"}},
  };
  solver.addPartition("shard", std::move(shards));

  // Define active scope
  std::map<std::string, std::string> activeScope = {
      {"node1", "active"},
      {"node2", "active"},
      {"node3", "active"},
      {"node4", "active"},
      {"node5", "active"},
  };
  solver.addScope("active", activeScope);

  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "active";
  groupCountSpec.dimension() = "replica_count";
  groupCountSpec.partitionName() = "shard";
  groupCountSpec.bound() = GroupCountSpecBound::EXACT;
  groupCountSpec.limit()->globalLimit() = 3; // 3 replicas per shard

  solver.addConstraint(std::move(groupCountSpec));

  LocalSearchSolverSpec solverSpec;

  ReplicaDropMoveTypeSpec replicaDropSpec;
  replicaDropSpec.replicaDropPartition() = "shard";
  replicaDropSpec.replicaDropScope() = "active";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().replicaDropMoveTypeSpec() = replicaDropSpec;

  solver.addSolver(solverSpec);
}
// shard_reduction_end

void shardReductionComplete() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("replica");
  solver.setContainerName("node");

  // Define shards with 5 replicas each
  std::unordered_map<std::string, std::vector<std::string>> shards = {
      {"shard1",
       {"shard1_r1", "shard1_r2", "shard1_r3", "shard1_r4", "shard1_r5"}},
      {"shard2",
       {"shard2_r1", "shard2_r2", "shard2_r3", "shard2_r4", "shard2_r5"}},
  };
  solver.addPartition("shard", std::move(shards));

  // Define active scope
  std::map<std::string, std::string> activeScope = {
      {"node1", "active"},
      {"node2", "active"},
      {"node3", "active"},
      {"node4", "active"},
      {"node5", "active"},
  };
  solver.addScope("active", activeScope);

  // Setup over-replicated shards
  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"node1", {"shard1_r1", "shard2_r1"}},
          {"node2", {"shard1_r2", "shard2_r2"}},
          {"node3", {"shard1_r3", "shard2_r3"}},
          {"node4", {"shard1_r4", "shard2_r4"}},
          {"node5", {"shard1_r5", "shard2_r5"}},
      });

  // Add quality dimension (staleness - lower is better)
  std::map<std::string, double> stalenessDim = {
      {"shard1_r1", 0.0},
      {"shard1_r2", 0.0},
      {"shard1_r3", 0.0},
      {"shard1_r4", 10.0}, // Stale - should be dropped
      {"shard1_r5", 15.0}, // Very stale - should be dropped
      {"shard2_r1", 0.0},
      {"shard2_r2", 0.0},
      {"shard2_r3", 0.0},
      {"shard2_r4", 8.0}, // Stale - should be dropped
      {"shard2_r5", 12.0}, // Very stale - should be dropped
  };
  solver.addObjectDimension("staleness", std::move(stalenessDim));

  std::map<std::string, double> sizeDim = {
      {"shard1_r1", 1.0},
      {"shard1_r2", 1.0},
      {"shard1_r3", 1.0},
      {"shard1_r4", 1.0},
      {"shard1_r5", 1.0},
      {"shard2_r1", 1.0},
      {"shard2_r2", 1.0},
      {"shard2_r3", 1.0},
      {"shard2_r4", 1.0},
      {"shard2_r5", 1.0},
  };
  solver.addObjectDimension("size", std::move(sizeDim));

  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "active";
  groupCountSpec.dimension() = "replica_count";
  groupCountSpec.partitionName() = "shard";
  groupCountSpec.bound() = GroupCountSpecBound::EXACT;
  groupCountSpec.limit()->globalLimit() = 3;
  solver.addConstraint(std::move(groupCountSpec));

  // Minimize staleness (keeps fresh replicas)
  MinimizeSquaresSpec minimizeStalenessSpec;
  minimizeStalenessSpec.name() = "minimize-staleness";
  minimizeStalenessSpec.scope() = "active";
  minimizeStalenessSpec.dimension() = "staleness";
  solver.addGoal(std::move(minimizeStalenessSpec), 1.0);

  BalanceSpec balanceSpec;
  balanceSpec.name() = "balance-size";
  balanceSpec.scope() = "node";
  balanceSpec.dimension() = "size";
  solver.addGoal(std::move(balanceSpec), 0.1);

  LocalSearchSolverSpec solverSpec;
  ReplicaDropMoveTypeSpec replicaDropSpec;
  replicaDropSpec.replicaDropPartition() = "shard";
  replicaDropSpec.replicaDropScope() = "active";
  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().replicaDropMoveTypeSpec() = replicaDropSpec;
  solver.addSolver(solverSpec);

  auto solution = solver.solve();
  std::cout << "  Initial objective: " << *solution.initialObjective()->value()
            << "\n";
  std::cout << "  Final objective: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "  Improvement: "
            << (*solution.initialObjective()->value() -
                *solution.finalObjective()->value())
            << "\n";
}

// quality_based_start
void qualityBased() {
  // Keep 2 freshest replicas, drop stale ones
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("replica");
  solver.setContainerName("server");

  // Define replicas
  std::unordered_map<std::string, std::vector<std::string>> data = {
      {"dataset1", {"replica1", "replica2", "replica3", "replica4"}},
  };
  solver.addPartition("data", std::move(data));

  // Define online scope
  std::map<std::string, std::string> onlineScope = {
      {"server1", "online"},
      {"server2", "online"},
      {"server3", "online"},
  };
  solver.addScope("online", onlineScope);

  // Add staleness dimension (lower is better)
  std::map<std::string, double> stalenessDimension = {
      {"replica1", 0.0}, // Fresh
      {"replica2", 5.0}, // Slightly stale
      {"replica3", 20.0}, // Very stale
      {"replica4", 2.0}, // Mostly fresh
  };
  solver.addObjectDimension("staleness", std::move(stalenessDimension));

  GroupCountSpec groupCountSpec;
  groupCountSpec.scope() = "online";
  groupCountSpec.dimension() = "replica_count";
  groupCountSpec.partitionName() = "data";
  groupCountSpec.bound() = GroupCountSpecBound::EXACT;
  groupCountSpec.limit()->globalLimit() = 2;

  solver.addConstraint(std::move(groupCountSpec));

  LocalSearchSolverSpec solverSpec;

  ReplicaDropMoveTypeSpec replicaDropSpec;
  replicaDropSpec.replicaDropPartition() = "data";
  replicaDropSpec.replicaDropScope() = "online";

  solverSpec.moveTypeList()->emplace_back();
  solverSpec.moveTypeList()->back().replicaDropMoveTypeSpec() = replicaDropSpec;

  solver.addSolver(solverSpec);
}
// quality_based_end

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "=== Quick example showing basic ReplicaDrop usage ===\n\n";

  std::cout << "Running quick_example...\n";
  quickExample();
  std::cout << "✓ quick_example completed\n\n";

  return 0;
}
