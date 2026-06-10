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

#include "algopt/rebalancer/benchmarks/utils/BenchmarkUtils.h"
#include "algopt/rebalancer/interface/tests/utils.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer::interface;
using SolverType = facebook::rebalancer::interface::benchmarks::SolverType;

void runGroupDiversity(int numServers, bool dynamic) {
  auto solver = initializeTestProblemSolver();
  // we will validate how GroupDiversitySpec can be used to reduce server
  // fragmentation. Each server has 12 GB memory
  // each odd reservation needs M3 = 3 GB memory shapes
  // each even reservation needs M4 = 8 GB memory shapes
  // initially, each reservation_i takes all its used parts from server_i
  // But clearly, this causes server fragmentation. Better solution uses fewer
  // servers
  const int numReservations = numServers;
  solver->setObjectName("server_part");
  solver->setContainerName("reservation");

  // If dynamic is false, we will use part_count dimension to model memory
  // That is, each server_part has 1 unit of memory. If dynamic is true, we will
  // use dynamic dimension to model memory. That is, each server_part has
  // contribution 3 or 8 depending on the reservation it is assigned to.
  // In this case, we will only need at most (12 / min(3, 8)) = 4 parts per
  // server
  const int numPartsPerServer = dynamic ? 4 : 12;

  folly::F14FastMap<std::string, std::string> serverPartToServer;
  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto i : folly::irange(numServers)) {
    const auto& server = fmt::format("server{}", i);
    const auto& reservation = fmt::format("reservation{}", i);
    const int reservationDemand = i % 2 == 0 ? 8 : 3;
    const int numAssignedParts = dynamic ? 1 : reservationDemand;
    for (const auto j : folly::irange(numPartsPerServer)) {
      const auto& serverPart = fmt::format("server_part_{}_{}", i, j);
      if (j < numAssignedParts) {
        assignment[reservation].push_back(serverPart);
      } else {
        assignment["unassigned"].push_back(serverPart);
      }
      serverPartToServer[serverPart] = server;
    }
  }
  solver->setAssignment(assignment);
  solver->addPartition("server", serverPartToServer);

  folly::F14FastMap<std::string, std::string> reservationToTrivialScopeItem;
  folly::F14FastMap<std::string, std::string> reservationToMemoryScopeItem;
  for (const auto i : folly::irange(numReservations)) {
    const auto& reservation = fmt::format("reservation{}", i);
    reservationToTrivialScopeItem[reservation] = "onlyScopeitem";
    // odd reservations need M3, even reservations need M8
    reservationToMemoryScopeItem[reservation] =
        fmt::format("M{}_scopeItem", i % 2 == 0 ? 8 : 3);
  }
  XLOG(INFO) << "reservationToTrivialScopeItem: ";
  solver->addScope("reservation_trivial", reservationToTrivialScopeItem);
  solver->addScope("reservation_memory", reservationToMemoryScopeItem);

  // setup a dynamic memory dimension
  if (dynamic) {
    folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
        serverPartToMemory;
    for (const auto i : folly::irange(numServers)) {
      for (const auto j : folly::irange(numPartsPerServer)) {
        const auto& serverPart = fmt::format("server_part_{}_{}", i, j);
        serverPartToMemory["M3_scopeItem"][serverPart] = 3;
        serverPartToMemory["M8_scopeItem"][serverPart] = 8;
      }
    }
    solver->addDynamicObjectDimension(
        "memory", "reservation_memory", serverPartToMemory);
  }

  const std::string kMemoryDimension = dynamic ? "memory" : "server_part_count";

  // add server resource constraint
  GroupCountSpec memoryLimit;
  memoryLimit.scope() = "reservation_trivial";
  memoryLimit.partitionName() = "server";
  memoryLimit.dimension() = kMemoryDimension;
  memoryLimit.bound() = GroupCountSpecBound::MAX;
  // each server has 12 GB memory
  memoryLimit.limit()->globalLimit() = 12;
  solver->addConstraint(std::move(memoryLimit));

  // add group diversity constraint (to reduce server fragmentation)
  GroupDiversitySpec spec;
  spec.partition() = "server";
  spec.dimension() = kMemoryDimension;
  spec.scope() = "reservation_trivial";
  spec.limit()->globalLimit() = 0;
  spec.bound() = GroupDiversityBound::MAX;
  solver->addGoal(std::move(spec));

  addSolver(
      *solver,
      SolverType::LOCAL_SEARCH,
      /*useParallelizedNewMaterializer=*/true,
      /*maxSolveTimeSeconds=*/100);
  auto solution = solver->solve();
}

BENCHMARK(GroupDiversity_part_count) {
  runGroupDiversity(/*numServers=*/1000, /*dynamic=*/false);
}
BENCHMARK(GroupDiversity_dynamic) {
  runGroupDiversity(/*numServers=*/1000, /*dynamic=*/true);
}

void runGroupDiversityMaterialization(int groupCount, int scopeItemCount) {
  folly::BenchmarkSuspender suspend;
  auto solver = initializeTestProblemSolver({.executorThreadCount = 1});
  solver->shouldUseDynamicObjectOrdering(false);
  solver->setObjectName("server");
  solver->setContainerName("reservation");

  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto scopeItem : folly::irange(scopeItemCount)) {
    assignment[fmt::format("reservation{}", scopeItem)] = {};
  }

  std::map<std::string, std::string> objectToGroup;
  for (const auto group : folly::irange(groupCount)) {
    const auto objectName = fmt::format("server{}", group);
    const auto groupName = fmt::format("group{}", group);
    objectToGroup[objectName] = groupName;
    assignment[fmt::format("reservation{}", group % scopeItemCount)].push_back(
        objectName);
  }

  solver->setAssignment(assignment);
  solver->addPartition("benchmark_group", objectToGroup);

  GroupDiversitySpec spec;
  spec.scope() = "reservation";
  spec.partition() = "benchmark_group";
  spec.dimension() = "server_count";
  spec.bound() = GroupDiversityBound::MAX;
  spec.limit()->globalLimit() = 1;
  solver->addGoal(std::move(spec));

  addSolver(
      *solver,
      SolverType::MATERIALIZE_ONLY,
      /*useParallelizedNewMaterializer=*/true);
  suspend.dismiss();

  const auto solution = solver->solve();
  folly::doNotOptimizeAway(*solution.initialObjective()->value());
}

BENCHMARK(GroupDiversity_materialize_static_max) {
  runGroupDiversityMaterialization(/*groupCount=*/256, /*scopeItemCount=*/2000);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
