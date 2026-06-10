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

#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;
using namespace facebook::rebalancer::entities;

class MinimizeSquaresTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(ThreadCounts, MinimizeSquaresTest, testThreadCounts());

TEST_P(MinimizeSquaresTest, Basic) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  const std::map<std::string, std::vector<std::string>> initialAssignment = {
      {"h1", {"t1", "t2", "t3"}}, {"h2", {"t4"}}, {"h3", {}}};
  solver->setAssignment(initialAssignment);

  const std::map<std::string, double> objectValues = {
      {"t1", 100}, {"t2", 5}, {"t3", 5}, {"t4", 10}};
  const std::map<std::string, double> scopeValues = {
      {"h1", 200}, {"h2", 200}, {"h3", 200}};
  solver->addObjectDimension("units", objectValues, 0);
  solver->addScopeDimension("units", "host", scopeValues);

  MinimizeSquaresSpec spec;
  spec.scope() = "host";
  spec.dimension() = "units";

  solver->addGoal(std::move(spec));

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  Map<std::string, int> hostToTaskCount;
  for (auto& [task, host] : assignment) {
    hostToTaskCount[host] += 1;
  }

  // ensure some task moved out of h1 and some task moved into h3.
  EXPECT_NE(3, hostToTaskCount["h1"]);
  EXPECT_NE(0, hostToTaskCount["h3"]);
}
