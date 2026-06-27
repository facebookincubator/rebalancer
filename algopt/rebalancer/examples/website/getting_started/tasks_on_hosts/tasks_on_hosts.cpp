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
 * Tutorial: Build Your First Model.
 *
 * Places 12 tasks on 4 hosts (all initially on host0) while:
 *   - balancing memory use across hosts,
 *   - keeping host0 empty (it is going down for maintenance),
 *   - keeping each job's two tasks in different racks.
 */

// solution_start
#include "algopt/rebalancer/interface/ProblemSolverFactory.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace interface = facebook::rebalancer::interface;

int main(int argc, char* argv[]) {
  const folly::Init init(&argc, &argv);

  // solver_instance_start
  // Create the solver object. The factory gives us one with a ready-to-use
  // thread pool, so we only pass a name and scope (used for logging).
  auto solver = interface::ProblemSolverFactory::makeProblemSolver(
      /*serviceName=*/"rebalancer", /*serviceScope=*/"examples");
  // solver_instance_end

  // basic_attrs_start
  // Tell the solver what we place (tasks) and where they go (hosts).
  solver->setObjectName("task");
  solver->setContainerName("host");

  // Start with all 12 tasks on host0 and the other hosts empty. This also
  // tells the solver the full list of tasks and hosts to work with.
  std::map<std::string, std::vector<std::string>> hostToTasks;
  for (const auto i : folly::irange(12)) {
    hostToTasks["host0"].push_back(fmt::format("task{}", i));
  }
  hostToTasks["host1"] = {};
  hostToTasks["host2"] = {};
  hostToTasks["host3"] = {};
  solver->setAssignment(hostToTasks);
  // basic_attrs_end

  // obj_dim_start
  // How much memory each task needs, in GB. Every task here needs 1 GB.
  std::map<std::string, double> taskToMemory;
  for (const auto i : folly::irange(12)) {
    taskToMemory[fmt::format("task{}", i)] = 1.0;
  }
  solver->addObjectDimension("memory", taskToMemory);
  // obj_dim_end

  // container_dim_start
  // How much memory each host has, in GB. host1 is twice as big as the rest.
  const std::map<std::string, double> hostToMemory = {
      {"host0", 10.0}, {"host1", 20.0}, {"host2", 10.0}, {"host3", 10.0}};
  solver->addContainerDimension("memory", hostToMemory);
  // container_dim_end

  // balance_start
  // Goal: balance memory utilization across hosts.
  interface::BalanceSpec balance;
  balance.name() = "balance-hosts";
  balance.scope() = "host";
  balance.dimension() = "memory";
  balance.formula() = interface::BalanceSpecFormula::LINEAR;
  solver->addGoal(balance, /*weight=*/1.0, /*tuplePos=*/0);
  // balance_end

  // scope_start
  // Group hosts into racks: rack0 holds host0 and host1, rack1 holds the rest.
  const std::map<std::string, std::vector<std::string>> rackToHosts = {
      {"rack0", {"host0", "host1"}}, {"rack1", {"host2", "host3"}}};
  solver->addScope("rack", rackToHosts);
  // scope_end

  // partition_start
  // Group tasks into jobs of two tasks each: job0 = task0, task1; and so on.
  std::map<std::string, std::vector<std::string>> jobToTasks;
  for (const auto i : folly::irange(12)) {
    jobToTasks[fmt::format("job{}", i / 2)].push_back(fmt::format("task{}", i));
  }
  solver->addPartition("job", jobToTasks);
  // partition_end

  // tofree_start
  // Constraint: host0 is going down for maintenance, so it must end up empty.
  interface::ToFreeSpec toFree;
  toFree.name() = "make-host0-empty";
  toFree.containers() = {"host0"};
  solver->addConstraint(toFree);
  // tofree_end

  // groupcount_start
  // Constraint: keep a job's two tasks in different racks, so losing one rack
  // can't take down a whole job. (At most 1 task per job in any rack.)
  interface::GroupCountSpec groupCount;
  groupCount.name() = "job-rack-fault-tolerance";
  groupCount.scope() = "rack";
  groupCount.partitionName() = "job";
  groupCount.bound() = interface::GroupCountSpecBound::MAX;
  interface::Limit limit;
  limit.type() = interface::LimitType::ABSOLUTE;
  limit.globalLimit() = 1;
  groupCount.limit() = std::move(limit);
  solver->addConstraint(std::move(groupCount));
  // groupcount_end

  // solve_print_start
  // Pick the local-search solver, then solve.
  interface::LocalSearchSolverSpec localSearch;
  localSearch.moveTypeList()->push_back(
      interface::ProblemSolver::makeMoveTypeSpec(
          interface::SingleMoveTypeSpec()));
  solver->addSolver(localSearch);

  const auto solution = solver->solve();

  // solution.assignment() tells us, for each task, the host it landed on.
  // Group the placed tasks by host.
  std::map<std::string, std::vector<std::string>> hostToFinalTasks;
  for (const auto& [task, host] : *solution.assignment()) {
    hostToFinalTasks[host].push_back(task);
  }

  // Look up each task's job so we can show it next to the task.
  std::map<std::string, std::string> taskToJob;
  for (const auto& [job, tasks] : jobToTasks) {
    for (const auto& task : tasks) {
      taskToJob[task] = job;
    }
  }

  // Reverse rackToHosts so we can look up each host's rack when printing.
  std::map<std::string, std::string> hostToRack;
  for (const auto& [rack, hosts] : rackToHosts) {
    for (const auto& host : hosts) {
      hostToRack[host] = rack;
    }
  }

  // Print each host with its rack, and each task with its job.
  std::cout << "\n=== Final placement ===\n";
  for (auto& [host, tasks] : hostToFinalTasks) {
    std::sort(tasks.begin(), tasks.end());
    std::cout << host << " (" << hostToRack.at(host) << "):";
    for (const auto& task : tasks) {
      std::cout << " " << task << "(" << taskToJob.at(task) << ")";
    }
    std::cout << "\n";
  }
  // solve_print_end

  return 0;
}
// solution_end
