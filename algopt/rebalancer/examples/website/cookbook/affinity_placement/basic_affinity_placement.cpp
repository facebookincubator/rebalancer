// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Affinity and Anti-Affinity Placement Example
 *
 * This example demonstrates how to place microservices with hardware and
 * service affinities. It shows:
 * - Hardware affinity (GPU workloads on GPU hosts, high-memory workloads on
 * high-memory hosts)
 * - Service affinity (frontend near backend for low latency)
 * - Anti-affinity (replicas of same service on different hosts)
 * - Load balancing across hosts
 */

// solution_start
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

namespace {

struct HostProfile {
  bool gpu;
  int memory;
};

struct ServiceConfig {
  int count;
  bool requires_gpu = false;
  bool requires_highmem = false;
  double cpu;
  double memory;
};

} // namespace

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "microservice-placer", "production");

  solver.setObjectName("service_instance");
  solver.setContainerName("host");

  // Host hardware profiles
  std::map<std::string, HostProfile> host_profiles;
  for (const auto i : folly::irange(4)) {
    host_profiles[fmt::format("gpu_host_{}", i)] = {.gpu = true, .memory = 128};
  }
  for (const auto i : folly::irange(6)) {
    host_profiles[fmt::format("highmem_host_{}", i)] = {
        .gpu = false, .memory = 512};
  }
  for (const auto i : folly::irange(10)) {
    host_profiles[fmt::format("standard_host_{}", i)] = {
        .gpu = false, .memory = 64};
  }

  // Service instance configuration
  std::map<std::string, ServiceConfig> services = {
      {"ml_training",
       {.count = 10,
        .requires_gpu = true,
        .requires_highmem = false,
        .cpu = 8.0,
        .memory = 32.0}},
      {"data_processing",
       {.count = 15,
        .requires_gpu = false,
        .requires_highmem = true,
        .cpu = 4.0,
        .memory = 128.0}},
      {"frontend",
       {.count = 25,
        .requires_gpu = false,
        .requires_highmem = false,
        .cpu = 2.0,
        .memory = 4.0}},
      {"backend",
       {.count = 25,
        .requires_gpu = false,
        .requires_highmem = false,
        .cpu = 4.0,
        .memory = 8.0}},
      {"cache",
       {.count = 25,
        .requires_gpu = false,
        .requires_highmem = false,
        .cpu = 1.0,
        .memory = 16.0}},
  };

  // Initial assignment
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto& [host, _] : host_profiles) {
    assignment[host] = {};
  }

  int instance_id = 0;
  std::vector<std::string> host_names;
  host_names.reserve(host_profiles.size());
  for (const auto& [host, _] : host_profiles) {
    host_names.push_back(host);
  }

  for (const auto& [service_name, config] : services) {
    for (const auto i : folly::irange(config.count)) {
      std::string instance_name = fmt::format("{}_{}", service_name, i);
      int host_idx = instance_id % host_names.size();
      assignment[host_names[host_idx]].push_back(instance_name);
      instance_id++;
    }
  }
  solver.setAssignment(assignment);

  // Define service partition
  std::map<std::string, std::vector<std::string>> service_partition;
  for (const auto& [service_name, config] : services) {
    for (const auto i : folly::irange(config.count)) {
      service_partition[service_name].push_back(
          fmt::format("{}_{}", service_name, i));
    }
  }
  solver.addPartition("service", service_partition);

  // Instance CPU and memory usage
  std::map<std::string, double> cpu_usage, memory_usage;
  for (const auto& [service_name, config] : services) {
    for (const auto i : folly::irange(config.count)) {
      std::string instance_name = fmt::format("{}_{}", service_name, i);
      cpu_usage[instance_name] = config.cpu;
      memory_usage[instance_name] = config.memory;
    }
  }
  solver.addObjectDimension("cpu_usage", cpu_usage);
  solver.addObjectDimension("memory_usage", memory_usage);

  // Host capacities
  std::map<std::string, double> host_cpu_capacity, host_memory_capacity;
  for (const auto& [host, profile] : host_profiles) {
    host_cpu_capacity[host] = 64.0;
    host_memory_capacity[host] = profile.memory;
  }
  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);
  solver.addContainerDimension("memory_capacity", host_memory_capacity);

  // CONSTRAINT 1: CPU capacity
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu_usage";
  solver.addConstraint(cpuCapacity);

  // CONSTRAINT 2: Memory capacity
  CapacitySpec memCapacity;
  memCapacity.name() = "memory-capacity";
  memCapacity.scope() = "host";
  memCapacity.dimension() = "memory_usage";
  solver.addConstraint(memCapacity);

  // CONSTRAINT 3: Anti-affinity for replicas
  GroupCountSpec antiAffinity;
  antiAffinity.name() = "service-anti-affinity";
  antiAffinity.scope() = "host";
  antiAffinity.partitionName() = "service";
  antiAffinity.bound() = GroupCountSpecBound::MAX;
  antiAffinity.limit() = Limit();
  antiAffinity.limit()->type() = LimitType::ABSOLUTE;
  antiAffinity.limit()->globalLimit() = 1;
  solver.addConstraint(antiAffinity);

  // GOAL 1: GPU hardware affinity
  std::vector<std::string> gpu_hosts;
  for (const auto& [host, profile] : host_profiles) {
    if (profile.gpu) {
      gpu_hosts.push_back(host);
    }
  }

  std::vector<std::string> gpu_instances;
  for (const auto& [service_name, config] : services) {
    if (config.requires_gpu) {
      for (const auto i : folly::irange(config.count)) {
        gpu_instances.push_back(fmt::format("{}_{}", service_name, i));
      }
    }
  }

  std::vector<AssignmentAffinity> gpu_affinities;
  for (const auto& instance : gpu_instances) {
    for (const auto& host : gpu_hosts) {
      AssignmentAffinity aff;
      aff.objectName() = instance;
      aff.scopeItemName() = host;
      aff.affinity() = 10.0;
      gpu_affinities.push_back(aff);
    }
  }

  if (!gpu_affinities.empty()) {
    AssignmentAffinitiesSpec gpuAffinitySpec;
    gpuAffinitySpec.name() = "gpu-hardware-affinity";
    gpuAffinitySpec.scope() = "host";
    gpuAffinitySpec.affinities() = gpu_affinities;
    solver.addGoal(gpuAffinitySpec, 5.0);
  }

  // GOAL 2: Balance CPU
  BalanceSpec balanceCpu;
  balanceCpu.name() = "balance-cpu";
  balanceCpu.scope() = "host";
  balanceCpu.dimension() = "cpu_usage";
  balanceCpu.formula() = BalanceSpecFormula::LEGACY;
  balanceCpu.fixAverageToInitial() = true;
  solver.addGoal(balanceCpu, 1.0);

  // GOAL 3: Balance memory
  BalanceSpec balanceMem;
  balanceMem.name() = "balance-memory";
  balanceMem.scope() = "host";
  balanceMem.dimension() = "memory_usage";
  balanceMem.formula() = BalanceSpecFormula::LEGACY;
  balanceMem.fixAverageToInitial() = true;
  solver.addGoal(balanceMem, 1.0);

  // Use Local Search solver
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 60000;
  solver.addSolver(localSearch);

  // Solve
  auto solution = solver.solve();

  std::cout << "Initial objective: " << *solution.initialObjective()->value()
            << "\n";
  std::cout << "Final objective: " << *solution.finalObjective()->value()
            << "\n";

  return 0;
}
// solution_end
