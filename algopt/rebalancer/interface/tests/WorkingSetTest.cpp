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

class WorkingSetTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, WorkingSetTest, testThreadCounts());

TEST_P(WorkingSetTest, Basic) {
  // This example simulates the problem of assigning URLs to buckets in the
  // context of Semantic Routing. It is known what functions get called by which
  // URLs and their cost, represented as working units. The goal is to minimize
  // the overall code working set. A bucket will be assigned to a number of
  // servers proportional to the cpu load of that bucket. Any function used by a
  // URL which runs on a given server needs to be loaded and counts towards the
  // working set size of that server. The solver minimizes the overall working
  // set size by grouping URLs which interact with the same functions under the
  // same bucket.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});

  solver->setObjectName("url");
  solver->setContainerName("bucket");

  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"bucket0", {"url0", "url1", "url2", "url3"}},
          {"bucket1", {}},
      });

  solver->addObjectDimension(
      "cpu",
      std::map<std::string, double>{
          {"url0", 1},
          {"url1", 1},
          {"url2", 1},
          {"url3", 1},
      });

  {
    WorkingSetSpec spec;
    spec.scope() = "bucket";
    spec.dimension() = "cpu";
    {
      WorkingUnit unit;
      unit.weight() = 1;
      unit.endpoints() = {"url0", "url1", "url2"};
      spec.workingUnits()->push_back(unit);
    }
    {
      WorkingUnit unit;
      unit.weight() = 10;
      unit.endpoints() = {"url0", "url1"};
      spec.workingUnits()->push_back(unit);
    }
    {
      WorkingUnit unit;
      unit.weight() = 10;
      unit.endpoints() = {"url2", "url3"};
      spec.workingUnits()->push_back(unit);
    }
    {
      WorkingUnit unit;
      unit.weight() = 100;
      unit.endpoints() = {"url0", "url1", "url2", "url3"};
      spec.workingUnits()->push_back(unit);
    }
    solver->addGoal(spec);
  }

  {
    // Make the optimal solution unique by pegging url0 to bucket0.
    AvoidMovingSpec spec;
    spec.objects() = {"url0"};
    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  // Moving task3 from host1 to host fixes the initial flow violation.
  EXPECT_EQ("bucket0", assignment.at("url0"));
  EXPECT_EQ("bucket0", assignment.at("url1"));
  EXPECT_EQ("bucket1", assignment.at("url2"));
  EXPECT_EQ("bucket1", assignment.at("url3"));

  EXPECT_NEAR(444.0, *solution.finalObjective()->value(), 1e-8);
}
