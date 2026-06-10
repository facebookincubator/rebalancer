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
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <fmt/core.h>
#include <folly/container/irange.h>
#include <folly/Random.h>
#include <gtest/gtest.h>

#include <map>
#include <string>
#include <tuple>
#include <vector>

using namespace folly;
using namespace std;
using namespace facebook::rebalancer::interface;
using facebook::algopt::isSolverUnavailable;
using facebook::algopt::solverName;
using facebook::algopt::testSolverPackages;

namespace {

void setUpCommonProblem(
    ProblemSolver& solver,
    map<string, double>& taskCpu,
    map<string, double>& taskStorage) {
  solver.setObjectName("task");
  solver.setContainerName("host");

  // There are 2 hosts and 100 tasks. The initial assignment is random.
  map<string, vector<string>> assignment = {
      {"host0", {}},
      {"host1", {}},
  };
  for (const auto i : folly::irange(100)) {
    const string host = fmt::format("host{}", Random::rand32(0, 2));
    const string task = fmt::format("task{}", i);
    assignment[host].push_back(task);
  }
  solver.setAssignment(assignment);

  // Both hosts have the same amount of cpu, but one has more storage than
  // the other.
  solver.addContainerDimension(
      "cpu", std::map<std::string, double>{{"host0", 1}, {"host1", 1}});
  solver.addContainerDimension(
      "storage_gb",
      std::map<std::string, double>{{"host0", 8000}, {"host1", 2000}});

  // Each task requires 50 GB of storage. Half the tasks are "hot" and require
  // 0.01 cpu, while the other tasks are "cold" and require only 0.005 cpu.
  for (const auto i : folly::irange(100)) {
    const string task = fmt::format("task{}", i);
    taskCpu[task] = i < 50 ? 0.005 : 0.01;
    taskStorage[task] = 50;
  }
  solver.addObjectDimension("cpu", taskCpu);
  solver.addObjectDimension("storage_gb", taskStorage);

  // Hosts can't exceed their capacity.
  CapacitySpec capacitySpec1;
  capacitySpec1.scope() = "host";
  capacitySpec1.dimension() = "cpu";
  solver.addConstraint(capacitySpec1);

  CapacitySpec capacitySpec2;
  capacitySpec2.scope() = "host";
  capacitySpec2.dimension() = "storage_gb";
  solver.addConstraint(capacitySpec2);
}

map<string, double> aggregateByHost(
    const folly::F14FastMap<string, string>& taskToHost,
    const map<string, double>& taskValue) {
  map<string, double> hostValue;
  for (const auto& [task, host] : taskToHost) {
    hostValue[host] += taskValue.at(task);
  }
  return hostValue;
}

} // namespace

class BalanceTwoResourcesTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    solver = initializeTestProblemSolver({.executorThreadCount = GetParam()});
    setUpCommonProblem(*solver, taskCpu, taskStorage);
  }

  std::unique_ptr<ProblemSolver> solver;
  map<string, double> taskCpu;
  map<string, double> taskStorage;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    BalanceTwoResourcesTest,
    testThreadCounts());

TEST_P(BalanceTwoResourcesTest, TwoResourcesStorageFirst) {
  // Create a goal tuple to indicate that balacing storage has strictly higher
  // priority than balancing cpu.
  BalanceSpec balanceSpec1;
  balanceSpec1.scope() = "host";
  balanceSpec1.dimension() = "storage_gb";
  balanceSpec1.formula() = BalanceSpecFormula::IDEAL;
  solver->addGoal(balanceSpec1);

  solver->addGoalBoundary();

  BalanceSpec balanceSpec2;
  balanceSpec2.scope() = "host";
  balanceSpec2.dimension() = "cpu";
  balanceSpec2.formula() = BalanceSpecFormula::IDEAL;
  solver->addGoal(balanceSpec2);

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();
  auto assignment = solution.assignment().value();

  // We expect cpu to not be perfectly balanced, but to be as balanced as
  // possible without hurting storage balance.
  auto hostCpu = aggregateByHost(assignment, taskCpu);
  EXPECT_NEAR(0.55, hostCpu["host0"], 1e-8); // 55% utilization
  EXPECT_NEAR(0.2, hostCpu["host1"], 1e-8); // 20% utilization

  // We expect storage to be perfectly balanced.
  auto hostStorage = aggregateByHost(assignment, taskStorage);
  EXPECT_NEAR(4000, hostStorage["host0"], 1e-8); // 50% utilization
  EXPECT_NEAR(1000, hostStorage["host1"], 1e-8); // 50% utilization
}

TEST_P(BalanceTwoResourcesTest, TwoResourcesCpuFirst) {
  // Create a goal tuple to indicate that balacing cpu has strictly higher
  // priority than balancing storage.
  BalanceSpec balanceSpec1;
  balanceSpec1.scope() = "host";
  balanceSpec1.dimension() = "cpu";
  balanceSpec1.formula() = BalanceSpecFormula::IDEAL;
  solver->addGoal(balanceSpec1);

  solver->addGoalBoundary();

  BalanceSpec balanceSpec2;
  balanceSpec2.scope() = "host";
  balanceSpec2.dimension() = "storage_gb";
  balanceSpec2.formula() = BalanceSpecFormula::IDEAL;
  solver->addGoal(balanceSpec2);

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();
  auto assignment = solution.assignment().value();

  // We expect cpu to be perfectly balanced.
  auto hostCpu = aggregateByHost(assignment, taskCpu);
  EXPECT_NEAR(0.375, hostCpu["host0"], 1e-8); // 37.5% utilization
  EXPECT_NEAR(0.375, hostCpu["host1"], 1e-8); // 37.5% utilization

  // We expect storage to not be perfectly balanced, but to be as balanced as
  // possible without hurting cpu balance.
  auto hostStorage = aggregateByHost(assignment, taskStorage);
  EXPECT_NEAR(3000, hostStorage["host0"], 1e-8); // 37.5% utilization
  EXPECT_NEAR(2000, hostStorage["host1"], 1e-8); // 100% utilization
}

