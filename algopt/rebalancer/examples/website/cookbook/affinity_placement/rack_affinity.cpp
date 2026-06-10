// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Rack Affinity Variation
 *
 * This variation demonstrates how to use rack-level affinity to prefer
 * instances on the same rack for low latency while maintaining host-level
 * anti-affinity. This is useful for colocating related services within the same
 * rack for performance while still spreading replicas across different hosts.
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

  // Setup hosts and racks
  int num_racks = 4;
  int hosts_per_rack = 5;
  std::map<std::string, std::string> host_to_rack;
  std::map<std::string, std::vector<std::string>> assignment;

  for (const auto rack_id : folly::irange(num_racks)) {
    for (const auto host_id : folly::irange(hosts_per_rack)) {
      std::string host_name = fmt::format("rack{}_host{}", rack_id, host_id);
      std::string rack_name = fmt::format("rack{}", rack_id);
      host_to_rack[host_name] = rack_name;
      assignment[host_name] = {};
    }
  }

  // Create service instances
  std::vector<std::string> services = {"frontend", "backend", "cache"};
  int instances_per_service = 20;

  std::map<std::string, std::vector<std::string>> service_partition;
  std::vector<std::string> host_names;
  host_names.reserve(assignment.size());
  for (const auto& [host, _] : assignment) {
    host_names.push_back(host);
  }

  for (const auto& service_name : services) {
    service_partition[service_name] = {};
    for (const auto i : folly::irange(instances_per_service)) {
      std::string instance_name = fmt::format("{}_{}", service_name, i);
      service_partition[service_name].push_back(instance_name);
      // Random initial assignment
      int host_idx =
          std::hash<std::string>{}(instance_name) % host_names.size();
      assignment[host_names[host_idx]].push_back(instance_name);
    }
  }

  solver.setAssignment(assignment);
  solver.addPartition("service", service_partition);

  // variation_start
  // Define rack topology
  solver.addScope("rack", host_to_rack);

  // Prefer frontend and backend on same rack
  ColocateGroupsSpec rackLocality;
  rackLocality.name() = "frontend-backend-rack-locality";
  rackLocality.scope() = "rack";
  rackLocality.partitionName() = "service";
  rackLocality.bound() = ColocateGroupsSpecBound::MAX;
  rackLocality.limits() = Limit();
  rackLocality.limits()->type() = LimitType::ABSOLUTE;
  rackLocality.limits()->globalLimit() = 2;
  solver.addGoal(rackLocality, 1.0);
  // variation_end

  // Maintain host-level anti-affinity
  GroupCountSpec hostAntiAffinity;
  hostAntiAffinity.name() = "host-anti-affinity";
  hostAntiAffinity.scope() = "host";
  hostAntiAffinity.partitionName() = "service";
  hostAntiAffinity.bound() = GroupCountSpecBound::MAX;
  hostAntiAffinity.limit() = Limit();
  hostAntiAffinity.limit()->type() = LimitType::ABSOLUTE;
  hostAntiAffinity.limit()->globalLimit() = 1;
  solver.addConstraint(hostAntiAffinity);

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
