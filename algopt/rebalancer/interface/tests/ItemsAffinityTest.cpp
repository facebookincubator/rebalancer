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
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <folly/container/irange.h>
#include <folly/MapUtil.h>
#include <gtest/gtest.h>

#include <string>

using namespace std;
using namespace facebook::rebalancer::interface;

class ItemsAffinityTest
    : public ::testing::TestWithParam<std::tuple<int, bool>> {
 protected:
  void SetUp() override {
    solver_ = initializeTestProblemSolver(
        {.executorThreadCount = std::get<0>(GetParam())});
    solver_->setObjectName("server_part");
    solver_->setContainerName("reservation");
    if (std::get<1>(GetParam())) {
      solver_->useParallelizedNewMaterializer();
    }
  }
  void setupInstance(
      int numServers,
      int numPartsPerServer,
      std::map<std::string, std::vector<std::string>> initialAssignment,
      int demandPerReservation);

  int countNumLowHighPairs(
      const folly::F14FastMap<std::string, std::string>& assignment);

  std::unique_ptr<ProblemSolver> solver_;
  std::map<std::string, std::string> reservationType_;
  std::map<std::string, std::string> partsToServers_;
  const std::string kUnassigned = "unassigned";
  const std::string kLow = "lowCpu";
  const std::string kHigh = "highCpu";
  const std::string kNormal = "normal";
};

void ItemsAffinityTest::setupInstance(
    int numServers,
    int numPartsPerServer,
    std::map<std::string, std::vector<std::string>> initialAssignment,
    int demandPerReservation) {
  // In this example there are four reservations, two with highCPU requirement
  // and two with lowCPU requirement.
  std::map<std::string, std::vector<std::string>> serverToParts;
  for (const auto i : folly::irange(numServers)) {
    auto server = fmt::format("server_{}", i);
    for (const auto j : folly::irange(numPartsPerServer)) {
      auto part = fmt::format("server_part_{}_{}", i, j);
      serverToParts[server].push_back(part);
      partsToServers_[part] = server;
    }
  }

  // Initial assignment does not stack servers in an optimal way. In particular,
  // server1 is assigned both highCPU reservations
  solver_->setAssignment(initialAssignment);

  // In this example there are four reservations, two with highCPU
  // requirement and two with lowCPU requirement.
  // all reservations have similar memory requirement, so they require 2 server
  // parts each
  CapacitySpec capacitySpec;
  capacitySpec.scope() = "reservation";
  capacitySpec.dimension() = "server_part_count";
  capacitySpec.filter()->itemsBlacklist() = {kUnassigned};
  capacitySpec.bound() = CapacitySpecBound::MIN;
  capacitySpec.limit()->globalLimit() = demandPerReservation;
  solver_->addConstraint(capacitySpec);
  capacitySpec.bound() = CapacitySpecBound::MAX;
  capacitySpec.limit()->globalLimit() = demandPerReservation;
  solver_->addConstraint(capacitySpec);

  solver_->addPartition("server_partition", serverToParts);

  // This ensures group diversity. Every server must strive to having parts of
  // both highCPU and lowCPU types
  std::vector<std::string> highShapes;
  std::vector<std::string> lowShapes;
  for (const auto& [reservation, type] : reservationType_) {
    if (type == kLow) {
      lowShapes.push_back(reservation);
    } else if (type == kHigh) {
      highShapes.push_back(reservation);
    }
  }

  ItemsAffinitySpec itemsAffinitySpec;
  itemsAffinitySpec.scope() = "reservation";
  itemsAffinitySpec.partitionName() = "server_partition";
  itemsAffinitySpec.scopeItemsOfType1() = highShapes;
  itemsAffinitySpec.scopeItemsOfType2() = lowShapes;
  itemsAffinitySpec.dimension() = "server_part_count";
  solver_->addGoal(itemsAffinitySpec);

  // Use the optimal solver.
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  solver_->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());
}

int ItemsAffinityTest::countNumLowHighPairs(
    const folly::F14FastMap<std::string, std::string>& assignment) {
  std::map<std::string, std::map<std::string, int>> serverToShapeTypeCount;
  for (auto& [part, shape] : assignment) {
    auto server = partsToServers_.at(part);
    if (shape == kUnassigned) {
      continue;
    }
    auto type = folly::get_default(reservationType_, shape, kNormal);
    serverToShapeTypeCount[server][type]++;
  }
  int numPairs = 0;
  for (auto& [server, shapeCount] : serverToShapeTypeCount) {
    int numLow = folly::get_default(shapeCount, kLow, 0);
    int numHigh = folly::get_default(shapeCount, kHigh, 0);
    int numNormal = folly::get_default(shapeCount, kNormal, 0);
    XLOG(INFO) << fmt::format(
        "{} is assigned  L {} H {} N {}", server, numLow, numHigh, numNormal);
    numPairs += std::min(numLow, numHigh);
  }
  return numPairs;
}

INSTANTIATE_TEST_CASE_P(
    LShapeStacking,
    ItemsAffinityTest,
    ::testing::Combine(testThreadCounts(), ::testing::Values(false)));

INSTANTIATE_TEST_CASE_P(
    LShapeStackingWithParallelMaterializer,
    ItemsAffinityTest,
    ::testing::Combine(testThreadCounts(), ::testing::Values(true)));

