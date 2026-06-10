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

#include "algopt/rebalancer/interface/tests/ProblemSolverTest.h"

#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <folly/executors/CPUThreadPoolExecutor.h>
#include <gtest/gtest.h>

using facebook::algopt::isSolverUnavailable;
using facebook::algopt::solverName;
using facebook::algopt::testSolverPackages;
using namespace facebook::rebalancer::interface;
using namespace folly;
using namespace std;

class ProblemSolverTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, ProblemSolverTest, testThreadCounts());

TEST_P(ProblemSolverTest, Basic) {
  auto executor = make_shared<CPUThreadPoolExecutor>(1);
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  runBasicTest(*solver);
}

TEST_P(ProblemSolverTest, BottomUp) {
  auto executor = make_shared<CPUThreadPoolExecutor>(1);
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  runBasicTest(*solver, false);
}

TEST_P(ProblemSolverTest, Parallel) {
  auto executor = make_shared<CPUThreadPoolExecutor>(1);
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});
  runBasicTest(*solver, false, true);
}

TEST_P(ProblemSolverTest, MakeMoveTypes) {
  // verify if the move type spec helper correctly works. It is sufficient to
  // test on a few (> 1) move types, so this test does not needs to be
  // exhaustive
  auto single = ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec());
  EXPECT_EQ(MoveTypeSpec::Type::singleMoveTypeSpec, single.getType());
  auto swap = ProblemSolver::makeMoveTypeSpec(SwapMoveTypeSpec());
  EXPECT_EQ(MoveTypeSpec::Type::swapMoveTypeSpec, swap.getType());
  auto klSearch = ProblemSolver::makeMoveTypeSpec(KLSearchMoveTypeSpec());
  EXPECT_EQ(MoveTypeSpec::Type::klSearchMoveTypeSpec, klSearch.getType());
  // Move type specification using names will eventually be deprecated
  auto moveTypeName =
      ProblemSolver::makeMoveTypeSpec("SINGLE_RANDOM_STRATIFIED");
  EXPECT_EQ(MoveTypeSpec::Type::moveTypeName, moveTypeName.getType());
}

TEST_P(ProblemSolverTest, MakeCPUThreadPoolExecutor) {
  // Test the static makeCPUThreadPoolExecutor method
  auto executor = ProblemSolver::makeCPUThreadPoolExecutor("TestPool", 4);
  ASSERT_NE(nullptr, executor);
  EXPECT_EQ(4, executor->numThreads());
}

class ProblemSolverOptimalTest
    : public ::testing::TestWithParam<std::tuple<int, OptimalSolverPackage>> {
 protected:
  void SetUp() override {
    const auto [threads, solver] = GetParam();
    if (isSolverUnavailable(solver)) {
      GTEST_SKIP() << solverName(solver) << " solver not available";
    }
  }
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    ProblemSolverOptimalTest,
    ::testing::Combine(testThreadCounts(), testSolverPackages()));

TEST_P(ProblemSolverOptimalTest, Optimal) {
  auto [threads, solver] = GetParam();
  auto problemSolver =
      initializeTestProblemSolver({.executorThreadCount = threads});
  runBasicTest(*problemSolver, true, false, solver);
}
