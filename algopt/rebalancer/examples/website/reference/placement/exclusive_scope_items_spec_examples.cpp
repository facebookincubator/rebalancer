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
 * ExclusiveScopeItemsSpec Reference Examples
 *
 * This file demonstrates all usage patterns shown in the reference
 * documentation. Each function is a complete, runnable example.
 */

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
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"shard0"}},
          {"rack2", {"shard1"}},
      });
  solver.addObjectDimension(
      "storage",
      std::map<std::string, double>{{"shard0", 1.0}, {"shard1", 1.0}});

  // quick_example_start
  // Rack1 and Rack2 are mutually exclusive
  ExclusiveScopeItemsSpec spec;
  spec.name() = "rack-exclusivity";
  spec.scope() = "rack";
  spec.dimension() = "storage";

  ConflictingScopeItemInfo conflict;
  conflict.conflictingScopeItem() = "rack2";
  conflict.overlap() = 1.0;

  ScopeItemConflictInfo info;
  info.scopeItem() = "rack1";
  info.conflictingScopeItemsWithOverlap() = {conflict};

  spec.conflictInfoList() = {info};

  solver.addConstraint(spec);
  // quick_example_end
}

void rack_anti_affinity() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("rack");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"rack1", {"shard0"}},
          {"rack2", {"shard1"}},
          {"rack3", {"shard2"}},
          {"rack4", {"shard3"}},
      });
  solver.addObjectDimension(
      "storage",
      std::map<std::string, double>{
          {"shard0", 1.0}, {"shard1", 1.0}, {"shard2", 1.0}, {"shard3", 1.0}});
  solver.addPartition(
      "database",
      std::map<std::string, std::vector<std::string>>{
          {"db_a", {"shard0", "shard1"}},
          {"db_b", {"shard2", "shard3"}},
      });

  // rack_anti_affinity_start
  // Replicas of same database cannot use racks 1 and 2 together
  ExclusiveScopeItemsSpec spec;
  spec.name() = "replica-rack-anti-affinity";
  spec.scope() = "rack";
  spec.dimension() = "storage";
  spec.partitionName() = "database";

  ConflictingScopeItemInfo conflict1;
  conflict1.conflictingScopeItem() = "rack2";

  ScopeItemConflictInfo info1;
  info1.scopeItem() = "rack1";
  info1.conflictingScopeItemsWithOverlap() = {std::move(conflict1)};

  ConflictingScopeItemInfo conflict2;
  conflict2.conflictingScopeItem() = "rack4";

  ScopeItemConflictInfo info2;
  info2.scopeItem() = "rack3";
  info2.conflictingScopeItemsWithOverlap() = {std::move(conflict2)};

  spec.conflictInfoList() = {std::move(info1), std::move(info2)};

  solver.addConstraint(std::move(spec));
  // rack_anti_affinity_end
}

void datacenter_exclusivity() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("datacenter");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"dc-west", {"task0"}},
          {"dc-east", {"task1"}},
      });
  solver.addObjectDimension(
      "cpu", std::map<std::string, double>{{"task0", 1.0}, {"task1", 1.0}});
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"svc_a", {"task0", "task1"}},
      });

  // datacenter_exclusivity_start
  // DC-West and DC-East cannot both be used by same service
  ExclusiveScopeItemsSpec spec;
  spec.name() = "datacenter-exclusivity";
  spec.scope() = "datacenter";
  spec.dimension() = "cpu";
  spec.partitionName() = "service";

  ConflictingScopeItemInfo conflict;
  conflict.conflictingScopeItem() = "dc-east";

  ScopeItemConflictInfo info;
  info.scopeItem() = "dc-west";
  info.conflictingScopeItemsWithOverlap() = {std::move(conflict)};

  spec.conflictInfoList() = {std::move(info)};

  solver.addConstraint(std::move(spec));
  // datacenter_exclusivity_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all ExclusiveScopeItems examples...\n";

  std::cout << "\n1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Rack Anti Affinity...\n";
  rack_anti_affinity();

  std::cout << "\n3. Datacenter Exclusivity...\n";
  datacenter_exclusivity();

  std::cout << "\nAll ExclusiveScopeItems examples completed successfully!\n";
  return 0;
}
