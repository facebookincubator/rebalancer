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

class DrainCapacityTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, DrainCapacityTest, testThreadCounts());

TEST_P(DrainCapacityTest, Basic) {
  // In this example there are 3 hosts and 2 tasks. host0 spills 80% of its
  // utilization to host2 while host1 spills 50% of its utilization to host2.
  // task0 is free to move but task1 is pegged to host2.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0"}},
          {"host1", {}},
          {"host2", {"task1"}},
      });

  solver->addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 4},
          {"task1", 8},
      });

  solver->addContainerDimension(
      "cpu",
      folly::F14FastMap<std::string, double>{
          {"host0", 10}, {"host1", 10}, {"host2", 10}});

  {
    DrainCapacitySpec spec;
    spec.scope() = "host";
    spec.dimension() = "cpu";
    spec.spillDistribution() = {
        {"host0", {{"host2", 0.8}}},
        {"host1", {{"host2", 0.5}}},
    };
    solver->addConstraint(spec);
  }

  {
    AvoidMovingSpec spec;
    spec.objects() = {"task1"};
    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  // Moving task0 from host0 to host1 fixes the drain capacity violation.
  EXPECT_EQ("host1", assignment.at("task0"));
  EXPECT_EQ("host2", assignment.at("task1"));
}
