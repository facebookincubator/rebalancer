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
 * MoveGroupSpec Reference Examples

This file demonstrates all the usage patterns for MoveGroupSpec shown in the
reference documentation. Each function is a complete, runnable example.
 *
 * This file demonstrates all the usage patterns for MoveGroupSpec shown in the
 * reference documentation. Each function is a complete, runnable example.
 */

// example_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void quick_example() {
  /**
   * Quick example showing basic MoveGroupSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("replica");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"replica0", "replica1"}},
          {"host1", {"replica2", "replica3"}},
      });
  solver.addPartition(
      "shard",
      std::map<std::string, std::vector<std::string>>{
          {"shard_a", {"replica0", "replica1"}},
          {"shard_b", {"replica2", "replica3"}},
      });

  // quick_example_start
  // Move all replicas of each shard together
  MoveGroupSpec spec;
  spec.name() = "move-shard-replicas-together";
  spec.partitionName() = "shard"; // Partition grouping replicas by shard

  solver.addConstraint(spec);
  // quick_example_end
}

void replica_set_cohesion() {
  /**
   * Keep database replicas together.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("replica");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard_0_primary", "shard_1_primary", "shard_2_primary"}},
          {"host1",
           {"shard_0_replica_1", "shard_1_replica_1", "shard_2_replica_1"}},
          {"host2",
           {"shard_0_replica_2", "shard_1_replica_2", "shard_2_replica_2"}},
      });

  // replica_set_cohesion_start
  // Partition by replica set
  std::map<std::string, std::vector<std::string>> replica_sets = {
      {"db_shard_0",
       {"shard_0_primary", "shard_0_replica_1", "shard_0_replica_2"}},
      {"db_shard_1",
       {"shard_1_primary", "shard_1_replica_1", "shard_1_replica_2"}},
      {"db_shard_2",
       {"shard_2_primary", "shard_2_replica_1", "shard_2_replica_2"}},
  };
  solver.addPartition("replica_set", std::move(replica_sets));

  // Move replicas together (for migration, not anti-affinity!)
  MoveGroupSpec spec;
  spec.name() = "move-replica-sets-together";
  spec.partitionName() = "replica_set";

  solver.addConstraint(std::move(spec));
  // replica_set_cohesion_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "2. Replica Set Cohesion...\n";
  replica_set_cohesion();

  std::cout << "\n✓ All examples completed successfully!\n";
  return 0;
}
// example_end
