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
 * Basic Service Colocation for Microservices
 *
 * This example demonstrates how to colocate related microservices together
 * for performance benefits while maintaining fault tolerance through diversity.
 *
 * Problem: 60 service instances (20 frontend, 20 API, 20 cache) need to be
 * deployed across 20 hosts. Related services benefit from colocation (low
 * latency) but each service needs diversity across hosts for fault tolerance.
 *
 * Goal: Maximize colocation of service pairs while spreading each service type
 * across multiple hosts.
 */

// solution_start
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <folly/container/irange.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

void colocateMicroservices() {
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "microservice-colocation", "production");

  solver.setObjectName("instance");
  solver.setContainerName("host");

  // Service configuration
  struct ServiceConfig {
    int count;
    double cpu;
    double memory;
  };

  std::map<std::string, ServiceConfig> services = {
      {"frontend", {.count = 20, .cpu = 2.0, .memory = 4.0}},
      {"api", {.count = 20, .cpu = 4.0, .memory = 8.0}},
      {"cache", {.count = 20, .cpu = 1.0, .memory = 16.0}},
  };

  // Initial assignment (spread across all hosts)
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(20)) {
    assignment["host" + std::to_string(i)] = {};
  }

  int instance_id = 0;
  for (const auto& [service_name, config] : services) {
    for (const auto i : folly::irange(config.count)) {
      std::string instance_name =
          service_name + "_instance_" + std::to_string(i);
      int host_idx = instance_id % 20;
      assignment["host" + std::to_string(host_idx)].push_back(instance_name);
      instance_id++;
    }
  }

  solver.setAssignment(assignment);

  // Define service_type partition
  std::map<std::string, std::vector<std::string>> service_type_partition;
  for (const auto& [service_name, config] : services) {
    std::vector<std::string> instances;
    instances.reserve(config.count);
    for (const auto i : folly::irange(config.count)) {
      instances.push_back(service_name + "_instance_" + std::to_string(i));
    }
    service_type_partition[service_name] = instances;
  }
  solver.addPartition("service_type", std::move(service_type_partition));

  // Define service_pair partition
  std::map<std::string, std::vector<std::string>> service_pair_partition;

  // Frontend-API pairs
  for (const auto i : folly::irange(20)) {
    std::string pair_name = "frontend_api_pair_" + std::to_string(i);
    service_pair_partition[pair_name] = {
        "frontend_instance_" + std::to_string(i),
        "api_instance_" + std::to_string(i),
    };
  }

  // API-Cache pairs
  for (const auto i : folly::irange(20)) {
    std::string pair_name = "api_cache_pair_" + std::to_string(i);
    service_pair_partition[pair_name] = {
        "api_instance_" + std::to_string(i),
        "cache_instance_" + std::to_string(i),
    };
  }

  solver.addPartition("service_pair", std::move(service_pair_partition));

  // Instance resource usage
  std::map<std::string, double> cpu_usage;
  std::map<std::string, double> memory_usage;
  for (const auto& [service_name, config] : services) {
    for (const auto i : folly::irange(config.count)) {
      std::string instance_name =
          service_name + "_instance_" + std::to_string(i);
      cpu_usage[instance_name] = config.cpu;
      memory_usage[instance_name] = config.memory;
    }
  }
  solver.addObjectDimension("cpu_usage", std::move(cpu_usage));
  solver.addObjectDimension("memory_usage", std::move(memory_usage));

  // Host capacities
  std::map<std::string, double> host_cpu_capacity;
  std::map<std::string, double> host_memory_capacity;
  for (const auto i : folly::irange(20)) {
    std::string host = "host" + std::to_string(i);
    host_cpu_capacity[host] = 32.0;
    host_memory_capacity[host] = 128.0;
  }
  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);
  solver.addContainerDimension("memory_capacity", host_memory_capacity);

  // CONSTRAINT 1: CPU capacity
  CapacitySpec cpu_cap;
  cpu_cap.name() = "cpu-capacity";
  cpu_cap.scope() = "host";
  cpu_cap.dimension() = "cpu_usage";
  solver.addConstraint(std::move(cpu_cap));

  // CONSTRAINT 2: Memory capacity
  CapacitySpec mem_cap;
  mem_cap.name() = "memory-capacity";
  mem_cap.scope() = "host";
  mem_cap.dimension() = "memory_usage";
  solver.addConstraint(std::move(mem_cap));

  // CONSTRAINT 3: Max instances per service per host
  GroupCountSpec max_per_host;
  max_per_host.name() = "max-instances-per-service-per-host";
  max_per_host.scope() = "host";
  max_per_host.partitionName() = "service_type";
  max_per_host.bound() = GroupCountSpecBound::MAX;
  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 5;
  max_per_host.limit() = std::move(limit);
  solver.addConstraint(std::move(max_per_host));

  // GOAL 1: Colocate service pairs
  ColocateGroupsSpec colocate;
  colocate.name() = "colocate-service-pairs";
  colocate.scope() = "host";
  colocate.partitionName() = "service_pair";
  colocate.bound() = ColocateGroupsSpecBound::MAX;
  Limit coloc_limit;
  coloc_limit.type() = LimitType::ABSOLUTE;
  coloc_limit.globalLimit() = 1;
  colocate.limits() = std::move(coloc_limit);
  solver.addGoal(std::move(colocate), 5.0);

  // GOAL 2: Spread services
  ColocateGroupsSpec spread;
  spread.name() = "spread-services";
  spread.scope() = "host";
  spread.partitionName() = "service_type";
  spread.bound() = ColocateGroupsSpecBound::MIN;
  Limit spread_limit;
  spread_limit.type() = LimitType::ABSOLUTE;
  spread_limit.globalLimit() = 10;
  spread.limits() = std::move(spread_limit);
  solver.addGoal(std::move(spread), 3.0);

  // GOAL 3: Balance CPU
  BalanceSpec balance_cpu;
  balance_cpu.name() = "balance-cpu";
  balance_cpu.scope() = "host";
  balance_cpu.dimension() = "cpu_usage";
  balance_cpu.formula() = BalanceSpecFormula::LEGACY;
  balance_cpu.fixAverageToInitial() = true;
  solver.addGoal(std::move(balance_cpu), 1.0);

  // GOAL 4: Balance memory
  BalanceSpec balance_mem;
  balance_mem.name() = "balance-memory";
  balance_mem.scope() = "host";
  balance_mem.dimension() = "memory_usage";
  balance_mem.formula() = BalanceSpecFormula::LEGACY;
  balance_mem.fixAverageToInitial() = true;
  solver.addGoal(std::move(balance_mem), 1.0);

  // GOAL 5: Minimize movement
  MinimizeMovementSpec min_move;
  min_move.name() = "minimize-movement";
  min_move.scope() = "host";
  min_move.dimension() = "cpu_usage";
  solver.addGoal(std::move(min_move), 0.2);

  // Use Local Search solver
  LocalSearchSolverSpec ls_solver;
  ls_solver.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  ls_solver.solveTime() = 60000;
  solver.addSolver(ls_solver);

  // Solve
  auto solution = solver.solve();

  std::cout << "AssignmentSolution found in "
            << *solution.problemProfile()->solveSec() << " seconds\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";

  int move_count = 0;
  if (!solution.solverSummaries()->empty()) {
    auto& first_solver = solution.solverSummaries()->at(0);
    if (first_solver.moveStats()) {
      move_count = *first_solver.moveStats()->numMoves();
    }
  }
  std::cout << "Instances moved: " << move_count << "\n";
}

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  colocateMicroservices();
  return 0;
}
// solution_end
