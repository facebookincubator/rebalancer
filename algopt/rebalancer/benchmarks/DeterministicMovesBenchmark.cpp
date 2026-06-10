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

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer::interface;

static void run(int numContainers, int numObjects) {
  auto solver = initializeTestProblemSolver();

  // set up the problem
  solver->setObjectName("task");
  solver->setContainerName("host");

  std::unordered_map<std::string, std::vector<std::string>> assignment;
  // create numContainers hosts
  for (const auto i : folly::irange(numContainers)) {
    auto host = fmt::format("host{}", i);
    assignment[host] = {};
  }

  // numObjects tasks in host 0 and host 1
  for (const auto i : folly::irange(numObjects)) {
    auto host = fmt::format("host{}", 0);
    auto host1 = fmt::format("host{}", 1);
    assignment[host].push_back(fmt::format("task{}_{}", 0, i));
    assignment[host1].push_back(fmt::format("task{}_{}", 1, i));
  }
  solver->setAssignment(assignment);

  std::map<std::string, double> taskCpu;
  for (const auto i : folly::irange(numObjects)) {
    const std::string task = fmt::format("task{}_{}", 0, i);
    const std::string task1 = fmt::format("task{}_{}", 1, i);
    taskCpu[task] = 1;
    taskCpu[task1] = 1;
  }
  solver->addObjectDimension("cpu", taskCpu);

  BalanceSpec balanceSpec;
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  balanceSpec.formula() = BalanceSpecFormula::SQUARES;

  solver->addGoal(balanceSpec);

  auto searchSolve = LocalSearchSolverSpec();
  searchSolve.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  solver->addSolver(searchSolve);

  auto solution = solver->solve();
}

static void runMultiMoveSet(int numContainers, int numObjects) {
  auto solver = initializeTestProblemSolver();
  // set up the problem
  solver->setObjectName("task");
  solver->setContainerName("host");

  std::unordered_map<std::string, std::vector<std::string>> assignment;
  // create numContainers hosts
  for (const auto i : folly::irange(numContainers)) {
    auto host = fmt::format("host{}", i);
    assignment[host] = {};
  }

  for (const auto i : folly::irange(numObjects)) {
    for (const auto j : folly::irange(numContainers)) {
      auto host = fmt::format("host{}", j);
      assignment[host].push_back(fmt::format("task{}_{}", j, i));
    }
  }

  solver->setAssignment(assignment);

  AssignmentAffinitiesSpec affinitiesSpec;
  affinitiesSpec.scope() = "host";
  std::vector<AssignmentAffinity> affinities;
  for (const auto i : folly::irange(numObjects)) {
    for (const auto j : folly::irange(numContainers)) {
      auto task = fmt::format("task{}_{}", j, i);
      auto host = fmt::format("host{}", (j + 1) % numContainers);
      affinities.push_back(makeAssignmentAffinity(task, host, 1));
    }
    affinitiesSpec.affinities() = affinities;
  }
  solver->addGoal(affinitiesSpec);

  auto searchSolve = LocalSearchSolverSpec();
  searchSolve.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(KLSearchMoveTypeSpec()));
  solver->addSolver(searchSolve);

  auto solution = solver->solve();
}

static void runToFreeBenchmark(int numContainers, int numObjects) {
  auto solver = initializeTestProblemSolver();
  // set up the problem
  solver->setObjectName("task");
  solver->setContainerName("host");

  std::unordered_map<std::string, std::vector<std::string>> assignment;
  // create numContainers hosts
  for (const auto i : folly::irange(numContainers)) {
    auto host = fmt::format("host{}", i);
    assignment[host] = {};
  }

  for (const auto i : folly::irange(numObjects)) {
    auto host = fmt::format("host{}", 0);
    assignment[host].push_back(fmt::format("task{}_{}", 0, i));
  }

  solver->setAssignment(assignment);

  ToFreeSpec toFreeSpec;
  toFreeSpec.containers() = {"host0"};
  solver->addConstraint(toFreeSpec);

  auto searchSolve = LocalSearchSolverSpec();
  searchSolve.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  solver->addSolver(searchSolve);

  auto solution = solver->solve();
}

BENCHMARK(Deterministic_Move_Basic_Benchmark) {
  run(100000, 100);
}

BENCHMARK(DeterminismMoreObjThanContainer) {
  run(4000, 6000);
}

BENCHMARK(DeterminismMultiMoveSet) {
  runMultiMoveSet(500, 50);
}

BENCHMARK(Deterministic_Move_To_Free_Benchmark) {
  runToFreeBenchmark(3, 100000);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
