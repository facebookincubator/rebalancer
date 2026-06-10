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
 * ColocateGroupsSpec Reference Examples

This file demonstrates all the usage patterns for ColocateGroupsSpec shown in
the reference documentation. Each function is a complete, runnable example.
 *
 * This file demonstrates all the usage patterns for ColocateGroupsSpec shown in
the
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
   * Quick example showing basic ColocateGroupsSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
          {"host2", {"task4", "task5"}},
      });
  solver.addPartition(
      "tenant",
      std::map<std::string, std::vector<std::string>>{
          {"tenant_a", {"task0", "task1", "task2"}},
          {"tenant_b", {"task3", "task4", "task5"}},
      });

  // quick_example_start
  // Colocate each tenant's objects on at most 3 hosts
  ColocateGroupsSpec spec;
  spec.name() = "colocate-tenants";
  spec.scope() = "host";
  spec.partitionName() = "tenant";
  spec.bound() = ColocateGroupsSpecBound::MAX;

  Limit limits;
  limits.type() = LimitType::ABSOLUTE;
  limits.globalLimit() = 3;
  spec.limits() = limits;

  solver.addConstraint(spec);
  // quick_example_end
}

void colocate_related_objects() {
  /**
   * Keep tenant's objects together on few hosts.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("obj");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"obj_1", "obj_2", "obj_3"}},
          {"host1", {"obj_4", "obj_5"}},
          {"host2", {"obj_6", "obj_7"}},
      });

  // colocate_related_objects_start
  // Define tenant partition
  std::map<std::string, std::vector<std::string>> tenants = {
      {"tenant_a", {"obj_1", "obj_2", "obj_3", "obj_4"}},
      {"tenant_b", {"obj_5", "obj_6", "obj_7"}},
  };
  solver.addPartition("tenant", std::move(tenants));

  // Each tenant on at most 2 hosts
  ColocateGroupsSpec spec;
  spec.name() = "colocate-tenants";
  spec.scope() = "host";
  spec.partitionName() = "tenant";
  spec.bound() = ColocateGroupsSpecBound::MAX;

  Limit limits;
  limits.type() = LimitType::ABSOLUTE;
  limits.globalLimit() = 2;
  spec.limits() = std::move(limits);

  solver.addConstraint(std::move(spec));
  // colocate_related_objects_end
}

void spread_for_fault_tolerance() {
  /**
   * Ensure groups spread across sufficient racks.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
          {"host2", {"task4", "task5"}},
          {"host3", {"task6", "task7"}},
          {"host4", {"task8", "task9"}},
          {"host5", {"task10", "task11"}},
      });
  solver.addScope(
      "rack",
      std::map<std::string, std::string>{
          {"host0", "rack0"},
          {"host1", "rack0"},
          {"host2", "rack1"},
          {"host3", "rack1"},
          {"host4", "rack2"},
          {"host5", "rack2"},
      });
  solver.addPartition(
      "replica",
      std::map<std::string, std::vector<std::string>>{
          {"replica_set_a",
           {"task0", "task1", "task2", "task3", "task4", "task5"}},
          {"replica_set_b",
           {"task6", "task7", "task8", "task9", "task10", "task11"}},
      });

  // spread_for_fault_tolerance_start
  // Each replica group on at least 3 racks
  ColocateGroupsSpec spec;
  spec.name() = "spread-replicas";
  spec.scope() = "rack";
  spec.partitionName() = "replica";
  spec.bound() = ColocateGroupsSpecBound::MIN;

  Limit limits;
  limits.type() = LimitType::ABSOLUTE;
  limits.globalLimit() = 3;
  spec.limits() = std::move(limits);

  solver.addConstraint(std::move(spec));
  // spread_for_fault_tolerance_end
}

void prefer_colocation() {
  /**
   * Soft preference for colocation.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
          {"host2", {"task4", "task5"}},
      });
  solver.addPartition(
      "tenant",
      std::map<std::string, std::vector<std::string>>{
          {"tenant_a", {"task0", "task1", "task2"}},
          {"tenant_b", {"task3", "task4", "task5"}},
      });

  // prefer_colocation_start
  // Try to colocate, but don't fail if can't
  ColocateGroupsSpec spec;
  spec.name() = "prefer-colocation";
  spec.scope() = "host";
  spec.partitionName() = "tenant";
  spec.bound() = ColocateGroupsSpecBound::MAX;

  Limit limits;
  limits.type() = LimitType::ABSOLUTE;
  limits.globalLimit() = 1;
  spec.limits() = std::move(limits);

  spec.squares() = true; // Stronger penalty for spread

  solver.addGoal(std::move(spec), 0.5);
  // prefer_colocation_end
}

void weighted_scope_items() {
  /**
   * Prefer colocation on certain scope items.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"high_capacity_host_1", {"task0", "task1"}},
          {"high_capacity_host_2", {"task2", "task3"}},
          {"low_capacity_host_3", {"task4", "task5"}},
      });
  solver.addPartition(
      "tenant",
      std::map<std::string, std::vector<std::string>>{
          {"tenant_a", {"task0", "task1", "task2"}},
          {"tenant_b", {"task3", "task4", "task5"}},
      });

  // weighted_scope_items_start
  // Prefer spreading to high-capacity hosts (lower weight = better for
  // colocation)
  ColocateGroupsSpec spec;
  spec.name() = "weighted-colocation";
  spec.scope() = "host";
  spec.partitionName() = "tenant";
  spec.bound() = ColocateGroupsSpecBound::MAX;

  Limit limits;
  limits.type() = LimitType::ABSOLUTE;
  limits.globalLimit() = 5;
  spec.limits() = std::move(limits);

  spec.scopeItemWeights() = {
      {"high_capacity_host_1", 0.5}, // Prefer these
      {"high_capacity_host_2", 0.5},
      {"low_capacity_host_3", 1.0}, // Default
  };

  solver.addConstraint(std::move(spec));
  // weighted_scope_items_end
}

void relative_to_group_size() {
  /**
   * Limit spread proportionally.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
          {"host2", {"task4", "task5"}},
      });
  solver.addPartition(
      "shard",
      std::map<std::string, std::vector<std::string>>{
          {"shard_a", {"task0", "task1", "task2"}},
          {"shard_b", {"task3", "task4", "task5"}},
      });

  // relative_to_group_size_start
  // Large groups can spread more, small groups must colocate
  ColocateGroupsSpec spec;
  spec.name() = "proportional-spread";
  spec.scope() = "host";
  spec.partitionName() = "shard";
  spec.bound() = ColocateGroupsSpecBound::MAX;

  Limit limits;
  limits.type() = LimitType::ABSOLUTE;
  limits.globalLimit() = 2; // Each group on at most 2 scope items
  spec.limits() = std::move(limits);

  solver.addConstraint(std::move(spec));
  // relative_to_group_size_end
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "2. Colocate Related Objects...\n";
  colocate_related_objects();

  std::cout << "3. Spread For Fault Tolerance...\n";
  spread_for_fault_tolerance();

  std::cout << "4. Prefer Colocation...\n";
  prefer_colocation();

  std::cout << "5. Weighted Scope Items...\n";
  weighted_scope_items();

  std::cout << "6. Relative To Group Size...\n";
  relative_to_group_size();

  std::cout << "\n✓ All examples completed successfully!\n";
  return 0;
}
// example_end
