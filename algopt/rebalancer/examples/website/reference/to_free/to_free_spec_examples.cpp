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
 * ToFreeSpec Reference Examples
 *
 * This file demonstrates all the usage patterns for ToFreeSpec shown in the
 * reference documentation. Each function is a complete, runnable example.
 */

// example_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void quick_example() {
  /**
   * Quick example showing basic ToFreeSpec usage.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1"}},
          {"host3", {"task2", "task3"}},
          {"host7", {"task4"}},
          {"host12", {"task5", "task6"}},
      });

  // quick_example_start
  // Drain all objects from hosts under maintenance
  ToFreeSpec spec;
  spec.name() = "drain-maintenance-hosts";
  spec.containers() = std::vector<std::string>{"host3", "host7", "host12"};
  solver.addConstraint(spec);
  // quick_example_end
}

void drain_hosts_for_maintenance() {
  /**
   * Remove all objects from hosts under maintenance.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0"}},
          {"host5", {"task1", "task2"}},
          {"host8", {"task3"}},
          {"host15", {"task4"}},
      });

  // drain_maintenance_start
  // Hosts scheduled for maintenance
  std::vector<std::string> maintenance_hosts = {"host5", "host8", "host15"};

  ToFreeSpec spec;
  spec.name() = "maintenance-drain";
  spec.containers() = std::move(maintenance_hosts);
  solver.addConstraint(std::move(spec));
  // drain_maintenance_end
}

void decommission_old_hardware() {
  /**
   * Empty old hosts before removing from cluster.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"old_host_1", {"task0", "task1"}},
          {"old_host_2", {"task2"}},
          {"old_host_3", {"task3"}},
          {"new_host_1", {}},
      });

  // decommission_old_hardware_start
  // Old generation hosts to retire
  std::vector<std::string> old_hosts = {
      "old_host_1", "old_host_2", "old_host_3"};

  ToFreeSpec spec;
  spec.name() = "decommission-old-hardware";
  spec.containers() = std::move(old_hosts);
  solver.addConstraint(std::move(spec));
  // decommission_old_hardware_end
}

void drain_specific_resource_type() {
  /**
   * Reduce specific dimension to zero (not all objects).
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0"}},
          {"host1", {"task1", "task2"}},
          {"host2", {"task3"}},
      });
  solver.addObjectDimension(
      "storage_gb",
      std::map<std::string, double>{
          {"task0", 50.0}, {"task1", 100.0}, {"task2", 80.0}, {"task3", 60.0}});

  // drain_specific_resource_start
  // Remove all storage from hosts, but keep CPU workloads
  ToFreeSpec spec;
  spec.name() = "drain-storage";
  spec.containers() = std::vector<std::string>{"host1", "host2"};
  spec.dimension() = "storage_gb"; // Only storage, not all objects
  solver.addGoal(std::move(spec), 1.0);
  // drain_specific_resource_end
}

void gradual_draining_as_goal() {
  /**
   * Soft goal to drain, but don't fail if impossible.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {}},
          {"host1", {"task0", "task1"}},
          {"host2", {"task2"}},
          {"host3", {"task3"}},
      });

  std::vector<std::string> overloaded_hosts = {"host1", "host2", "host3"};

  // gradual_draining_start
  // Try to drain, but don't fail if capacity insufficient elsewhere
  ToFreeSpec spec;
  spec.name() = "prefer-empty-hosts";
  spec.containers() = std::move(overloaded_hosts);
  solver.addGoal(std::move(spec), 0.5);
  // gradual_draining_end
}

void drain_prioritization() {
  /**
   * Use formula to control draining strategy.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {}},
          {"host1", {"task0"}},
          {"host2", {"task1"}},
          {"host3", {"task2"}},
          {"host4", {"task3"}},
      });

  std::vector<std::string> candidate_hosts = {
      "host1", "host2", "host3", "host4"};

  // drain_prioritization_start
  // Fully drain SOME hosts (don't spread evenly)
  ToFreeSpec spec;
  spec.name() = "focused-drain";
  spec.containers() = std::move(candidate_hosts);
  solver.addConstraint(std::move(spec));
  // drain_prioritization_end
}

void drain_by_priority() {
  /**
   * Drain high-priority containers first.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {}},
          {"host1", {"task0"}},
          {"host2", {"task1"}},
          {"host3", {"task2"}},
          {"host4", {"task3"}},
          {"host5", {"task4"}},
      });

  std::vector<std::string> critical_maintenance = {"host1", "host2"};
  std::vector<std::string> optional_maintenance = {"host3", "host4", "host5"};

  // drain_by_priority_start
  // Tuple 0: Drain critical maintenance hosts
  ToFreeSpec spec1;
  spec1.name() = "drain-critical";
  spec1.containers() = critical_maintenance;
  solver.addGoal(spec1, 1.0, /* tuplePos= */ 0);

  // Tuple 1: Drain nice-to-have hosts
  ToFreeSpec spec2;
  spec2.name() = "drain-optional";
  spec2.containers() = optional_maintenance;
  solver.addGoal(spec2, 1.0, /* tuplePos= */ 1);
  // drain_by_priority_end
}

