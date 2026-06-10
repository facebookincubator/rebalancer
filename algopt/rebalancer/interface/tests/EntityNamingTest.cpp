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

using namespace std;
using namespace facebook::rebalancer::interface;

class EntityNamingTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, EntityNamingTest, testThreadCounts());

TEST_P(EntityNamingTest, DotsInDimensionName) {
  // This test asserts that the following code does not throw exceptions
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});

  // Set the object and container names
  solver->setObjectName("shard");
  solver->setContainerName("host");

  // Set the initial assignment
  const map<string, vector<string>> initial_assignment = {
      {"h1", {"s1", "s2"}},
      {"h2", {}},
  };
  solver->setAssignment(initial_assignment);

  // Define cpu dimension
  const map<string, double> shard_cpu = {
      {"s1", 40},
      {"s2", 40},
  };
  const map<string, double> host_cpu = {
      {"h1", 100},
      {"h2", 100},
  };
  solver->addObjectDimension("cpu_load_percent.avg.60", shard_cpu);
  solver->addContainerDimension("cpu_load_percent.avg.60", host_cpu);

  // Add capacity constraint and balance goal on host cpu
  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "cpu_load_percent.avg.60";
  solver->addConstraint(capacitySpec);

  BalanceSpec balanceSpec;
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu_load_percent.avg.60";
  solver->addGoal(balanceSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  solver->solve();
}
