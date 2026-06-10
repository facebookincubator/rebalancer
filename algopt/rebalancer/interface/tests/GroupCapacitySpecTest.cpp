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

#include "algopt/rebalancer/interface/tests/utils.h"
#include "algopt/rebalancer/tests/SolverTestUtils.h"
#include <algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h>

#include <folly/container/irange.h>
#include <gtest/gtest.h>

using facebook::algopt::isSolverUnavailable;
using facebook::algopt::solverName;
using facebook::algopt::testSolverPackages;

namespace facebook::rebalancer::interface::tests {

class GroupCapacitySpecTest
    : public ::testing::TestWithParam<
          std::tuple<int, SolverAlgoType, OptimalSolverPackage>> {
 protected:
  static GroupCapacitySpec getGroupCapacitySpec(
      std::map<std::string, double> groupLimits,
      std::map<std::string, std::map<std::string, double>>
          scopeItemContributions,
      GroupCapacitySpecDefinition definition =
          GroupCapacitySpecDefinition::AFTER,
      GroupCapacitySpecBound bound = GroupCapacitySpecBound::MAX);

  void SetUp() override;

  static int getThreadCount() {
    const auto [threadCount, algoType, solver] = GetParam();
    return threadCount;
  }
  static SolverAlgoType getSolverAlgoType() {
    const auto [threadCount, algoType, solver] = GetParam();
    return algoType;
  }
  static OptimalSolverPackage getSolverPackage() {
    const auto [threadCount, algoType, solver] = GetParam();
    return solver;
  }

  std::unique_ptr<ProblemSolver> solver_;
  std::map<std::string, std::string> taskToJob_;
};

GroupCapacitySpec GroupCapacitySpecTest::getGroupCapacitySpec(
    std::map<std::string, double> groupLimits,
    std::map<std::string, std::map<std::string, double>> scopeItemContributions,
    GroupCapacitySpecDefinition definition,
    GroupCapacitySpecBound bound) {
  GroupCapacitySpec groupCapacitySpec;
  groupCapacitySpec.scope() = "host";
  groupCapacitySpec.partitionName() = "job";
  groupCapacitySpec.contributionPartition() = "job";
  groupCapacitySpec.definition() = definition;
  groupCapacitySpec.bound() = bound;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1;
  limit.groupLimits() = std::move(groupLimits);

  Limit contribution;
  contribution.type() = LimitType::ABSOLUTE;
  contribution.globalLimit() = 0.0;
  contribution.scopeItemToGroupLimits() = std::move(scopeItemContributions);

  groupCapacitySpec.limit() = std::move(limit);
  groupCapacitySpec.contribution() = std::move(contribution);
  return groupCapacitySpec;
}

void GroupCapacitySpecTest::SetUp() {
  solver_ =
      initializeTestProblemSolver({.executorThreadCount = getThreadCount()});
  solver_->setObjectName("task");
  solver_->setContainerName("host");

  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2", "task3", "task4", "task5"}},
          {"host1", {}},
          {"host2", {}},
      });

  taskToJob_ = {
      {"task0", "job0"},
      {"task1", "job0"},
      {"task2", "job1"},
      {"task3", "job1"},
      {"task4", "job2"},
      {"task5", "job2"},
  };
  solver_->addPartition("job", taskToJob_);

  switch (getSolverAlgoType()) {
    case SolverAlgoType::LOCALSEARCH:
      solver_->addSolver(makeDefaultLocalSearchSolver());
      break;
    case SolverAlgoType::OPTIMAL: {
      const auto solver = getSolverPackage();
      if (isSolverUnavailable(solver)) {
        GTEST_SKIP() << solverName(solver) << " solver not available";
      }
      auto optimalSpec = interface::OptimalSolverSpec{};
      optimalSpec.solverPackage() = solver;
      solver_->addSolver(optimalSpec);
      break;
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    LocalSearch,
    GroupCapacitySpecTest,
    ::testing::Combine(
        testThreadCounts(),
        ::testing::Values(SolverAlgoType::LOCALSEARCH),
        ::testing::Values(OptimalSolverPackage::GUROBI)));

INSTANTIATE_TEST_CASE_P(
    Optimal,
    GroupCapacitySpecTest,
    ::testing::Combine(
        testThreadCounts(),
        ::testing::Values(SolverAlgoType::OPTIMAL),
        testSolverPackages()));

TEST_P(GroupCapacitySpecTest, oneJobPerHost) {
  // Make host_i expensive for all job_j with i != j
  // This forces the initial assignment to shuffle so that job_i gets host_i
  solver_->addConstraint(getGroupCapacitySpec(
      /* every job has limit of 2 */
      {{"job0", 2}, {"job1", 2}, {"job2", 2}},
      /* contribution of an object of job_i is 1 on host_i */
      {{"host0", {{"job0", 1}, {"job1", 10}, {"job2", 10}}},
       {"host1", {{"job0", 10}, {"job1", 1}, {"job2", 10}}},
       {"host2", {{"job0", 10}, {"job1", 10}, {"job2", 1}}}}));

  auto solution = solver_->solve();
  // Count how many tasks of each job are in each host.
  std::map<std::string, std::map<std::string, int>> numTasksPerHostPerGroup;
  for (auto& [task, host] : *solution.assignment()) {
    auto group = taskToJob_.at(task);
    ++numTasksPerHostPerGroup[host][group];
  }
  EXPECT_EQ(2, numTasksPerHostPerGroup["host0"]["job0"]);
  EXPECT_FALSE(numTasksPerHostPerGroup["host0"].contains("job1"));
  EXPECT_FALSE(numTasksPerHostPerGroup["host0"].contains("job2"));
  EXPECT_EQ(2, numTasksPerHostPerGroup["host1"]["job1"]);
  EXPECT_FALSE(numTasksPerHostPerGroup["host1"].contains("job2"));
  EXPECT_FALSE(numTasksPerHostPerGroup["host1"].contains("job0"));
  EXPECT_EQ(2, numTasksPerHostPerGroup["host2"]["job2"]);
  EXPECT_FALSE(numTasksPerHostPerGroup["host2"].contains("job0"));
  EXPECT_FALSE(numTasksPerHostPerGroup["host2"].contains("job1"));
}

TEST_P(GroupCapacitySpecTest, jobDiversity) {
  // we want to increase diversity on each host, that is
  // host0 has both job0 and job1
  // host1 has both job1 and job2
  // host2 has both job2 and job0
  solver_->addConstraint(getGroupCapacitySpec(
      /* every job has limit of 2 */
      {{"job0", 11}, {"job1", 11}, {"job2", 11}},
      /* contribution of an object of job_i is 1 on host_i */
      {{"host0", {{"job0", 1}, {"job1", 10}, {"job2", 20}}},
       {"host1", {{"job0", 20}, {"job1", 1}, {"job2", 10}}},
       {"host2", {{"job0", 10}, {"job1", 20}, {"job2", 1}}}},
      GroupCapacitySpecDefinition::AFTER,
      GroupCapacitySpecBound::EXACT));

  auto solution = solver_->solve();
  // Count how many tasks of each job are in each host.
  std::map<std::string, std::map<std::string, int>> numTasksPerHostPerGroup;
  for (auto& [task, host] : *solution.assignment()) {
    auto group = taskToJob_.at(task);
    ++numTasksPerHostPerGroup[host][group];
  }
  EXPECT_EQ(1, numTasksPerHostPerGroup["host0"]["job0"]);
  EXPECT_EQ(1, numTasksPerHostPerGroup["host0"]["job1"]);
  EXPECT_FALSE(numTasksPerHostPerGroup["host0"].contains("job2"));
  EXPECT_EQ(1, numTasksPerHostPerGroup["host1"]["job1"]);
  EXPECT_EQ(1, numTasksPerHostPerGroup["host1"]["job2"]);
  EXPECT_FALSE(numTasksPerHostPerGroup["host1"].contains("job0"));
  EXPECT_EQ(1, numTasksPerHostPerGroup["host2"]["job2"]);
  EXPECT_EQ(1, numTasksPerHostPerGroup["host2"]["job0"]);
  EXPECT_FALSE(numTasksPerHostPerGroup["host2"].contains("job1"));
}

TEST_P(GroupCapacitySpecTest, PlaceExactlyOneFromInnerPartition) {
  if (getSolverAlgoType() == SolverAlgoType::LOCALSEARCH) {
    GTEST_SKIP() << "MIP-only test";
  }
  // A common requirement is to enforce the following constraint: we have two
  // partition P1 and P2, where P2 is a more granular version of P1 and we want
  // to ensure that for each group G_i in P1, we want to place exactly k_i
  // groups from P2 among all the groups in P2 that are a subset of G1
  // This test shows how to model that use GroupCapacitySpec
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = getThreadCount()});
  solver->setObjectName("shard");
  solver->setContainerName("rank");
  solver->setConstraintPolicy(interface::ConstraintPolicy::HARD);

  entities::Map<std::string, std::vector<std::string>> tableToShards;
  tableToShards["table0"] = {
      "table0_type0.0",
      "table0_type0.1",
      "table0_type0.2",
      "table0_type1.0",
      "table0_type2.0"};
  tableToShards["table1"] = {
      "table1_type0.0",
      "table1_type0.1",
      "table1_type0.2",
      "table1_type1.0",
      "table1_type2.0",
  };
  tableToShards["table2"] = {
      "table2_type0.0",
      "table2_type1.0",
      "table2_type2.0",
  };

  // create an initialAssignment where all objects are in dummy
  entities::Map<std::string, std::vector<std::string>> rankToShards;
  for (auto& [_, shards] : tableToShards) {
    for (auto& shard : shards) {
      rankToShards["dummy"].push_back(shard);
    }
  }
  // two empty ranks
  rankToShards["rank1"] = {};
  rankToShards["rank2"] = {};
  solver->setAssignment(rankToShards);

  // outerPartition
  solver->addPartition("table", tableToShards);

  // innerPartition

  entities::Map<std::string, std::vector<std::string>> tableTypeToShards;
  tableTypeToShards["table0_type0"] = {
      "table0_type0.0", "table0_type0.1", "table0_type0.2"};
  tableTypeToShards["table1_type0"] = {
      "table1_type0.0", "table1_type0.1", "table1_type0.2"};
  tableTypeToShards["table2_type0"] = {"table2_type0.0"};

  tableTypeToShards["table0_type1"] = {"table0_type1.0"};
  tableTypeToShards["table1_type1"] = {"table1_type1.0"};
  tableTypeToShards["table2_type1"] = {"table2_type1.0"};

  tableTypeToShards["table0_type2"] = {"table0_type2.0"};
  tableTypeToShards["table1_type2"] = {"table1_type2.0"};
  tableTypeToShards["table2_type2"] = {"table2_type2.0"};
  solver->addPartition("table_type", tableTypeToShards);

  // add a scope
  solver->addScope(
      "is_real_rank",
      entities::Map<std::string, std::string>{
          {"rank1", "real"}, {"rank2", "real"}, {"dummy", "NOT_real"}});

  // add storage dimension
  const entities::Map<std::string, double> objectToStorage = {
      // table0 type1 and type2 objects all require more storage than all type0
      // objects combined
      {"table0_type1.0", 4},
      {"table0_type2.0", 5},
      // table1 type2 object requires more more storage than all type0 and type
      // 1 objects combined
      {"table1_type2.0", 5},
      // table 2 objects need a lot of storage
      {"table2_type0.0", 4000},
      {"table2_type1.0", 4000},
      {"table2_type2.0", 4000}};
  solver->addObjectDimension("storage", objectToStorage, 1.0);

  // add constraint which enforces that for each table_i, exactly i+1 types of
  // that table is allocated across all valid ranks (rank1 and rank2). So, for
  // table0, we expect one type type to be placed, for table1 we expect two
  // types, and for table2 we expect all 3 types to be allocated
  {
    interface::GroupCapacitySpec groupCapacitySpec;
    groupCapacitySpec.scope() = "is_real_rank";
    groupCapacitySpec.filter()->itemsBlacklist() = {"NOT_real"};

    groupCapacitySpec.partitionName() = "table";
    groupCapacitySpec.contributionPartition() = "table_type";
    groupCapacitySpec.definition() =
        interface::GroupCapacitySpecDefinition::AFTER;
    groupCapacitySpec.bound() = interface::GroupCapacitySpecBound::EXACT;
    groupCapacitySpec.limit()->globalLimit() = 1;
    groupCapacitySpec.limit()->groupLimits() = {{"table1", 2}, {"table2", 3}};
    groupCapacitySpec.contribution()->globalLimit() = 1;
    groupCapacitySpec.utilType() = interface::GroupCapacitySpecUtilType::STEP;

    solver->addConstraint(groupCapacitySpec);
  }

  {
    // add a constraint saying if a type is allocated, then all the objects of
    // that type must be allocated
    interface::ColocateGroupsSpec colocateGroupsSpec;
    colocateGroupsSpec.scope() = "is_real_rank";
    colocateGroupsSpec.partitionName() = "table_type";
    colocateGroupsSpec.bound() = interface::ColocateGroupsSpecBound::MAX;
    colocateGroupsSpec.limits()->globalLimit() = 1.0;

    solver->addConstraint(colocateGroupsSpec);
  }

  // add a goal to minimize the total amount of allocated storage
  {
    interface::CapacitySpec capacitySpec;
    capacitySpec.scope() = "rank";
    capacitySpec.dimension() = "storage";
    capacitySpec.filter()->itemsBlacklist() = {"dummy"};
    capacitySpec.bound() = interface::CapacitySpecBound::MAX;
    capacitySpec.limit()->type() = LimitType::ABSOLUTE;
    capacitySpec.limit()->globalLimit() = 0.0;

    solver->addGoal(capacitySpec);
  }

  // use optimal solver since local search requires very specific move types to
  // achieve this
  {
    auto optimalSpec = interface::OptimalSolverSpec{};
    optimalSpec.solverPackage() = getSolverPackage();
    solver->addSolver(optimalSpec);
  }

  auto solution = solver->solve();
  auto& finalAssignment = *solution.assignment();

  // expect all table0_type0 objects to be allocated
  auto& table0Type0 = tableTypeToShards["table0_type0"];
  auto expectedAllocatedTable0Objects =
      std::set<std::string>(table0Type0.begin(), table0Type0.end());
  for (auto& object : tableToShards["table0"]) {
    if (expectedAllocatedTable0Objects.contains(object)) {
      EXPECT_NE("dummy", finalAssignment[object]) << " failed for " << object;
    } else {
      EXPECT_EQ("dummy", finalAssignment[object]) << " failed for " << object;
    }
  }

  // expect all table1_type0 and table1_type1 objects to be allocated
  auto& table1Type0 = tableTypeToShards["table1_type0"];
  auto& table1Type1 = tableTypeToShards["table1_type1"];
  auto expectedAllocatedTable1Objects =
      std::set<std::string>(table1Type0.begin(), table1Type0.end());
  expectedAllocatedTable1Objects.insert(table1Type1.begin(), table1Type1.end());
  for (auto& object : tableToShards["table1"]) {
    if (expectedAllocatedTable1Objects.contains(object)) {
      EXPECT_NE("dummy", finalAssignment[object]) << " failed for " << object;
    } else {
      EXPECT_EQ("dummy", finalAssignment[object]) << " failed for " << object;
    }
  }

  // expect all table2_type0, table2_type1, and table2_type2 objects (i.e., all
  // obejcts of table2) to be allocated
  for (auto& object : tableToShards["table2"]) {
    EXPECT_NE("dummy", finalAssignment[object]) << " failed for " << object;
  }
}