void constraint_usage() {
  /**
   * Use ToFreeSpec as a hard constraint.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {}},
          {"host1", {"task0"}},
          {"host2", {"task1"}},
      });

  std::vector<std::string> decommission_hosts = {"host1", "host2"};

  // constraint_usage_start
  // Hard requirement: These hosts MUST be empty
  ToFreeSpec spec;
  spec.name() = "must-drain";
  spec.containers() = std::move(decommission_hosts);
  solver.addConstraint(std::move(spec));
  // constraint_usage_end
}

void goal_usage() {
  /**
   * Use ToFreeSpec as a soft goal.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {}},
          {"host1", {"task0"}},
          {"host2", {"task1"}},
          {"host3", {"task2"}},
      });

  std::vector<std::string> candidate_hosts = {"host1", "host2", "host3"};

  // goal_usage_start
  // Nice to have: Try to drain these hosts
  ToFreeSpec spec;
  spec.name() = "prefer-drain";
  spec.containers() = std::move(candidate_hosts);
  solver.addGoal(std::move(spec), 0.5);
  // goal_usage_end
}

void combining_with_capacity_spec() {
  /**
   * Ensure receiving hosts don't exceed capacity.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {}},
          {"host1", {"task0", "task1"}},
          {"host2", {"task2"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 4.0}, {"task1", 8.0}, {"task2", 6.0}});
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{
          {"host0", 32.0}, {"host1", 32.0}, {"host2", 32.0}});

  std::vector<std::string> maintenance_hosts = {"host1", "host2"};

  // combining_capacity_start
  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "drain-maintenance";
  toFreeSpec.containers() = std::move(maintenance_hosts);
  solver.addConstraint(std::move(toFreeSpec));

  CapacitySpec capacitySpec;
  capacitySpec.name() = "capacity";
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "memory";
  solver.addConstraint(std::move(capacitySpec));
  // combining_capacity_end
}

void combining_with_non_accepting_spec() {
  /**
   * Prevent new placements AND drain existing.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {}},
          {"host1", {"task0", "task1"}},
          {"host2", {"task2"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 4.0}, {"task1", 8.0}, {"task2", 6.0}});
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{
          {"host0", 32.0}, {"host1", 32.0}, {"host2", 32.0}});

  std::vector<std::string> maintenance_hosts = {"host1", "host2"};

  // combining_non_accepting_start
  // Block new assignments
  NonAcceptingSpec nonAcceptingSpec;
  nonAcceptingSpec.name() = "block-maintenance-hosts";
  nonAcceptingSpec.scope() = "host";
  nonAcceptingSpec.items() = maintenance_hosts;
  solver.addConstraint(std::move(nonAcceptingSpec));

  // Drain existing objects
  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "drain-maintenance-hosts";
  toFreeSpec.containers() = std::move(maintenance_hosts);
  solver.addConstraint(std::move(toFreeSpec));
  // combining_non_accepting_end
}

bool verify_drained(
    const AssignmentSolution& solution,
    const std::vector<std::string>& containers_to_drain) {
  /**
   * Verify containers were successfully drained.
   */
  // verification_example_start
  // Build reverse map: container -> objects
  std::map<std::string, std::vector<std::string>> container_to_objects;
  for (const auto& [object, container] : *solution.assignment()) {
    container_to_objects[container].push_back(object);
  }

  bool all_drained = true;
  for (const auto& container : containers_to_drain) {
    const auto& objects = container_to_objects[container];
    if (!objects.empty()) {
      std::cout << "X " << container << " still has " << objects.size()
                << " objects: ";
      for (const auto& obj : objects) {
        std::cout << obj << " ";
      }
      std::cout << "\n";
      all_drained = false;
    } else {
      std::cout << "✓ " << container << " successfully drained\n";
    }
  }
  return all_drained;
  // verification_example_end
}

