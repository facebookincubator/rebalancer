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

class AvoidAssignmentsTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, AvoidAssignmentsTest, testThreadCounts());

TEST_P(AvoidAssignmentsTest, Basic) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"h0", {"t0", "t1"}},
          {"h1", {"t2"}},
          {"h2", {"t3"}},
      });

  AvoidAssignmentsSpec avoidAssignmentsSpec;
  avoidAssignmentsSpec.scope() = "host";
  avoidAssignmentsSpec.assignments() = {
      makeAvoidAssignment("t0", {"h0", "h1"}),
      makeAvoidAssignment("t1", {"h0", "h2"}),
      makeAvoidAssignment("t2", {"h0"})};

  solver->addConstraint(avoidAssignmentsSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();
  auto& assignment = *solution.assignment();
  EXPECT_EQ("h2", assignment.at("t0"));
  EXPECT_EQ("h1", assignment.at("t1"));
  EXPECT_EQ("h1", assignment.at("t2"));
}