TEST_P(GroupCapacitySpecTest, MinimizeNumberOfInnerPartitions) {
  if (getSolverAlgoType() == SolverAlgoType::LOCALSEARCH) {
    GTEST_SKIP() << "MIP-only test";
  }
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = getThreadCount()});
  solver->setObjectName("server");
  solver->setContainerName("reservation");

  const std::map<std::string, std::vector<std::string>> reservationToServers = {
      {"unassigned",
       {"server3", "server4", "server5", "server6", "server8", "server11"}},
      {"reservation0",
       // evenAiZone, FD1 (server0, server2), FD2 (server10)
       {"server0", "server2", "server10"}},
      {"reservation1",
       // oddAiZone, FD1 (server1) FD2 (server7, server9)
       {"server1", "server7", "server9"}},
  };
  solver->setAssignment(reservationToServers);

  // create an `aiZone` partition
  const std::map<std::string, std::vector<std::string>> aiZoneToServers = {
      {"evenAiZone",
       {"server0", "server2", "server4", "server6", "server8", "server10"}},
      {"oddAiZone",
       {"server1", "server3", "server5", "server7", "server9", "server11"}}};
  solver->addPartition("aiZone", aiZoneToServers);

  // create a `failureDomain` partition
  const std::map<std::string, std::vector<std::string>> serverTypeToServers = {
      {"FD1",
       {"server0", "server1", "server2", "server3", "server4", "server5"}},
      {"FD2",
       {"server6", "server7", "server8", "server9", "server10", "server11"}},
  };
  solver->addPartition("failureDomain", serverTypeToServers);

  // create a partition `aiZoneAndFailureDomain` which is a combination of the
  // `aiZone` and `failureDomain` partitions
  const std::map<std::string, std::vector<std::string>>
      aiZoneAndFailureDomainToServers = {
          {"evenFD1", {"server0", "server2", "server4"}},
          {"evenFD2", {"server6", "server8", "server10"}},
          {"oddFD1", {"server1", "server3", "server5"}},
          {"oddFD2", {"server7", "server9", "server11"}}};
  solver->addPartition(
      "aiZoneAndFailureDomain", aiZoneAndFailureDomainToServers);

  // Add capacity constraints to ensure that each reservation has exactly 3
  // servers
  CapacitySpec capacitySpec;
  capacitySpec.scope() = "reservation";
  capacitySpec.name() = "capacity LB";
  capacitySpec.dimension() = "server_count";
  capacitySpec.bound() = CapacitySpecBound::MIN;
  capacitySpec.limit()->type() = LimitType::ABSOLUTE;
  capacitySpec.limit()->globalLimit() = 3;
  capacitySpec.filter()->itemsBlacklist() = {"unassigned"};
  solver->addConstraint(capacitySpec);
  capacitySpec.name() = "capacity UB";
  capacitySpec.bound() = CapacitySpecBound::MAX;
  solver->addConstraint(capacitySpec);

  // For a given reservation R and aiZone, we want to ensure that the number of
  // failure domains R gets its servers from is minimized. That is, in the
  // initial assignment, reservation0 has servers from 2 failure domains (FD1
  // and FD2), and reservation1 has servers from 2 failure domains (FD1 and
  // FD2). We want to ensure that the final assignment has at most 1 failure
  // domain per reservation. So, we expect reservation0 to have servers from
  // either FD1 or FD2, and reservation1 to have servers from either FD1 or FD2.
  GroupCapacitySpec groupCapacitySpec;
  groupCapacitySpec.scope() = "reservation";
  groupCapacitySpec.partitionName() = "aiZone";
  groupCapacitySpec.contributionPartition() = "aiZoneAndFailureDomain";
  groupCapacitySpec.bound() = GroupCapacitySpecBound::MAX;
  groupCapacitySpec.limit()->type() = LimitType::ABSOLUTE;
  groupCapacitySpec.limit()->globalLimit() = 1;
  groupCapacitySpec.filter()->itemsBlacklist() = {"unassigned"};
  groupCapacitySpec.contribution()->globalLimit() = 1;
  groupCapacitySpec.utilType() = GroupCapacitySpecUtilType::STEP;
  solver->addConstraint(groupCapacitySpec);

  // Minimize the number of changes to the initial assignment
  MinimizeMovementSpec minimizeMovementSpec;
  minimizeMovementSpec.scope() = "reservation";
  minimizeMovementSpec.dimension() = "server_count";
  solver->addGoal(minimizeMovementSpec);

  OptimalSolverSpec optimalSolverSpec;
  optimalSolverSpec.solverPackage() = getSolverPackage();
  solver->addSolver(optimalSolverSpec);

  const auto solution = solver->solve();

  folly::F14FastMap<std::string, folly::F14FastSet<std::string>> result;
  const auto& finalAssignment = *solution.assignment();
  for (auto& [server, reservation] : finalAssignment) {
    result[reservation].insert(server);
    XLOG(INFO) << fmt::format("{} -> {}", server, reservation);
  }

  // expect reservation0 to have servers from either FD1 and evenAiZone
  const folly::F14FastSet<std::string> expectedReservation0Servers = {
      "server0", "server2", "server4"};
  // expect reservation1 to have servers from either FD2 and oddAiZone
  const folly::F14FastSet<std::string> expectedReservation1Servers = {
      "server7", "server9", "server11"};

  EXPECT_EQ(expectedReservation0Servers, result["reservation0"]);
  EXPECT_EQ(expectedReservation1Servers, result["reservation1"]);
}

