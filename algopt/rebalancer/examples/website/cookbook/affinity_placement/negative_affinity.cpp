// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Negative Affinity (Avoid Assignments) Variation
 *
 * This variation demonstrates how to use AvoidAssignmentsSpec to enforce hard
 * incompatibility constraints. This is useful when certain workloads CANNOT run
 * on specific hosts due to hardware requirements or security restrictions.
 * Unlike soft affinity, this creates a hard constraint that cannot be violated.
 */

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

struct HostProfile {
  bool gpu;
  int memory;
};

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "microservice-placer", "production");

  solver.setObjectName("service_instance");
  solver.setContainerName("host");

  // Host hardware profiles
  std::map<std::string, HostProfile> host_profiles = {
      {"gpu_host_0", {.gpu = true, .memory = 128}},
      {"gpu_host_1", {.gpu = true, .memory = 128}},
      {"standard_host_0", {.gpu = false, .memory = 64}},
      {"standard_host_1", {.gpu = false, .memory = 64}},
      {"standard_host_2", {.gpu = false, .memory = 64}},
      {"standard_host_3", {.gpu = false, .memory = 64}},
  };

  // Initial assignment
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto& [host, _] : host_profiles) {
    assignment[host] = {};
  }

  // Create GPU instances
  std::vector<std::string> gpu_instances;
  gpu_instances.reserve(10);
  for (const auto i : folly::irange(10)) {
    gpu_instances.push_back(fmt::format("ml_training_{}", i));
  }

  std::vector<std::string> cpu_instances;
  cpu_instances.reserve(20);
  for (const auto i : folly::irange(20)) {
    cpu_instances.push_back(fmt::format("web_server_{}", i));
  }

  // Random initial assignment
  std::vector<std::string> all_hosts;
  all_hosts.reserve(host_profiles.size());
  for (const auto& [host, _] : host_profiles) {
    all_hosts.push_back(host);
  }

  std::vector<std::string> all_instances;
  all_instances.insert(
      all_instances.end(), gpu_instances.begin(), gpu_instances.end());
  all_instances.insert(
      all_instances.end(), cpu_instances.begin(), cpu_instances.end());

  for (const auto i : folly::irange(all_instances.size())) {
    int host_idx = i % all_hosts.size();
    assignment[all_hosts[host_idx]].push_back(all_instances[i]);
  }

  solver.setAssignment(assignment);

  // Add dimensions
  std::map<std::string, double> resource_usage;
  for (const auto& inst : gpu_instances) {
    resource_usage[inst] = 8.0;
  }
  for (const auto& inst : cpu_instances) {
    resource_usage[inst] = 2.0;
  }
  solver.addObjectDimension("cpu_usage", resource_usage);

  std::map<std::string, double> host_cpu_capacity;
  for (const auto& [host, _] : host_profiles) {
    host_cpu_capacity[host] = 64.0;
  }
  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);

  // CPU capacity constraint
  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu_usage";
  solver.addConstraint(cpuCapacity);

  // variation_start
  // GPU workloads CANNOT run on non-GPU hosts
  std::vector<std::string> non_gpu_hosts;
  for (const auto& [host, profile] : host_profiles) {
    if (!profile.gpu) {
      non_gpu_hosts.push_back(host);
    }
  }

  std::vector<AvoidAssignment> avoidances;
  for (const auto& inst : gpu_instances) {
    AvoidAssignment avoid;
    avoid.object() = inst;
    avoid.scopeItems() = non_gpu_hosts;
    avoidances.push_back(avoid);
  }

  AvoidAssignmentsSpec gpuRequirement;
  gpuRequirement.name() = "gpu-hardware-requirement";
  gpuRequirement.scope() = "host";
  gpuRequirement.assignments() = avoidances;
  solver.addConstraint(gpuRequirement);
  // variation_end

  // Use Local Search solver
  LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearch.solveTime() = 30000;
  solver.addSolver(localSearch);

  // Solve
  auto solution = solver.solve();

  std::cout << "AssignmentSolution found in " << "<solve time>" << "ms\n";
  std::cout << "Objective value: " << *solution.finalObjective()->value()
            << "\n";
  std::cout << "Instances moved: " << "<move count>" << "\n";

  return 0;
}