TEST_P(ItemsAffinityTest, LShapeStacking) {
  constexpr int numServers = 3;
  constexpr int numPartsPerServer = 4;
  constexpr int demandPerReservation = 2;
  const std::map<std::string, std::vector<std::string>> initialAssignment = {
      {"reservation0", {"server_part_0_0", "server_part_0_1"}},
      {"reservation1", {"server_part_0_2", "server_part_0_3"}},
      {"reservation2", {"server_part_1_0", "server_part_1_1"}},
      {"reservation3", {"server_part_1_2", "server_part_1_3"}},
      {"reservation4", {"server_part_2_0", "server_part_2_1"}},
      {"reservation5", {"server_part_2_2", "server_part_2_3"}},
      {kUnassigned, {}}};

  // reservation4 and reservation5 are normal types (neither highCpu or lowCPU)
  reservationType_ = {
      {"reservation0", kLow},
      {"reservation1", kLow},
      {"reservation2", kHigh},
      {"reservation3", kHigh}};

  setupInstance(
      numServers, numPartsPerServer, initialAssignment, demandPerReservation);
  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  XLOG(INFO) << fmt::format(
      "Initial objective: {}, Final Objective: {}",
      *solution.initialObjective()->value(),
      *solution.finalObjective()->value());

  // Initially all 8 parts assigned to different servers => 0 pairs
  const int numPairsBefore =
      countNumLowHighPairs(*solution.initialAssignment());
  XLOG(INFO) << "Total number of pairs before solve: " << numPairsBefore;
  EXPECT_EQ(0, numPairsBefore);
  // All 8 parts assigned to L/H shapes are paired
  const int numPairsAfter = countNumLowHighPairs(*solution.assignment());
  XLOG(INFO) << "Total number of pairs after solve: " << numPairsAfter;
  EXPECT_EQ(4, numPairsAfter);
}

TEST_P(ItemsAffinityTest, LShapeStackingWithEmptySlots) {
  constexpr int numServers = 2;
  constexpr int numPartsPerServer = 4;
  constexpr int demandPerReservation = 1;
  const std::map<std::string, std::vector<std::string>> initialAssignment = {
      {"reservation0", {"server_part_0_0"}},
      {"reservation1", {"server_part_0_1"}},
      {"reservation2", {"server_part_1_0"}},
      {"reservation3", {"server_part_1_1"}},
      {"reservation4", {"server_part_0_2"}},
      {"reservation5", {"server_part_1_2"}},
      {kUnassigned, {"server_part_0_3", "server_part_1_3"}}};
  // reservation4 and reservation5 are normal types (neither highCpu or lowCPU)
  reservationType_ = {
      {"reservation0", kLow},
      {"reservation1", kLow},
      {"reservation2", kHigh},
      {"reservation3", kHigh},
      {"reservation4", kHigh},
      {"reservation5", kLow}};
  // Initial assignment is {2L, H} and {2H, L}
  // Final assignment should be {2H, 2L} {H, L}

  setupInstance(
      numServers, numPartsPerServer, initialAssignment, demandPerReservation);
  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  XLOG(INFO) << fmt::format(
      "Initial objective: {}, Final Objective: {}",
      *solution.initialObjective()->value(),
      *solution.finalObjective()->value());

  const int numPairsBefore =
      countNumLowHighPairs(*solution.initialAssignment());
  XLOG(INFO) << "Total number of pairs before solve: " << numPairsBefore;
  EXPECT_EQ(2, numPairsBefore);
  // All 8 parts assigned to L/H shapes are paired
  const int numPairsAfter = countNumLowHighPairs(*solution.assignment());
  XLOG(INFO) << "Total number of pairs after solve: " << numPairsAfter;
  EXPECT_EQ(3, numPairsAfter);
}

TEST_P(ItemsAffinityTest, LShapeStackingWithAllUnassigned) {
  constexpr int numServers = 2;
  constexpr int numPartsPerServer = 4;
  constexpr int demandPerReservation = 1;
  const std::map<std::string, std::vector<std::string>> initialAssignment = {
      {"reservation0", {}},
      {"reservation1", {}},
      {"reservation2", {}},
      {"reservation3", {}},
      {"reservation4", {}},
      {"reservation5", {}},
      {kUnassigned,
       {"server_part_0_0",
        "server_part_0_1",
        "server_part_0_2",
        "server_part_0_3",
        "server_part_1_0",
        "server_part_1_1",
        "server_part_1_2",
        "server_part_1_3"}}};
  // reservation4 and reservation5 are normal types (neither highCpu or lowCPU)
  reservationType_ = {
      {"reservation0", kLow},
      {"reservation1", kLow},
      {"reservation2", kHigh},
      {"reservation3", kHigh},
      {"reservation4", kHigh},
      {"reservation5", kLow}};
  // Initial assignment is {2L, H} and {2H, L}
  // Final assignment should be {2H, 2L} {H, L}

  setupInstance(
      numServers, numPartsPerServer, initialAssignment, demandPerReservation);
  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  XLOG(INFO) << fmt::format(
      "Initial objective: {}, Final Objective: {}",
      *solution.initialObjective()->value(),
      *solution.finalObjective()->value());

  const int numPairsBefore =
      countNumLowHighPairs(*solution.initialAssignment());
  XLOG(INFO) << "Total number of pairs before solve: " << numPairsBefore;
  EXPECT_EQ(0, numPairsBefore);
  // All 8 parts assigned to L/H shapes are paired
  const int numPairsAfter = countNumLowHighPairs(*solution.assignment());
  XLOG(INFO) << "Total number of pairs after solve: " << numPairsAfter;
  EXPECT_EQ(3, numPairsAfter);
}
