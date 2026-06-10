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
 * GroupAssignmentAffinitiesSpec Reference Examples
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
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host1", {"task0"}},
          {"host2", {"task1"}},
      });
  solver.addObjectDimension(
      "cpu", std::map<std::string, double>{{"task0", 1.0}, {"task1", 1.0}});
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"service-A", {"task0"}},
          {"service-B", {"task1"}},
      });

  // quick_example_start
  // Prefer service-A on host1
  GroupAssignmentAffinitiesSpec spec;
  spec.name() = "service-placement-preferences";
  spec.scope() = "host";
  spec.partition() = "service";
  spec.dimension() = "cpu";

  GroupScopeItemAffinity affinity;
  affinity.group() = "service-A";
  affinity.scopeItem() = "host1";
  affinity.targetDimensionValue() = 1.0;
  affinity.affinity() = 10.0;

  spec.affinities() = {affinity};

  solver.addGoal(spec, 1.0);
  // quick_example_end
}

void designated_hosts() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"premium-host-1", {"task0"}},
          {"standard-host-1", {"task1"}},
      });
  solver.addObjectDimension(
      "memory", std::map<std::string, double>{{"task0", 1.0}, {"task1", 1.0}});
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"critical-service", {"task0"}},
          {"batch-service", {"task1"}},
      });

  // designated_hosts_start
  // Route different services to specific hosts
  GroupAssignmentAffinitiesSpec spec;
  spec.name() = "service-routing";
  spec.scope() = "host";
  spec.partition() = "service";
  spec.dimension() = "memory";

  GroupScopeItemAffinity critical;
  critical.group() = "critical-service";
  critical.scopeItem() = "premium-host-1";
  critical.targetDimensionValue() = 1.0;
  critical.affinity() = 20.0;

  GroupScopeItemAffinity batch;
  batch.group() = "batch-service";
  batch.scopeItem() = "standard-host-1";
  batch.targetDimensionValue() = 1.0;
  batch.affinity() = 10.0;

  spec.affinities() = {std::move(critical), std::move(batch)};

  solver.addGoal(std::move(spec), 1.0);
  // designated_hosts_end
}

void split_group() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("shard");
  solver.setContainerName("datacenter");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"dc-west", {"shard0"}},
          {"dc-east", {"shard1"}},
          {"dc-central", {"shard2"}},
      });
  solver.addObjectDimension(
      "storage",
      std::map<std::string, double>{
          {"shard0", 1.0}, {"shard1", 1.0}, {"shard2", 1.0}});
  solver.addPartition(
      "database",
      std::map<std::string, std::vector<std::string>>{
          {"user-db", {"shard0", "shard1", "shard2"}},
      });

  // split_group_start
  // Split database across 3 datacenters
  GroupAssignmentAffinitiesSpec spec;
  spec.name() = "database-distribution";
  spec.scope() = "datacenter";
  spec.partition() = "database";
  spec.dimension() = "storage";

  GroupScopeItemAffinity west;
  west.group() = "user-db";
  west.scopeItem() = "dc-west";
  west.targetDimensionValue() = 0.5;
  west.affinity() = 15.0;

  GroupScopeItemAffinity east;
  east.group() = "user-db";
  east.scopeItem() = "dc-east";
  east.targetDimensionValue() = 0.3;
  east.affinity() = 15.0;

  GroupScopeItemAffinity central;
  central.group() = "user-db";
  central.scopeItem() = "dc-central";
  central.targetDimensionValue() = 0.2;
  central.affinity() = 15.0;

  spec.affinities() = {std::move(west), std::move(east), std::move(central)};

  solver.addGoal(std::move(spec), 1.0);
  // split_group_end
}

void priority_placement() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"ssd-host-1", {"task0"}},
          {"ssd-host-2", {"task1"}},
          {"standard-host-1", {"task2"}},
      });
  solver.addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 1.0}, {"task1", 1.0}, {"task2", 1.0}});
  solver.addPartition(
      "service",
      std::map<std::string, std::vector<std::string>>{
          {"p0-service", {"task0"}},
          {"p1-service", {"task1"}},
          {"p2-service", {"task2"}},
      });

  // priority_placement_start
  // Different affinity strengths for priority tiers
  GroupAssignmentAffinitiesSpec spec;
  spec.name() = "priority-placement";
  spec.scope() = "host";
  spec.partition() = "service";
  spec.dimension() = "cpu";

  GroupScopeItemAffinity p0;
  p0.group() = "p0-service";
  p0.scopeItem() = "ssd-host-1";
  p0.targetDimensionValue() = 1.0;
  p0.affinity() = 100.0;

  GroupScopeItemAffinity p1;
  p1.group() = "p1-service";
  p1.scopeItem() = "ssd-host-2";
  p1.targetDimensionValue() = 1.0;
  p1.affinity() = 50.0;

  GroupScopeItemAffinity p2;
  p2.group() = "p2-service";
  p2.scopeItem() = "standard-host-1";
  p2.targetDimensionValue() = 1.0;
  p2.affinity() = 10.0;

  spec.affinities() = {std::move(p0), std::move(p1), std::move(p2)};

  solver.addGoal(std::move(spec), 1.0);
  // priority_placement_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all GroupAssignmentAffinities examples...\n";

  std::cout << "\n1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Designated Hosts...\n";
  designated_hosts();

  std::cout << "\n3. Split Group...\n";
  split_group();

  std::cout << "\n4. Priority Placement...\n";
  priority_placement();

  std::cout
      << "\nAll GroupAssignmentAffinities examples completed successfully!\n";
  return 0;
}
