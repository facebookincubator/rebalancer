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

using namespace facebook::rebalancer::interface;

class ScopeAffinitiesTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, ScopeAffinitiesTest, testThreadCounts());

TEST_P(ScopeAffinitiesTest, Basic) {
  // In this test we use ScopeAffinitiesSpec to create a preference for specific
  // containers. Tasks compete to be placed in the preferred hosts in a way that
  // maximizes affinities.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {}},
          {"host2", {}}});

  // Create cpu dimension.
  solver->addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"task0", 20}, {"task1", 40}, {"task2", 80}});
  solver->addContainerDimension(
      "cpu",
      std::map<std::string, double>{
          {"host0", 100}, {"host1", 100}, {"host2", 100}});

  // Add capacity constraint.
  {
    CapacitySpec spec;
    spec.scope() = "host";
    spec.dimension() = "cpu";
    solver->addConstraint(spec);
  }

  // Add scope affinities: host2 is preferred, host1 is next best.
  {
    ScopeAffinitiesSpec spec;
    spec.scope() = "host";
    spec.dimension() = "cpu";
    spec.affinities() = {{"host1", 0.01}, {"host2", 0.1}};
    solver->addGoal(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  // Assert final assignment.
  EXPECT_EQ("host2", assignment.at("task0"));
  EXPECT_EQ("host1", assignment.at("task1"));
  EXPECT_EQ("host2", assignment.at("task2"));
}