void verification_example() {
  /**
   * Verify draining succeeded.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {}},
          {"host1", {"task0", "task1"}},
          {"host2", {"task2"}},
      });
  solver.addObjectDimension(
      "memory",
      std::map<std::string, double>{
          {"task0", 4.0}, {"task1", 8.0}, {"task2", 6.0}});
  solver.addContainerDimension(
      "memory_capacity",
      std::map<std::string, double>{
          {"host0", 32.0}, {"host1", 32.0}, {"host2", 32.0}});

  std::vector<std::string> maintenance_hosts = {"host1", "host2"};

  // Add drain constraint
  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "drain-maintenance";
  toFreeSpec.containers() = maintenance_hosts;
  solver.addConstraint(std::move(toFreeSpec));

  // Add solver
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 10;
  solver.addSolver(localSearch);

  // verification_usage_start
  // After solving
  auto solution = solver.solve();
  if (verify_drained(solution, maintenance_hosts)) {
    std::cout << "All maintenance hosts successfully drained!\n";
  } else {
    std::cout << "Warning: Some hosts not fully drained\n";
  }
  // verification_usage_end
}

void incremental_drain(
    const std::vector<std::string>& hosts_to_drain,
    int rounds = 3) {
  /**
   * Drain hosts over multiple rounds.
   */
  // incremental_drain_start
  int hosts_per_round = hosts_to_drain.size() / rounds;

  for (const auto round_num : folly::irange(rounds)) {
    // Drain subset this round
    int start = round_num * hosts_per_round;
    int end = (round_num + 1) * hosts_per_round;
    std::vector<std::string> drain_this_round(
        hosts_to_drain.begin() + start, hosts_to_drain.begin() + end);

    auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
    ProblemSolver solver(executor, "example", "test");
    solver.setObjectName("task");
    solver.setContainerName("host");

    // Create assignment with hosts to drain plus a receiving host
    std::map<std::string, std::vector<std::string>> assignment;
    assignment["host0"] = {}; // receiving host
    for (const auto& h : drain_this_round) {
      assignment[h] = {}; // hosts to drain
    }
    solver.setAssignment(assignment);

    ToFreeSpec spec;
    spec.name() = fmt::format("drain-round-{}", round_num);
    spec.containers() = drain_this_round;
    solver.addConstraint(spec);

    // auto solution = solver.solve();
    // apply_moves(solution);

    std::cout << "Round " << (round_num + 1) << ": Drained "
              << drain_this_round.size() << " hosts\n";
  }
  // incremental_drain_end
}

void pitfall_wrong_dimension() {
  /**
   * Common pitfall: Draining wrong dimension.
   */
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(1);
  ProblemSolver solver(executor, "example", "test");
  solver.setObjectName("task");
  solver.setContainerName("host");

  solver.setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {}},
          {"host1", {"task0", "task1"}},
          {"host2", {"task2"}},
      });
  solver.addObjectDimension(
      "storage_gb",
      std::map<std::string, double>{
          {"task0", 50.0}, {"task1", 100.0}, {"task2", 80.0}});

  std::vector<std::string> storage_hosts = {"host1", "host2"};

  // pitfall_wrong_dimension_good_start
  // GOOD: Only drain storage dimension
  ToFreeSpec spec;
  spec.containers() = std::move(storage_hosts);
  spec.dimension() = "storage_gb";
  solver.addConstraint(std::move(spec));
  // pitfall_wrong_dimension_good_end
}

