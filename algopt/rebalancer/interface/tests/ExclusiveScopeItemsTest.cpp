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

#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/interface/tests/utils.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/Types_types.h"
#include "algopt/rebalancer/solver/utils/Util.h"
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;

class ExclusiveScopeItemsTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    ExclusiveScopeItemsTest,
    testThreadCounts());

static std::vector<ScopeItemConflictInfo> makeConflictInfoList(
    const std::vector<std::pair<std::string, std::vector<std::string>>>&
        scopeItems) {
  std::vector<ScopeItemConflictInfo> result;
  for (auto& [scopeItem, conflictingScopeItems] : scopeItems) {
    auto info = ScopeItemConflictInfo();
    info.scopeItem() = scopeItem;
    std::vector<ConflictingScopeItemInfo> conflictingScopeItemInfoList;
    for (auto& conflictingScopeItem : conflictingScopeItems) {
      auto conflictingScopeItemInfo = ConflictingScopeItemInfo();
      conflictingScopeItemInfo.conflictingScopeItem() = conflictingScopeItem;
      conflictingScopeItemInfoList.push_back(
          std::move(conflictingScopeItemInfo));
    }
    info.conflictingScopeItemsWithOverlap() = conflictingScopeItemInfoList;
    result.push_back(std::move(info));
  }
  return result;
}

TEST_P(ExclusiveScopeItemsTest, ExclusiveScopeItemsConstraint) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  const std::map<std::string, std::vector<std::string>> initialAssignment = {
      {"host1", {}}, {"host2", {"task0", "task1"}}, {"host3", {}}};
  solver->setAssignment(initialAssignment);

  std::vector<ScopeItemConflictInfo> conflictInfoList =
      makeConflictInfoList({{"host1", {"host2"}}, {"host2", {"host3"}}});

  auto spec = ExclusiveScopeItemsSpec();
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "task_count";
  spec.conflictInfoList() = std::move(conflictInfoList);
  solver->addConstraint(std::move(spec));

  // Use the optimal solver, rather than the default (local search).
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  // Initial constraint is satisfied, hence constraint value is 0
  EXPECT_NEAR(0, *solution.initialConstraint()->brokenVal(), 1e-9);
  EXPECT_EQ(0, *solution.initialConstraint()->brokenCount());
  ASSERT_EQ(1, solution.initialConstraint()->constraints()->size());
  EXPECT_EQ("test", *solution.initialConstraint()->constraints()->at(0).name());
  EXPECT_EQ(0, *solution.initialConstraint()->constraints()->at(0).value());
}

TEST_P(ExclusiveScopeItemsTest, ExclusiveScopeItemsBrokenConstraint) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  const std::map<std::string, std::vector<std::string>> initialAssignment = {
      {"host1", {"task1"}}, {"host2", {"task2"}}, {"host3", {"task3"}}};
  solver->setAssignment(initialAssignment);

  std::vector<ScopeItemConflictInfo> conflictInfoList = makeConflictInfoList(
      {{"host1", {"host2", "host3"}}, {"host2", {"host3"}}});

  auto spec = ExclusiveScopeItemsSpec();
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "task_count";
  spec.conflictInfoList() = std::move(conflictInfoList);

  solver->addConstraint(std::move(spec));

  // Use the optimal solver, rather than the default (local search).
  auto solverSpec = facebook::algopt::makeAvailableOptimalSolverSpec();
  solverSpec.skipInitialAssignmentHint() = true;
  solver->addSolver(solverSpec);

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();
  facebook::rebalancer::entities::Map<std::string, int> hostToTaskCount;
  for (auto& [task, host] : assignment) {
    hostToTaskCount[host] += 1;
  }

  int numAssigned = 0;
  for (auto& host : {"host1", "host2", "host3"}) {
    numAssigned += hostToTaskCount[host];
    EXPECT_TRUE(hostToTaskCount[host] == 0 || hostToTaskCount[host] == 3);
  }
  // ensures all tasks are assigned
  EXPECT_EQ(3, numAssigned);

  EXPECT_EQ("test", *solution.initialConstraint()->constraints()->at(0).name());
  // in order to satisfy the constraint, host2 should have all 3 tasks or 0
  // tasks
}

TEST_P(ExclusiveScopeItemsTest, ExclusiveScopeItemsConstraintPerGroup) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("server_part");
  solver->setContainerName("reservation");

  const std::map<std::string, std::vector<std::string>> initialAssignment = {
      {"reservation0", {"server_part_0", "server_part_4"}},
      {"reservation1", {"server_part_1", "server_part_2"}},
      {"reservation2", {"server_part_3", "server_part_5"}}};
  solver->setAssignment(initialAssignment);

  const std::map<std::string, std::string> partToSevers = {
      {"server_part_0", "server0"},
      {"server_part_1", "server0"},
      {"server_part_2", "server1"},
      {"server_part_3", "server1"},
      {"server_part_4", "server2"},
      {"server_part_5", "server2"},
  };
  std::map<std::string, std::vector<std::string>> serversToParts;
  for (const auto& [part, server] : partToSevers) {
    serversToParts[server].push_back(part);
  }
  // each server consists of two parts
  solver->addPartition("server", partToSevers);

  std::vector<ScopeItemConflictInfo> conflictingReservations =
      makeConflictInfoList(
          {{"reservation0", {"reservation1", "reservation2"}},
           {"reservation1", {"reservation2"}}});

  auto spec = ExclusiveScopeItemsSpec();
  spec.name() =
      "parts of same server cannot be allocated to conflicting reservations";
  spec.scope() = "reservation";
  spec.dimension() = "server_part_count";
  spec.conflictInfoList() = std::move(conflictingReservations);
  spec.partitionName() = "server";
  solver->addConstraint(std::move(spec));

  // Every reservation must get at most exactly two server parts
  CapacitySpec capacitySpec;
  capacitySpec.scope() = "reservation";
  capacitySpec.dimension() = "server_part_count";
  capacitySpec.bound() = CapacitySpecBound::MIN;
  capacitySpec.limit()->globalLimit() = 2;
  solver->addConstraint(capacitySpec);
  capacitySpec.bound() = CapacitySpecBound::MAX;
  solver->addConstraint(capacitySpec);

  // Use the optimal solver, rather than the default (local search).
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();

  XLOG(INFO) << "Final assignment: ";
  for (auto [part, reservation] : *solution.assignment()) {
    XLOG(INFO) << fmt::format("{} => {}", part, reservation);
  }

  for (auto& [server, parts] : serversToParts) {
    EXPECT_EQ(2, parts.size());
    auto part0 = parts.at(0);
    auto part1 = parts.at(1);
    EXPECT_EQ(
        solution.assignment()->at(part0), solution.assignment()->at(part1));
  }
}
