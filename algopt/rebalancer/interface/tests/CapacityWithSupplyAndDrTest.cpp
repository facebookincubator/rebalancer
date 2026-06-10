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

#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace facebook::rebalancer::interface;

class CapacityWithSupplyAndDrTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    CapacityWithSupplyAndDrTest,
    testThreadCounts());

TEST_P(CapacityWithSupplyAndDrTest, Simple) {
  // In this example there are 3 regions and 3 namespaces. Each namespace has a
  // primary and a secondary replica. The secondary replica only uses up cpu
  // when the region where the corresponding primary goes down.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});

  solver->setObjectName("namespace");
  solver->setContainerName("region");

  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"r1", {"p1", "s2", "s3"}},
          {"r2", {"p2", "p3"}},
          {"r3", {"s1"}},
      });

  solver->addObjectDimension(
      "cpu",
      std::unordered_map<std::string, double>{
          {"p1", 10},
          {"p2", 10},
          {"p3", 10},
          {"s1", 10},
          {"s2", 10},
          {"s3", 10},
      },
      0);

  solver->addPartition("empty", std::map<std::string, std::string>());
  solver->addPartition(
      "primaries",
      folly::F14FastMap<std::string, std::vector<std::string>>{
          {"primaries", {"p1", "p2", "p3"}}});

  {
    CapacityWithSupplyAndDrSpec spec;
    spec.scope() = "region";
    spec.dimension() = "cpu";
    spec.drPairs() = {
        {"s1", "p1"},
        {"s2", "p2"},
        {"s3", "p3"},
    };
    spec.prodScope() = "region";
    spec.prodItem() = "r1";
    spec.partitionName() = "primaries";
    spec.supplyPartition() = "empty";
    spec.ratio() = 1;
    spec.limit()->globalLimit() = 20;
    solver->addConstraint(spec);
  }

  {
    AvoidMovingSpec spec;
    spec.objects() = {"p1", "p2", "s2", "s3"};
    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  // The initial solution is broken: if region r2 goes down, then both
  // namespaces p2 and p3 will use up cpu on region r1, on top of p1, and will
  // exceed the limit of 20. Namespace p3 is expected to move from r2 to r3 to
  // mitigate this.
  EXPECT_EQ("r1", assignment.at("p1"));
  EXPECT_EQ("r2", assignment.at("p2"));
  EXPECT_EQ("r3", assignment.at("p3"));
  EXPECT_EQ("r3", assignment.at("s1"));
  EXPECT_EQ("r1", assignment.at("s2"));
  EXPECT_EQ("r1", assignment.at("s3"));
}
