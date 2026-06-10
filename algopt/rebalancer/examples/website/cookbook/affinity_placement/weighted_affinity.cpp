// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Weighted Affinity Scores Variation
 *
 * This variation demonstrates how to use different affinity strengths for
 * different relationships. Strong affinities are used for critical colocation
 * requirements, medium affinities for preferred placements, and weak affinities
 * for nice-to-have optimizations. This allows fine-grained control over
 * placement priorities.
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

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);
  auto executor = std::make_shared<folly::CPUThreadPoolExecutor>(4);
  ProblemSolver solver(executor, "microservice-placer", "production");

  solver.setObjectName("service_instance");
  solver.setContainerName("host");

  // Setup hosts
  std::vector<std::string> hosts;
  hosts.reserve(10);
  for (const auto i : folly::irange(10)) {
    hosts.push_back(fmt::format("host_{}", i));
  }

  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto& host : hosts) {
    assignment[host] = {};
  }

  // Create service instances
  std::map<std::string, int> services = {
      {"api_gateway", 5},
      {"cache", 10},
      {"logger", 15},
  };

  std::vector<std::string> all_instances;
  for (const auto& [service_name, count] : services) {
    for (const auto i : folly::irange(count)) {
      std::string instance_name = fmt::format("{}_{}", service_name, i);
      all_instances.push_back(instance_name);
      // Random initial assignment
      int host_idx = all_instances.size() % hosts.size();
      assignment[hosts[host_idx]].push_back(instance_name);
    }
  }

  solver.setAssignment(assignment);

  // Add dimensions
  std::map<std::string, double> resource_usage;
  for (const auto& inst : all_instances) {
    resource_usage[inst] = 4.0;
  }
  solver.addObjectDimension("cpu_usage", resource_usage);

  std::map<std::string, double> host_cpu_capacity;
  for (const auto& host : hosts) {
    host_cpu_capacity[host] = 64.0;
  }
  solver.addContainerDimension("cpu_capacity", host_cpu_capacity);

  CapacitySpec cpuCapacity;
  cpuCapacity.name() = "cpu-capacity";
  cpuCapacity.scope() = "host";
  cpuCapacity.dimension() = "cpu_usage";
  solver.addConstraint(cpuCapacity);

  // variation_start
  // Strong affinity (must colocate)
  AssignmentAffinity critical;
  critical.objectName() = "api_gateway_0";
  critical.scopeItemName() = "host_0";
  critical.affinity() = 100.0;

  // Medium affinity (prefer colocate)
  AssignmentAffinity moderate1;
  moderate1.objectName() = "cache_0";
  moderate1.scopeItemName() = "host_0";
  moderate1.affinity() = 5.0;

  AssignmentAffinity moderate2;
  moderate2.objectName() = "cache_1";
  moderate2.scopeItemName() = "host_0";
  moderate2.affinity() = 5.0;

  // Weak affinity (nice to have)
  AssignmentAffinity weak1;
  weak1.objectName() = "logger_0";
  weak1.scopeItemName() = "host_0";
  weak1.affinity() = 1.0;

  AssignmentAffinity weak2;
  weak2.objectName() = "logger_1";
  weak2.scopeItemName() = "host_0";
  weak2.affinity() = 1.0;
  // variation_end

  // Add affinity goals with different weights
  AssignmentAffinitiesSpec criticalSpec;
  criticalSpec.name() = "critical-affinity";
  criticalSpec.scope() = "host";
  criticalSpec.affinities() = {critical};
  solver.addGoal(criticalSpec, 10.0); // Highest priority

  AssignmentAffinitiesSpec moderateSpec;
  moderateSpec.name() = "moderate-affinity";
  moderateSpec.scope() = "host";
  moderateSpec.affinities() = {moderate1, moderate2};
  solver.addGoal(moderateSpec, 3.0); // Medium priority

  AssignmentAffinitiesSpec weakSpec;
  weakSpec.name() = "weak-affinity";
  weakSpec.scope() = "host";
  weakSpec.affinities() = {weak1, weak2};
  solver.addGoal(weakSpec, 1.0); // Low priority

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