AssignmentSolution complete_example() {
  /**
   * Complete runnable example demonstrating ToFreeSpec.
   *
   * This example shows how to use ToFreeSpec to drain hosts under maintenance
   * while ensuring capacity constraints are respected on receiving hosts.
   */
  // Create solver
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "tofree-example", "production");

  solver.setObjectName("task");
  solver.setContainerName("host");

  // Initial assignment with tasks on maintenance hosts
  std::map<std::string, std::vector<std::string>> assignment = {
      {"host0", {"task0", "task1"}},
      {"host1", {"task2", "task3"}},
      {"host2", {"task4", "task5"}}, // Under maintenance
      {"host3", {"task6", "task7"}}, // Under maintenance
      {"host4", {"task8", "task9"}},
  };
  solver.setAssignment(assignment);

  // Task resources
  std::map<std::string, double> cpu_usage;
  std::map<std::string, double> memory_usage;
  for (const auto i : folly::irange(10)) {
    std::string task = fmt::format("task{}", i);
    cpu_usage[task] = 2.0;
    memory_usage[task] = 4.0;
  }

  solver.addObjectDimension("cpu", std::move(cpu_usage));
  solver.addObjectDimension("memory", std::move(memory_usage));

  // Host capacities
  std::map<std::string, double> host_cpu_capacity;
  std::map<std::string, double> host_memory_capacity;
  for (const auto i : folly::irange(5)) {
    std::string host = fmt::format("host{}", i);
    host_cpu_capacity[host] = 10.0;
    host_memory_capacity[host] = 20.0;
  }

  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);
  solver.addContainerDimension("memory_capacity", host_memory_capacity);

  // Hosts to drain
  std::vector<std::string> maintenance_hosts = {"host2", "host3"};

  // Add capacity constraints
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu";
  solver.addConstraint(std::move(cpuCapacity));

  CapacitySpec memoryCapacity;
  memoryCapacity.name() = "memory-capacity";
  memoryCapacity.scope() = "host";
  memoryCapacity.dimension() = "memory";
  solver.addConstraint(std::move(memoryCapacity));

  // Add ToFreeSpec to drain maintenance hosts
  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "drain-maintenance";
  toFreeSpec.containers() = maintenance_hosts;
  solver.addConstraint(std::move(toFreeSpec));

  // Block new assignments to maintenance hosts
  NonAcceptingSpec nonAcceptingSpec;
  nonAcceptingSpec.name() = "block-maintenance";
  nonAcceptingSpec.scope() = "host";
  nonAcceptingSpec.items() = maintenance_hosts;
  solver.addConstraint(std::move(nonAcceptingSpec));

  // Solve
  LocalSearchSolverSpec localSearchSpec;
  localSearchSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearchSpec.solveTime() = 10;
  solver.addSolver(localSearchSpec);
  auto solution = solver.solve();

  // Print results
  std::cout << "AssignmentSolution found in "
            << *solution.problemProfile()->solveSec() << "ms\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";

  // Verify draining
  std::cout << "\nVerifying draining:\n";
  verify_drained(solution, maintenance_hosts);

  return solution;
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  std::cout << "Running all ToFree examples...\n\n";

  std::cout << "1. Quick Example...\n";
  quick_example();

  std::cout << "\n2. Drain Hosts For Maintenance...\n";
  drain_hosts_for_maintenance();

  std::cout << "\n3. Decommission Old Hardware...\n";
  decommission_old_hardware();

  std::cout << "\n4. Drain Specific Resource Type...\n";
  drain_specific_resource_type();

  std::cout << "\n5. Gradual Draining As Goal...\n";
  gradual_draining_as_goal();

  std::cout << "\n6. Drain Prioritization...\n";
  drain_prioritization();

  std::cout << "\n7. Drain By Priority...\n";
  drain_by_priority();

  std::cout << "\n8. Constraint Usage...\n";
  constraint_usage();

  std::cout << "\n9. Goal Usage...\n";
  goal_usage();

  std::cout << "\n10. Combining With Capacity Spec...\n";
  combining_with_capacity_spec();

  std::cout << "\n11. Combining With Non Accepting Spec...\n";
  combining_with_non_accepting_spec();

  std::cout << "\n12. Verification Example...\n";
  verification_example();

  std::cout << "\n13. Incremental Drain...\n";
  std::vector<std::string> hosts_for_incremental = {
      "host1", "host2", "host3", "host4", "host5", "host6"};
  incremental_drain(hosts_for_incremental);

  std::cout << "\n14. Pitfall Wrong Dimension...\n";
  pitfall_wrong_dimension();

  std::cout << "\n15. Complete Example...\n";
  complete_example();

  std::cout << "\n✓ All ToFree examples completed successfully!\n";
  return 0;
}
// example_end