class BalanceTwoResourcesOptimalTest
    : public ::testing::TestWithParam<std::tuple<int, OptimalSolverPackage>> {
 protected:
  void SetUp() override {
    auto [threads, solverPkg] = GetParam();
    if (isSolverUnavailable(solverPkg)) {
      GTEST_SKIP() << solverName(solverPkg) << " solver not available";
    }
    solver = initializeTestProblemSolver({.executorThreadCount = threads});
    setUpCommonProblem(*solver, taskCpu, taskStorage);
  }

  std::unique_ptr<ProblemSolver> solver;
  map<string, double> taskCpu;
  map<string, double> taskStorage;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    BalanceTwoResourcesOptimalTest,
    ::testing::Combine(testThreadCounts(), testSolverPackages()));

TEST_P(BalanceTwoResourcesOptimalTest, TwoResourcesEqualPriority) {
  auto [threads, solverPkg] = GetParam();
  if (solverPkg == OptimalSolverPackage::HIGHS) {
    GTEST_SKIP() << "HiGHS does not support quadratic expressions";
  }
  // Both cpu and storage are competing for good balance with equal weights. We
  // expect the cpu and storage distributions to be somewhere in between the 2
  // extremes shown in the previous 2 examples.
  BalanceSpec balanceSpec1;
  balanceSpec1.scope() = "host";
  balanceSpec1.dimension() = "cpu";
  balanceSpec1.formula() = BalanceSpecFormula::IDEAL;
  solver->addGoal(balanceSpec1);

  BalanceSpec balanceSpec2;
  balanceSpec2.scope() = "host";
  balanceSpec2.dimension() = "storage_gb";
  balanceSpec2.formula() = BalanceSpecFormula::IDEAL;
  solver->addGoal(balanceSpec2);

  OptimalSolverSpec solverSpec;
  solverSpec.solverPackage() = solverPkg;
  solver->addSolver(solverSpec);

  auto solution = solver->solve();
  auto assignment = solution.assignment().value();

  // The cpu balance is not perfect, but it's better than if we make storage a
  // strictly higher priority objective than cpu.
  auto hostCpu = aggregateByHost(assignment, taskCpu);
  EXPECT_NEAR(0.48, hostCpu["host0"], 1e-8); // 48% utilization
  EXPECT_NEAR(0.27, hostCpu["host1"], 1e-8); // 27% utilization

  // The storage balance is not perfect either, but it's better than if we make
  // cpu a strictly higher priority objective than storage.
  auto hostStorage = aggregateByHost(assignment, taskStorage);
  EXPECT_NEAR(3650, hostStorage["host0"], 1e-8); // 45.625% utilization
  EXPECT_NEAR(1350, hostStorage["host1"], 1e-8); // 67.5% utilization
}

TEST_P(BalanceTwoResourcesOptimalTest, TwoResourcesCpuMoreImportant) {
  auto [threads, solverPkg] = GetParam();
  if (solverPkg == OptimalSolverPackage::HIGHS) {
    GTEST_SKIP() << "HiGHS does not support quadratic expressions";
  }
  // Similar to the previous example, but we give cpu balance a higher weight
  // than storage balance. We expect this setup to favor cpu balance slightly
  // more than the previous example.
  BalanceSpec balanceSpec1;
  balanceSpec1.scope() = "host";
  balanceSpec1.dimension() = "cpu";
  balanceSpec1.formula() = BalanceSpecFormula::IDEAL;
  solver->addGoal(balanceSpec1, 0.75);

  BalanceSpec balanceSpec2;
  balanceSpec2.scope() = "host";
  balanceSpec2.dimension() = "storage_gb";
  balanceSpec2.formula() = BalanceSpecFormula::IDEAL;
  solver->addGoal(balanceSpec2, 0.25);

  OptimalSolverSpec solverSpec;
  solverSpec.solverPackage() = solverPkg;
  solver->addSolver(solverSpec);

  auto solution = solver->solve();
  auto assignment = solution.assignment().value();

  // The cpu balance is still not perfect, but it's more balanced than in the
  // previous example.
  auto hostCpu = aggregateByHost(assignment, taskCpu);
  EXPECT_NEAR(0.43, hostCpu["host0"], 1e-8); // 43% utilization
  EXPECT_NEAR(0.32, hostCpu["host1"], 1e-8); // 32% utilization

  // The storage is a bit less balanced than before, because we've shifted the
  // priority from storage to cpu balance by adjusting the weights.
  auto hostStorage = aggregateByHost(assignment, taskStorage);
  EXPECT_NEAR(3400, hostStorage["host0"], 1e-8); // 42.5% utilization
  EXPECT_NEAR(1600, hostStorage["host1"], 1e-8); // 80% utilization
}