TEST_P(GroupCapacitySpecTest, MinimizeNumberOfBundleViolations) {
  if (getSolverAlgoType() == SolverAlgoType::LOCALSEARCH) {
    // Local search requires specific setup to achieve this
    // so we skip this test for local search
    return;
  }

  for (auto enforceBundleHomogenity : {true, false}) {
    auto solver =
        initializeTestProblemSolver({.executorThreadCount = getThreadCount()});
    solver->setObjectName("server");
    solver->setContainerName("reservation");

    const int kNumServers = 16;
    std::vector<std::string> allServers;
    allServers.reserve(kNumServers);
    // msbId = serverId / 4
    const auto getMsbId = [](int serverId) { return serverId / 4; };
    // aiZoneId = serverId / 8
    const auto getAiZoneId = [](int serverId) { return serverId / 8; };
    folly::F14FastMap<std::string, int> serverToId;
    folly::F14FastMap<std::string, std::vector<std::string>> msbToServers;
    folly::F14FastMap<std::string, std::vector<std::string>> aiZoneToServers;
    for (const auto i : folly::irange(kNumServers)) {
      auto server = fmt::format("server{}", i);
      serverToId[server] = i;
      msbToServers[fmt::format("msb{}", getMsbId(i))].emplace_back(server);
      aiZoneToServers[fmt::format("aiZone{}", getAiZoneId(i))].emplace_back(
          server);
      allServers.push_back(std::move(server));
    }

    const std::map<std::string, std::vector<std::string>> reservationToServers =
        {
            {"unassigned", allServers},
            {"reservation0", {}},
        };
    solver->setAssignment(reservationToServers);

    // create an `aiZone` partition
    solver->addPartition("aiZone", aiZoneToServers);

    // create a `msb` partition
    solver->addPartition("msb", msbToServers);

    // Add capacity constraints to ensure that reservation0 gets 8 servers
    CapacitySpec capacitySpec;
    capacitySpec.scope() = "reservation";
    capacitySpec.name() = "capacity LB";
    capacitySpec.dimension() = "server_count";
    capacitySpec.bound() = CapacitySpecBound::MIN;
    capacitySpec.limit()->type() = LimitType::ABSOLUTE;
    capacitySpec.limit()->globalLimit() = 8;
    capacitySpec.filter()->itemsBlacklist() = {"unassigned"};
    solver->addConstraint(capacitySpec);
    capacitySpec.name() = "capacity UB";
    capacitySpec.bound() = CapacitySpecBound::MAX;
    solver->addConstraint(capacitySpec);

    // ensure that these servers are allocated in bundles of 4 (from the same
    // aiZone)
    const int kBundleSize = 4;
    GroupCountSpec groupCountSpec;
    groupCountSpec.scope() = "reservation";
    groupCountSpec.name() = "respect bundle size";
    groupCountSpec.partitionName() = "aiZone";
    groupCountSpec.definition() = GroupCountSpecDefinition::AFTER;
    groupCountSpec.bound() = GroupCountSpecBound::MULTIPLE;
    groupCountSpec.limit()->type() = LimitType::ABSOLUTE;
    groupCountSpec.limit()->globalLimit() = kBundleSize;
    solver->addConstraint(groupCountSpec);

    if (enforceBundleHomogenity) {
      // ensure that if possible servers of the bundle are allocated in the same
      // msb but within a reservation (across bundles), servers are allocated in
      // different msbs
      GroupCapacitySpec groupCapacitySpec;
      groupCapacitySpec.name() = "form complete bundles with msb";
      groupCapacitySpec.scope() = "reservation";
      groupCapacitySpec.partitionName() = "msb";
      groupCapacitySpec.bound() = GroupCapacitySpecBound::MAX;
      groupCapacitySpec.definition() = GroupCapacitySpecDefinition::AFTER;
      groupCapacitySpec.contributionPartition() = "msb";
      groupCapacitySpec.limit()->type() = LimitType::ABSOLUTE;
      groupCapacitySpec.limit()->globalLimit() = 0;
      groupCapacitySpec.filter()->itemsBlacklist() = {"unassigned"};
      groupCapacitySpec.contribution()->globalLimit() = 1;
      Limit bundleConfig;
      bundleConfig.globalLimit() = 1;
      bundleConfig.type() = LimitType::ABSOLUTE;
      bundleConfig.scopeItemLimits()->emplace("reservation0", kBundleSize);
      groupCapacitySpec.bundleConfig() = std::move(bundleConfig);
      groupCapacitySpec.utilType() = GroupCapacitySpecUtilType::STEP_MOD_K;
      solver->addGoal(groupCapacitySpec, /*weight=*/10);
    }

    // spread allocation of servers across msbs
    SumOfMaxSpec sumOfMaxSpec;
    sumOfMaxSpec.name() = "spread allocation across msbs";
    sumOfMaxSpec.scope() = "reservation";
    sumOfMaxSpec.partitionName() = "msb";
    sumOfMaxSpec.dimension() = "server_count";
    sumOfMaxSpec.filter()->itemsBlacklist() = {"unassigned"};
    solver->addGoal(sumOfMaxSpec);

    OptimalSolverSpec optimalSolverSpec;
    optimalSolverSpec.solverPackage() = getSolverPackage();
    solver->addSolver(optimalSolverSpec);

    const auto solution = solver->solve();

    folly::F14FastMap<std::string, int> countAllocatedPerMsb;
    const auto& finalAssignment = *solution.assignment();
    for (auto& [server, reservation] : finalAssignment) {
      if (reservation == "reservation0") {
        const auto& serverId = serverToId[server];
        countAllocatedPerMsb[fmt::format("msb{}", getMsbId(serverId))]++;
      }
    }

    // if bundle homegenity (that each bundle comes from the same msb) is not
    // enforced, we expect the allocation to be spread across all 4 msbs.
    // Otherwise, we expect the allocation to be spread across 2 msbs (each
    // offering one bundle of 4 servers)
    const auto numMsbsExpected = enforceBundleHomogenity ? 2 : 4;
    const auto numServersPerMsbExpected = enforceBundleHomogenity ? 4 : 2;
    EXPECT_EQ(numMsbsExpected, countAllocatedPerMsb.size());
    for (auto& [msb, count] : countAllocatedPerMsb) {
      EXPECT_EQ(numServersPerMsbExpected, count);
      XLOG(INFO) << fmt::format("{} -> {}", msb, count);
    }
  }
}

} // namespace facebook::rebalancer::interface::tests
