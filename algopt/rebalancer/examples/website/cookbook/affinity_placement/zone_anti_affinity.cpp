// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

/**
 * Zone Anti-Affinity Variation
 *
 * This variation demonstrates how to spread replicas across availability zones
 * for high availability and fault tolerance. This ensures that if one zone
 * fails, the application remains available with replicas in other zones.
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

  // Setup hosts across zones
  std::vector<std::string> zones = {"us-east-1a", "us-east-1b", "us-east-1c"};
  int hosts_per_zone = 5;
  std::map<std::string, std::string> host_to_zone;
  std::map<std::string, std::vector<std::string>> assignment;

  for (const auto& zone : zones) {
    for (const auto host_id : folly::irange(hosts_per_zone)) {
      std::string host_name = fmt::format("{}_host_{}", zone, host_id);
      host_to_zone[host_name] = zone;
      assignment[host_name] = {};
    }
  }

  // Create replica groups
  int num_replicas = 3;
  int instances_per_replica = 5;
  std::map<std::string, std::vector<std::string>> replica_to_instances;
  std::vector<std::string> host_names;

  host_names.reserve(assignment.size());
  for (const auto& [host, _] : assignment) {
    host_names.push_back(host);
  }

  for (const auto replica_id : folly::irange(num_replicas)) {
    std::string replica_name = fmt::format("replica_{}", replica_id);
    replica_to_instances[replica_name] = {};
    for (const auto instance_id : folly::irange(instances_per_replica)) {
      std::string instance_name =
          fmt::format("app_{}_{}", replica_id, instance_id);
      replica_to_instances[replica_name].push_back(instance_name);
      // Random initial assignment
      int host_idx = (replica_id * instances_per_replica + instance_id) %
          host_names.size();
      assignment[host_names[host_idx]].push_back(instance_name);
    }
  }

  solver.setAssignment(assignment);

  // Define zone scope
  solver.addScope("availability_zone", host_to_zone);

  // variation_start
  // Define zone partition
  solver.addPartition("replica_group", replica_to_instances);

  // Max 1 replica per zone
  GroupCountSpec zoneAntiAffinity;
  zoneAntiAffinity.name() = "zone-anti-affinity";
  zoneAntiAffinity.scope() = "availability_zone";
  zoneAntiAffinity.partitionName() = "replica_group";
  zoneAntiAffinity.bound() = GroupCountSpecBound::MAX;
  zoneAntiAffinity.limit() = Limit();
  zoneAntiAffinity.limit()->type() = LimitType::ABSOLUTE;
  zoneAntiAffinity.limit()->globalLimit() = 1;
  solver.addConstraint(zoneAntiAffinity);
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
