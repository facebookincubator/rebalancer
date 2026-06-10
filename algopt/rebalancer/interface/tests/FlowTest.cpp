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

class FlowTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, FlowTest, testThreadCounts());

TEST_P(FlowTest, Basic) {
  // In this example there are 2 hosts and 4 tasks. A flow spec is carefully set
  // up so initial capacity is violated and moving a single task fixes it.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("task");
  solver->setContainerName("host");

  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task2"}},
          {"host1", {"task1", "task3"}},
      });

  solver->addObjectDimension(
      "cpu",
      std::vector<std::pair<std::string, double>>{
          {"task0", 10},
          {"task1", 100},
          {"task2", 20},
          {"task3", 200},
      });

  {
    FlowSpec spec;
    spec.scope() = "host";
    spec.dimension() = "cpu";
    {
      ObjectPair pair;
      pair.object1() = "task0";
      pair.object2() = "task1";
      spec.pairs()->push_back(pair);
    }
    {
      ObjectPair pair;
      pair.object1() = "task2";
      pair.object2() = "task3";
      spec.pairs()->push_back(pair);
    }
    auto& limits = *spec.limit()->scopeItemToGroupLimits();
    limits["host0"]["host1"] = 10;
    spec.sourceFilter()->itemsWhitelist() = {"host0"};
    auto& destinationFilters = *spec.destinationFilter();
    destinationFilters["host0"].itemsWhitelist() = {"host1"};
    solver->addConstraint(spec);
  }

  {
    AvoidMovingSpec spec;
    spec.objects() = {"task0", "task2"};
    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  // Moving task3 from host1 to host fixes the initial flow violation.
  EXPECT_EQ("host0", assignment.at("task0"));
  EXPECT_EQ("host1", assignment.at("task1"));
  EXPECT_EQ("host0", assignment.at("task2"));
  EXPECT_EQ("host0", assignment.at("task3"));
}
