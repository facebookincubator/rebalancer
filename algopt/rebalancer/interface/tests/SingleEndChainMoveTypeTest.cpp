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
#include <vector>

using namespace std;
using namespace facebook::rebalancer::interface;

class ChainEndMoveTypeTest : public ::testing::TestWithParam<int> {
 protected:
  static std::unique_ptr<ProblemSolver> makeSolver() {
    auto solver =
        initializeTestProblemSolver({.executorThreadCount = GetParam()});
    solver->setObjectName("task");
    solver->setContainerName("host");
    // 3 hosts and 2 tasks
    const map<string, vector<string>> assignment = {
        {"h0", {"t0"}},
        {"h1", {"t1"}},
        {"h2", {}},
    };
    solver->setAssignment(assignment);

    // cpu dimension: no more than 1 task per host
    solver->addContainerDimension(
        "cpu",
        std::map<std::string, double>{{"h0", 100}, {"h1", 100}, {"h2", 100}});
    solver->addObjectDimension(
        "cpu", std::map<std::string, double>{{"t0", 80}, {"t1", 80}});

    {
      CapacitySpec capacitySpec;
      capacitySpec.scope() = "host";
      capacitySpec.dimension() = "cpu";
      solver->addConstraint(capacitySpec);
    }

    // flash dimension: t0 can only be placed in h0 and h1
    solver->addContainerDimension(
        "flash",
        std::map<std::string, double>{{"h0", 100}, {"h1", 100}, {"h2", 0}});
    solver->addObjectDimension(
        "flash", std::map<std::string, double>{{"t0", 10}, {"t1", 0}});

    {
      CapacitySpec capacitySpec;
      capacitySpec.scope() = "host";
      capacitySpec.dimension() = "flash";
      solver->addConstraint(capacitySpec);
    }

    // gpu dimension: t1 can only be placed in h1 and h2
    solver->addContainerDimension(
        "gpu",
        std::map<std::string, double>{{"h0", 0}, {"h1", 100}, {"h2", 100}});
    solver->addObjectDimension(
        "gpu", std::map<std::string, double>{{"t0", 0}, {"t1", 10}});

    {
      CapacitySpec capacitySpec;
      capacitySpec.scope() = "host";
      capacitySpec.dimension() = "gpu";
      solver->addConstraint(capacitySpec);
    }

    AssignmentAffinitiesSpec assignmentAffinitiesSpec;
    assignmentAffinitiesSpec.affinities() = {
        makeAssignmentAffinity("t0", "h1", 2.0),
        makeAssignmentAffinity("t1", "h1", 1.0),
    };
    assignmentAffinitiesSpec.scope() = "host";

    // affinities: both tasks want to be in h1 but t0 has a greater affinity
    solver->addGoal(assignmentAffinitiesSpec);
    return solver;
  }
};

INSTANTIATE_TEST_CASE_P(NumThreads, ChainEndMoveTypeTest, testThreadCounts());

TEST_P(ChainEndMoveTypeTest, WithChain) {
  auto solver = makeSolver();

  LocalSearchSolverSpec localSearchSolverSpec;
  localSearchSolverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleEndChainMoveTypeSpec()));
  solver->addSolver(localSearchSolverSpec);

  auto solution = solver->solve();
  // able to find the optimal assignment
  EXPECT_NEAR(-2.0, *solution.finalObjective()->value(), 1e-8);
  EXPECT_EQ("h1", solution.assignment()->at("t0"));
  EXPECT_EQ("h2", solution.assignment()->at("t1"));
}

TEST_P(ChainEndMoveTypeTest, WithoutChain) {
  auto solver = makeSolver();

  LocalSearchSolverSpec localSearchSolverSpec;
  localSearchSolverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearchSolverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SwapMoveTypeSpec()));
  solver->addSolver(localSearchSolverSpec);

  auto solution = solver->solve();
  // unable to find the optimal assignment
  EXPECT_NEAR(-1.0, *solution.finalObjective()->value(), 1e-8);
}
