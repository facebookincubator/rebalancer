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
 * GroupCountSpec Reference Examples

This file demonstrates all the usage patterns for GroupCountSpec shown in the
reference documentation. Each function is a complete, runnable example.
 *
 * This file demonstrates all the usage patterns for GroupCountSpec shown in the
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
   * Quick example showing basic GroupCountSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0", "shard1"}},
          {"host1", {"shard2", "shard3"}},
          {"host2", {"shard4", "shard5"}},
          {"host3", {"shard6", "shard7"}},
      });
  solver.addScope(
      "rack",
      std::map<std::string, std::string>{
          {"host0", "rack0"},
          {"host1", "rack0"},
          {"host2", "rack1"},
          {"host3", "rack1"},
      });
  solver.addPartition(
      "replica",
      std::map<std::string, std::vector<std::string>>{
          {"db_a", {"shard0", "shard1", "shard2", "shard3"}},
          {"db_b", {"shard4", "shard5", "shard6", "shard7"}},
      });

  // quick_example_start
  // Max 1 replica of each database per rack (diversity)
  GroupCountSpec spec;
  spec.name() = "one-replica-per-rack";
  spec.scope() = "rack";
  spec.partitionName() = "replica";
  spec.bound() = GroupCountSpecBound::MAX;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1;
  spec.limit() = limit;

  spec.definition() = GroupCountSpecDefinition::AFTER;

  solver.addConstraint(spec);
  // quick_example_end
}

void rack_diversity() {
  /**
   * Ensure no two replicas of the same object on the same rack.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0", "shard1"}},
          {"host1", {"shard2", "shard3"}},
          {"host2", {"shard4", "shard5"}},
          {"host3", {"shard6", "shard7"}},
      });
  solver.addScope(
      "rack",
      std::map<std::string, std::string>{
          {"host0", "rack0"},
          {"host1", "rack0"},
          {"host2", "rack1"},
          {"host3", "rack1"},
      });

  // rack_diversity_start
  // Define replica partition
  const std::map<std::string, std::vector<std::string>> replica_to_objects = {
      {"db_a", {"shard0", "shard1", "shard2", "shard3"}},
      {"db_b", {"shard4", "shard5", "shard6", "shard7"}},
  };
  solver.addPartition("replica", replica_to_objects);

  // Max 1 replica per rack
  GroupCountSpec spec;
  spec.name() = "rack-diversity";
  spec.scope() = "rack";
  spec.partitionName() = "replica";
  spec.bound() = GroupCountSpecBound::MAX;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1;
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // rack_diversity_end
}

void minimum_presence() {
  /**
   * Ensure minimum number of groups on each container.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"shard0", "shard1"}},
          {"host1", {"shard2", "shard3"}},
          {"host2", {"shard4", "shard5"}},
          {"host3", {"shard6", "shard7"}},
          {"host4", {"shard8", "shard9"}},
          {"host5", {"shard10", "shard11"}},
      });
  solver.addScope(
      "datacenter",
      std::map<std::string, std::string>{
          {"host0", "dc1"},
          {"host1", "dc1"},
          {"host2", "dc2"},
          {"host3", "dc2"},
          {"host4", "dc3"},
          {"host5", "dc3"},
      });
  solver.addPartition(
      "replica",
      std::map<std::string, std::vector<std::string>>{
          {"db_a",
           {"shard0", "shard1", "shard2", "shard3", "shard4", "shard5"}},
          {"db_b",
           {"shard6", "shard7", "shard8", "shard9", "shard10", "shard11"}},
      });

  // minimum_presence_start
  // Each datacenter must have at least 3 replicas
  GroupCountSpec spec;
  spec.name() = "min-replicas-per-dc";
  spec.scope() = "datacenter";
  spec.partitionName() = "replica";
  spec.bound() = GroupCountSpecBound::MIN;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 3;
  spec.limit() = std::move(limit);

  solver.addConstraint(std::move(spec));
  // minimum_presence_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "2. Rack Diversity...\n";
  rack_diversity();

  std::cout << "3. Minimum Presence...\n";
  minimum_presence();

  std::cout << "\n✓ All examples completed successfully!\n";
  return 0;
}
// example_end
