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

namespace facebook::rebalancer::interface::tests {

class DiversifyWithinScopeItemSpecTest : public ::testing::TestWithParam<int> {
 protected:
  void setUpProblem() {
    solver = initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver->setObjectName("trafficObject");
    solver->setContainerName("host");

    // Set some random assignment
    solver->setAssignment(
        std::vector<std::pair<std::string, std::vector<std::string>>>{
            {"region1-host1", {"p2-t1"}},
            {"region1-host2", {"p1-t1", "p1-t5", "p1-t6"}},
            {"region2-host1", {"p1-t2", "p2-t2", "p2-t3"}},
            {"region2-host2", {"p1-t3", "p1-t4", "p2-t4", "p2-t5", "p2-t6"}},
        });

    // Add a region scope
    solver->addScope(
        "region",
        std::vector<std::pair<std::string, std::vector<std::string>>>{
            {"region1", {"region1-host1", "region1-host2"}},
            {"region2", {"region2-host1", "region2-host2"}}});

    // add partition
    std::map<std::string, std::vector<std::string>> tenantToTrafficObjects;
    tenantToTrafficObjects["tenant1-trafficObjects"] = {
        "p1-t1", "p1-t2", "p1-t3", "p1-t4", "p1-t5", "p1-t6"};
    tenantToTrafficObjects["tenant2-trafficObjects"] = {
        "p2-t1", "p2-t2", "p2-t3", "p2-t4", "p2-t5", "p2-t6"};
    solver->addPartition("tenantTrafficObjects", tenantToTrafficObjects);

    {
      // To simplify the setup, let's set regional affinity for the objects.
      // This will allow easy verification of spread across hosts in a region.
      AssignmentAffinitiesSpec spec;
      spec.scope() = "region";
      // 2 objects of p1 and 4 objects of p2 have affinity to region1
      for (auto obj : {"p1-t1", "p1-t2", "p2-t1", "p2-t2", "p2-t3", "p2-t4"}) {
        AssignmentAffinity affinity;
        affinity.scopeItemName() = "region1";
        affinity.objectName() = obj;
        affinity.affinity() = 1;
        spec.affinities()->push_back(std::move(affinity));
      }

      // Similarly, 4 objects of p1 and 2 objects of p2 have affinity to region2
      for (auto obj : {"p1-t3", "p1-t4", "p1-t5", "p1-t6", "p2-t5", "p2-t6"}) {
        AssignmentAffinity affinity;
        affinity.scopeItemName() = "region2";
        affinity.objectName() = obj;
        affinity.affinity() = 1;
        spec.affinities()->push_back(std::move(affinity));
      }

      solver->addGoal(spec);
    }
  }

  std::unique_ptr<ProblemSolver> solver;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    DiversifyWithinScopeItemSpecTest,
    testThreadCounts());

TEST_P(DiversifyWithinScopeItemSpecTest, Basic) {
  setUpProblem();

  DiversifyWithinScopeItemSpec spec;
  spec.scope() = "region";
  spec.partition() = "tenantTrafficObjects";
  spec.dimension() = "trafficObject_count";
  solver->addGoal(spec);

  LocalSearchSolverSpec localSearchSolverSpec;
  localSearchSolverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  solver->addSolver(localSearchSolverSpec);
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // p1-t1 and p1-t2 are the only objects from p1 in region1. So we expect them
  // to be on different hosts.
  EXPECT_NE(assignment.at("p1-t1"), assignment.at("p1-t2"));

  // Similarly, p2-t5 and p2-t6 are the only objects from p2 in region1. So we
  // expect them to be on different hosts.
  EXPECT_NE(assignment.at("p2-t5"), assignment.at("p2-t6"));

  // Remaining objects from p1 and p2 should be spread across hosts in their
  // respective regions.
  {
    std::unordered_map<std::string, int> region2Count;
    for (auto& obj : {"p1-t3", "p1-t4", "p1-t5", "p1-t6"}) {
      region2Count[assignment[obj]] += 1;
    }

    EXPECT_EQ(region2Count.at("region2-host1"), 2);
    EXPECT_EQ(region2Count.at("region2-host2"), 2);
  }

  {
    std::unordered_map<std::string, int> region1Count;
    for (auto& obj : {"p2-t1", "p2-t2", "p2-t3", "p2-t4"}) {
      region1Count[assignment[obj]] += 1;
    }

    EXPECT_EQ(region1Count.at("region1-host1"), 2);
    EXPECT_EQ(region1Count.at("region1-host2"), 2);
  }
}

} // namespace facebook::rebalancer::interface::tests
