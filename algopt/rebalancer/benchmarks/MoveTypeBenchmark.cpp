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

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/ProblemSolverFactory.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

namespace interface = facebook::rebalancer::interface;

static std::map<std::string, std::vector<std::string>>
getInitialContainerToObjects(int objectCount, int containerCount) {
  std::map<std::string, std::vector<std::string>> containerToObjects;

  // initially, all objects are in 'unassigned'
  std::vector<std::string> allObjects;
  for (const auto i : folly::irange(objectCount)) {
    auto objectName = fmt::format("task{}", i);
    allObjects.push_back(objectName);
  }
  containerToObjects.emplace("unassigned", std::move(allObjects));

  for (const auto i : folly::irange(1, containerCount)) {
    auto hostName = fmt::format("host{}", i);
    containerToObjects.emplace(hostName, std::vector<std::string>{});
  }

  return containerToObjects;
}

static void setUpProblem(std::unique_ptr<interface::ProblemSolver>& solver) {
  solver->setObjectName("task");
  solver->setContainerName("host");

  // Use a large number of objects and containers so that many evaluations
  // are required for each SINGLE move. With 100e3 objects and 20 containers,
  // where all the objects are in unassigned container, we expect ~1.9 million
  // evaluations per SINGLE move iteration.
  const int objectCount = 100e3;
  constexpr int containerCount = 20;
  solver->setAssignment(
      getInitialContainerToObjects(objectCount, containerCount));

  {
    // only goal is to drain unassigned container
    interface::ToFreeSpec toFreeSpec;
    toFreeSpec.containers() = {"unassigned"};
    solver->addGoal(std::move(toFreeSpec));
  }

  {
    // add a dummy dimension value and a CapacitySpec that uses this dimension
    // to ensure that the objects are not equivalent
    folly::F14FastMap<std::string, double> objectToDummyDimValue;
    for (const auto i : folly::irange(objectCount)) {
      auto objectName = fmt::format("task{}", i);
      objectToDummyDimValue.emplace(objectName, i);
    }
    solver->addObjectDimension("dummy_dim", std::move(objectToDummyDimValue));

    interface::CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "dummy_dim";
    capacitySpec.limit()->type() = interface::LimitType::ABSOLUTE;
    capacitySpec.limit()->globalLimit() = std::numeric_limits<double>::max();
    solver->addConstraint(std::move(capacitySpec));
  }
}

static void run(const interface::MoveTypeSpec& moveTypeSpec) {
  auto solver =
      interface::ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  setUpProblem(solver);

  // use the given move type
  interface::LocalSearchSolverSpec localSearchSolverSpec;
  localSearchSolverSpec.moveTypeList()->push_back(moveTypeSpec);
  localSearchSolverSpec.stopAfterMoves() = 5;

  solver->addSolver(localSearchSolverSpec);

  auto solution = solver->solve();
}

BENCHMARK(Single_100e3_Objects_20_Containers) {
  // This benchmark is to track if single move evaluations are as fast as
  // expected on a very simple setup that uses a large number (100e3) of objects
  // and 20 containers, which in turn generates ~1.9 million single
  // move evaluations per SINGLE move iteration.
  run(interface::ProblemSolver::makeMoveTypeSpec(
      interface::SingleMoveTypeSpec()));
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
