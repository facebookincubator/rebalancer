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

class CapacityRatioTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, CapacityRatioTest, testThreadCounts());

TEST_P(CapacityRatioTest, Basic) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"h1", {"t1"}}, {"h2", {"t2"}}});

  const std::map<std::string, double> values = {{"t1", 10}};
  solver->addObjectDimension("dim", values, 0);

  CapacityRatioSpec spec;
  spec.name() = "test";
  spec.scope() = "host";
  spec.dimension() = "dim";
  spec.ratios() = {{"h1", {{"h2", 0.5}}}};

  solver->addConstraint(std::move(spec));

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();
  auto& assignment = *solution.assignment();
  auto initalAssignment = *solution.initialAssignment();

  // ensure t1 is moved to h2.
  EXPECT_EQ("h2", assignment["t1"]);
  EXPECT_EQ("h1", initalAssignment["t1"]);
}
