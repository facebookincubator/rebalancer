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

#include <folly/container/irange.h>
#include <gtest/gtest.h>

#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace facebook::rebalancer::interface;

class SwapNMoveTypeTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, SwapNMoveTypeTest, testThreadCounts());

TEST_P(SwapNMoveTypeTest, ThreeHostsPerPod) {
  // This test recreates a simpler version of the original CRP (capacity request
  // portal) problem which prompted the implementation of SWAP_N. An exact limit
  // constraint makes it impossible for other move types to allocate all
  // requested hosts.
  constexpr int hostCount = 200;
  constexpr int rackCount = 20;
  constexpr int podCount = 2;
  constexpr int requestCount = 6;

  // create solver
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("host");
  solver->setContainerName("rack");

  // set initial assignment
  map<string, vector<string>> rackToHosts;
  for (const auto hostId : folly::irange(hostCount)) {
    int rackId = hostId % rackCount;
    rackToHosts[fmt::format("rack{}", rackId)].push_back(
        fmt::format("host{}", hostId));
  }
  for (const auto requestId : folly::irange(requestCount)) {
    rackToHosts["unallocated_requests"].push_back(
        fmt::format("request{}", requestId));
  }
  solver->setAssignment(rackToHosts);

  // add pod scope
  map<string, string> rackToPod;
  for (const auto rackId : folly::irange(rackCount)) {
    int podId = rackId % podCount;
    rackToPod[fmt::format("rack{}", rackId)] = fmt::format("pod{}", podId);
  }
  solver->addScope("pod", rackToPod);

  // binary dimension representing whether a host is a request
  map<string, double> hostToRequest;
  for (const auto requestId : folly::irange(requestCount)) {
    hostToRequest[fmt::format("request{}", requestId)] = 1.0;
  }
  for (const auto hostId : folly::irange(hostCount)) {
    hostToRequest[fmt::format("host{}", hostId)] = 0.0;
  }
  solver->addObjectDimension("request", hostToRequest);

  // allocate all unallocated requests
  {
    CapacitySpec spec;
    spec.scope() = "rack";
    spec.dimension() = "request";
    spec.bound() = CapacitySpecBound::MAX;

    Limit limit;
    limit.globalLimit() = 0;

    Filter filter;
    filter.itemsWhitelist() = {{"unallocated_requests"}};

    spec.limit() = limit;
    spec.filter() = filter;

    solver->addConstraint(spec);
  }

  // exactly 3 requests in the same pod
  // constraint 1 of 2: at most 3 requests in the same pod
  {
    CapacitySpec spec;
    spec.scope() = "pod";
    spec.dimension() = "request";
    spec.bound() = CapacitySpecBound::MAX;

    Limit limit;
    limit.globalLimit() = 3;
    spec.limit() = limit;

    solver->addConstraint(spec);
  }

  // exactly 3 requests in the same pod
  // constraint 2 of 2: at least 3 requests in the same pod, or zero
  {
    CapacitySpec spec;
    spec.scope() = "pod";
    spec.dimension() = "request";
    spec.bound() = CapacitySpecBound::MIN;
    spec.zeroAllowed() = true;

    Limit limit;
    limit.globalLimit() = 3;
    spec.limit() = limit;

    solver->addConstraint(spec);
  }

  // configure swap n=3 move type
  map<string, vector<string>> podToRacks;
  for (auto& [rack, pod] : rackToPod) {
    podToRacks[pod].push_back(rack);
  }
  vector<vector<string>> destinationScope;
  destinationScope.reserve(podToRacks.size());
  for (auto& [_, racks] : podToRacks) {
    destinationScope.push_back(racks);
  }

  LocalSearchSolverSpec localSearchSolverSpec;
  SwapNMoveTypeSpec swapNMoveTypeSpec;
  swapNMoveTypeSpec.swapNConcurrentObjects() = 3;
  swapNMoveTypeSpec.swapNSourceObjects() =
      rackToHosts.at("unallocated_requests");
  swapNMoveTypeSpec.swapNDestinationScope() = destinationScope;
  swapNMoveTypeSpec.swapNIterations() = 100;
  localSearchSolverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(swapNMoveTypeSpec));

  solver->addSolver(localSearchSolverSpec);

  // compute solution
  auto solution = solver->solve();

  // assert 3 requests in each pod
  auto& assignment = *solution.assignment();
  map<string, int> podToRequestCount;
  for (const auto requestId : folly::irange(requestCount)) {
    ++podToRequestCount[rackToPod.at(
        assignment.at(fmt::format("request{}", requestId)))];
  }
  EXPECT_EQ(3, podToRequestCount["pod0"]);
  EXPECT_EQ(3, podToRequestCount["pod1"]);
}
