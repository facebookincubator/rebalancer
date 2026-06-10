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
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <fmt/core.h>
#include <folly/container/irange.h>
#include <gtest/gtest.h>

#include <map>

using namespace std;
using namespace facebook::rebalancer::interface;

class BipartiteSwapsTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, BipartiteSwapsTest, testThreadCounts());

TEST_P(BipartiteSwapsTest, Basic) {
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("host");
  solver->setContainerName("rack");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"atn2.08.nc", {"cache0017.08.atn2"}},
          {"atn2.08.nd", {"twshared0033.atn3"}},
          {"atn2.08.ne", {}},
      });
  BipartiteSwapsSpec bipartiteSwapsSpec;
  bipartiteSwapsSpec.name() = "test";
  bipartiteSwapsSpec.subsetContainers() = {{"atn2.08.nc"}};

  solver->addConstraint(bipartiteSwapsSpec);
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());

  solver->solve();
}

/*
 * This test sets up drain config for r1 by setting 0 capacity.
 * Without BipartiteSwapsSpec(), solver can just move h2:r1->r2
 * and fix the broken capacity. But due to swap, it has to do
 * h1:r2->r1 to keep same number of objects exchanged between r1 and r2.
 */
TEST_P(BipartiteSwapsTest, Drain) {
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("host");
  solver->setContainerName("rack");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"r1", {"h2"}},
          {"r2", {"h1"}},
      });
  solver->addObjectDimension(
      "drain",
      std::vector<std::pair<std::string, double>>{{"h2", 1}, {"h1", 0}});
  solver->addContainerDimension(
      "drain", std::map<std::string, double>{{"r1", 0}, {"r2", 1}});

  BipartiteSwapsSpec bipartiteSwapsSpec;
  bipartiteSwapsSpec.name() = "test";
  bipartiteSwapsSpec.subsetContainers() = {{"r1"}};
  solver->addConstraint(bipartiteSwapsSpec);

  CapacitySpec drainSpec;
  drainSpec.scope() = "rack";
  drainSpec.dimension() = "drain";

  Limit limit;
  limit.globalLimit() = 1;

  drainSpec.limit() = limit;

  solver->addConstraint(drainSpec);
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());

  auto solution = solver->solve();
  auto assignment = *solution.assignment();
  EXPECT_EQ("r1", assignment.at("h1"));
  EXPECT_EQ("r2", assignment.at("h2"));
}

TEST_P(BipartiteSwapsTest, Large) {
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("host");
  solver->setContainerName("rack");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"r1", {}},
          {"r2", {"h1", "h2", "h3"}},
          {"r3", {"h4", "h5", "h6"}},
      });
  map<string, double> drain_weights;
  for (const auto i : folly::irange(1, 7)) {
    if (i == 1 || i == 2 || i == 3) {
      drain_weights[fmt::format("h{}", i)] = 0;
      continue;
    }
    drain_weights[fmt::format("h{}", i)] = 1;
  }
  solver->addObjectDimension("drain", drain_weights);
  solver->addContainerDimension(
      "drain",
      std::map<std::string, double>{{"r1", 100}, {"r2", 100}, {"r3", 0}});

  BipartiteSwapsSpec bipartiteSwapsSpec;
  bipartiteSwapsSpec.name() = "test";
  bipartiteSwapsSpec.subsetContainers() = {{"r3"}};
  solver->addConstraint(bipartiteSwapsSpec);

  CapacitySpec drainSpec;

  drainSpec.scope() = "rack";
  drainSpec.dimension() = "drain";

  Limit limit;
  limit.globalLimit() = 100;
  drainSpec.limit() = limit;

  solver->addConstraint(drainSpec);
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());

  auto solution = solver->solve();
  auto assignment = *solution.assignment();
  for (auto forced : {"h1", "h2", "h3"}) {
    EXPECT_EQ("r3", assignment.at(forced));
  }
  for (auto other : {"h4", "h5", "h6"}) {
    EXPECT_EQ("r2", assignment.at(other));
  }
}

TEST_P(BipartiteSwapsTest, SameSide) {
  REBALANCER_SKIP_IF_NO_MIP_SOLVER();
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver->setObjectName("host");
  solver->setContainerName("rack");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"r1", {"h2"}},
          {"r2", {"h1"}},
          {"r3", {"h3"}},
      });
  solver->addObjectDimension(
      "drain", std::map<std::string, double>{{"h2", 1}, {"h1", 0}, {"h3", 0}});
  solver->addContainerDimension(
      "drain", std::map<std::string, double>{{"r1", 0}, {"r2", 1}, {"r3", 10}});

  BipartiteSwapsSpec bipartiteSwapsSpec;
  bipartiteSwapsSpec.name() = "test";
  bipartiteSwapsSpec.subsetContainers() = {{"r1"}, {"r2"}, {"r3"}};

  solver->addConstraint(bipartiteSwapsSpec);

  CapacitySpec drainSpec;
  drainSpec.scope() = "rack";
  drainSpec.dimension() = "drain";

  Limit limit;
  limit.globalLimit() = 1;
  drainSpec.limit() = limit;

  solver->addConstraint(drainSpec);
  solver->addSolver(facebook::algopt::makeAvailableOptimalSolverSpec());

  auto solution = solver->solve();

  /*
  TODO: This test only verifies apply() doesn't return infeasible

  It should also verify there are no same side swaps.
  Please check BipartiteSwaps::inner_lp() for more context on
  why current expression doesn't handle this internally.

  This test can be enabled once bipartite_swaps expression handles this.

  EXPECT_EQ("r1", assignment_ref.at("h2"));
  EXPECT_EQ("r2", assignment_ref.at("h1"));
  */
}
