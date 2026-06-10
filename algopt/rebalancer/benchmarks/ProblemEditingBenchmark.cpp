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

#include "algopt/rebalancer/benchmarks/utils/BenchmarkUtils.h"
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/ProblemSolverFactory.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace facebook::rebalancer::interface {

using SolverType = benchmarks::SolverType;

static void addGoals(ProblemSolver& solver, int startId, int endId) {
  for (const auto goalId : folly::irange(startId, endId)) {
    BalanceSpec spec;
    spec.name() = fmt::format("goal_{}", goalId);
    spec.scope() = fmt::format("scope_{}", goalId % 100);
    spec.dimension() = fmt::format("dimension_{}", goalId % 100);
    solver.addGoal(spec);
  }
}

static void addConstraints(ProblemSolver& solver, int startId, int endId) {
  for (const auto constraintId : folly::irange(startId, endId)) {
    CapacitySpec spec;
    spec.name() = fmt::format("constraint_{}", constraintId);
    spec.scope() = fmt::format("scope_{}", constraintId % 100);
    spec.dimension() = fmt::format("dimension_{}", constraintId % 100);
    solver.addConstraint(spec);
  }
}

static void addRoutingConfigs(
    ProblemSolver& solver,
    int startId,
    int endId,
    int containerCount) {
  const int numScopeItems = std::min(containerCount, 5);
  for (const auto configId : folly::irange(startId, endId)) {
    const int scopeId = configId % 100;
    std::unordered_map<std::string, std::unordered_map<std::string, double>>
        latency;
    for (const auto i : folly::irange(numScopeItems)) {
      for (const auto j : folly::irange(numScopeItems)) {
        if (i != j) {
          latency[fmt::format("scope_item_{}_{}", scopeId, i)]
                 [fmt::format("scope_item_{}_{}", scopeId, j)] = (i + j) * 1.0;
        }
      }
    }
    solver.addRoutingConfig(
        fmt::format("routing_config_{}", configId),
        fmt::format("scope_{}", scopeId),
        fmt::format("partition_{}", configId % 100),
        {},
        latency,
        std::nullopt);
  }
}

static void
addPartitions(ProblemSolver& solver, int startId, int endId, int objectCount) {
  for (const auto partitionId : folly::irange(startId, endId)) {
    std::map<std::string, std::string> objectToGroup;
    for (const auto taskId : folly::irange(objectCount)) {
      objectToGroup[fmt::format("task_{}", taskId)] =
          fmt::format("group_{}_{}", partitionId, taskId % 10);
    }
    solver.addPartition(
        fmt::format("partition_{}", partitionId), objectToGroup);
  }
}

static void
addScopes(ProblemSolver& solver, int startId, int endId, int containerCount) {
  for (const auto scopeId : folly::irange(startId, endId)) {
    std::map<std::string, std::string> containerToScopeItem;
    for (const auto hostId : folly::irange(containerCount)) {
      containerToScopeItem[fmt::format("host_{}", hostId)] =
          fmt::format("scope_item_{}_{}", scopeId, hostId % 5);
    }
    solver.addScope(fmt::format("scope_{}", scopeId), containerToScopeItem);
  }
}

static void addObjectDimensions(
    ProblemSolver& solver,
    int startId,
    int endId,
    int objectCount) {
  for (const auto dimensionId : folly::irange(startId, endId)) {
    std::map<std::string, double> dimension;
    for (const auto taskId : folly::irange(objectCount)) {
      dimension[fmt::format("task_{}", taskId)] = taskId / double(objectCount);
    }
    solver.addObjectDimension(
        fmt::format("dimension_{}", dimensionId), dimension);
  }
}

static std::unique_ptr<ProblemSolver> createSolverWithAssignment(
    int objectCount,
    int containerCount) {
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("tasks");
  solver->setContainerName("host");

  std::map<std::string, std::vector<std::string>> assignment;
  for (const auto hostId : folly::irange(containerCount)) {
    assignment[fmt::format("host_{}", hostId)] = {};
  }
  for (const auto taskId : folly::irange(objectCount)) {
    const int hostId = taskId % containerCount;
    assignment[fmt::format("host_{}", hostId)].push_back(
        fmt::format("task_{}", taskId));
  }
  solver->setAssignment(assignment);
  return solver;
}

static void solveFirstHalf(
    ProblemSolver& solver,
    int objectCount,
    int containerCount,
    int halfDimensions,
    int halfGoals,
    int halfConstraints,
    int halfRoutingConfigs,
    int halfPartitions,
    int halfScopes) {
  addScopes(solver, 0, halfScopes, containerCount);
  addPartitions(solver, 0, halfPartitions, objectCount);
  addObjectDimensions(solver, 0, halfDimensions, objectCount);
  addGoals(solver, 0, halfGoals);
  addConstraints(solver, 0, halfConstraints);
  addRoutingConfigs(solver, 0, halfRoutingConfigs, containerCount);

  addSolver(solver, SolverType::MATERIALIZE_ONLY);
  solver.solve();
}

static void runSolveTwiceFromScratch(
    int objectCount,
    int containerCount,
    int dimensionCount,
    int goalCount,
    int constraintCount,
    int routingConfigCount,
    int partitionCount,
    int scopeCount) {
  auto solver = createSolverWithAssignment(objectCount, containerCount);

  solveFirstHalf(
      *solver,
      objectCount,
      containerCount,
      dimensionCount / 2,
      goalCount / 2,
      constraintCount / 2,
      routingConfigCount / 2,
      partitionCount / 2,
      scopeCount / 2);

  solver = createSolverWithAssignment(objectCount, containerCount);

  addScopes(*solver, 0, scopeCount, containerCount);
  addPartitions(*solver, 0, partitionCount, objectCount);
  addObjectDimensions(*solver, 0, dimensionCount, objectCount);
  addGoals(*solver, 0, goalCount);
  addConstraints(*solver, 0, constraintCount);
  addRoutingConfigs(*solver, 0, routingConfigCount, containerCount);

  addSolver(*solver, SolverType::MATERIALIZE_ONLY);
  solver->solve();
}

static void runSolveTwiceWithEditing(
    int objectCount,
    int containerCount,
    int dimensionCount,
    int goalCount,
    int constraintCount,
    int routingConfigCount,
    int partitionCount,
    int scopeCount) {
  auto solver = createSolverWithAssignment(objectCount, containerCount);

  const int halfDimensions = dimensionCount / 2;
  const int halfGoals = goalCount / 2;
  const int halfConstraints = constraintCount / 2;
  const int halfRoutingConfigs = routingConfigCount / 2;
  const int halfPartitions = partitionCount / 2;
  const int halfScopes = scopeCount / 2;

  solveFirstHalf(
      *solver,
      objectCount,
      containerCount,
      halfDimensions,
      halfGoals,
      halfConstraints,
      halfRoutingConfigs,
      halfPartitions,
      halfScopes);

  addScopes(*solver, halfScopes, scopeCount, containerCount);
  addPartitions(*solver, halfPartitions, partitionCount, objectCount);
  addObjectDimensions(*solver, halfDimensions, dimensionCount, objectCount);
  addGoals(*solver, halfGoals, goalCount);
  addConstraints(*solver, halfConstraints, constraintCount);
  addRoutingConfigs(
      *solver, halfRoutingConfigs, routingConfigCount, containerCount);

  solver->solve();
}

constexpr int kObjectCount = 100'000;
constexpr int kContainerCount = 10;
constexpr int kScopeCount = 100;
constexpr int kPartitionCount = 100;
constexpr int kRoutingConfigCount = 100;
constexpr int kDimensionCount = 100;
constexpr int kGoalCount = 100;
constexpr int kConstraintCount = 100;

BENCHMARK(SolveTwiceFromScratch) {
  runSolveTwiceFromScratch(
      kObjectCount,
      kContainerCount,
      kDimensionCount,
      kGoalCount,
      kConstraintCount,
      kRoutingConfigCount,
      kPartitionCount,
      kScopeCount);
}

BENCHMARK(SolveTwiceWithEditing) {
  runSolveTwiceWithEditing(
      kObjectCount,
      kContainerCount,
      kDimensionCount,
      kGoalCount,
      kConstraintCount,
      kRoutingConfigCount,
      kPartitionCount,
      kScopeCount);
}

} // namespace facebook::rebalancer::interface

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
