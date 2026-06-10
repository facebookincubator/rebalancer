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

#include "algopt/rebalancer/entities/tests/UniverseBuilderTestUtils.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/Orchestrator.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"
#include "algopt/rebalancer/solver/tests/ExprProblemCreation.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

namespace rebalancer = facebook::rebalancer;
using namespace facebook::rebalancer;

namespace {

struct BenchmarkUniverse : public entities::tests::UniverseBuilderTestUtils {
  // Build universe with all objects in container0.
  std::shared_ptr<const entities::Universe> build(
      int objectCount,
      int containerCount) {
    entities::Map<std::string, std::vector<std::string>> initialAssignment;
    for (const auto i : folly::irange(containerCount)) {
      initialAssignment[fmt::format("container{}", i)] = {};
    }
    for (const auto i : folly::irange(objectCount)) {
      initialAssignment["container0"].push_back(fmt::format("object{}", i));
    }

    setInitialAssignment(initialAssignment);

    return buildUniverse();
  }
};

inline std::shared_ptr<const entities::Universe> buildUniverse(
    int objectCount,
    int containerCount) {
  BENCHMARK_SUSPEND {
    return BenchmarkUniverse().build(objectCount, containerCount);
  }
  throw std::runtime_error("unreachable");
}

} // namespace

static PackerSet<entities::ContainerId> getDynamicContainers(
    const std::shared_ptr<const entities::Universe>& universe,
    int containerCount) {
  PackerSet<entities::ContainerId> dynamicContainers;
  for (const auto i : folly::irange(containerCount)) {
    dynamicContainers.insert(
        universe->getContainerId(fmt::format("container{}", i)));
  }
  return dynamicContainers;
}

static ExprPtr getSumOfContainersUsingVars(
    const std::shared_ptr<const entities::Universe>& universe,
    int objectCount,
    int containerCount) {
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  std::vector<ExprPtr> containerSum;
  for (const auto i : folly::irange(containerCount)) {
    containerSum.push_back(const_expr(0, universe));
    for (const auto j : folly::irange(objectCount)) {
      containerSum[i] += rebalancer::variable(
          universe->getObjectId(fmt::format("object{}", j)),
          universe->getContainerId(fmt::format("container{}", i)),
          universe,
          assignment);
    }
  }

  auto sumOfAllContainers = rebalancer::const_expr(0, universe);
  for (const auto i : folly::irange(containerCount)) {
    inplace_add(sumOfAllContainers, containerSum.at(i), universe);
  }

  return sumOfAllContainers;
}

static ExprPtr getObjectVector(
    const std::shared_ptr<const entities::Universe>& universe,
    int objectCount,
    int containerIndex) {
  PackerMap<entities::ObjectId, double> vec;
  for (const auto i : folly::irange(objectCount)) {
    // each object has value id * containerIndex
    vec.emplace(
        universe->getObjectId(fmt::format("object{}", i)), i * containerIndex);
  }
  return makeObjectVector(vec, universe);
}

static ExprPtr getSumOfPairsOfContainers(
    const std::shared_ptr<const entities::Universe>& universe,
    int objectCount,
    int containerCount) {
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto sumOfContainers = const_expr(0, universe);
  for (const auto i : folly::irange(containerCount)) {
    auto objectVector = getObjectVector(universe, objectCount, i);
    for (int j = i + 1; j < containerCount; ++j) {
      inplace_add(
          sumOfContainers,
          object_lookup(
              objectVector,
              std::make_shared<PackerSet<entities::ContainerId>>(
                  PackerSet<entities::ContainerId>{
                      universe->getContainerId(fmt::format("container{}", i)),
                      universe->getContainerId(fmt::format("container{}", j))}),
              universe,
              assignment),
          universe);
    }
  }

  return sumOfContainers;
}

static ExprPtr getMaxOfContainers(
    const std::shared_ptr<const entities::Universe>& universe,
    int objectCount,
    int containerCount) {
  const Assignment assignment(universe->getContainers().getInitialAssignment());
  auto maxOfContainers = const_expr(0, universe);
  for (const auto i : folly::irange(containerCount)) {
    auto objectVector = getObjectVector(universe, objectCount, i);
    maxOfContainers = rebalancer::max(
        maxOfContainers,
        object_lookup(
            objectVector,
            std::make_shared<PackerSet<entities::ContainerId>>(
                PackerSet<entities::ContainerId>{
                    universe->getContainerId(fmt::format("container{}", i))}),
            universe,
            assignment),
        universe);
  }

  return maxOfContainers;
}

BENCHMARK(AvoidCopyingDynamicChildren) {
  // This benchmark is to ensure we avoid copying dynamic children when building
  // lp expressions
  folly::BenchmarkSuspender suspend;

  constexpr int containerCount = 100;
  constexpr int objectCount = 100000;

  const auto universe = buildUniverse(objectCount, containerCount);
  const auto dynamicContainers = getDynamicContainers(universe, containerCount);

  auto sumOfAllContainers =
      getSumOfContainersUsingVars(universe, objectCount, containerCount);
  auto problem_ptr = packer::tests::createTestProblem(
      universe, {const_expr(0, universe)}, sumOfAllContainers);
  auto& problem = *problem_ptr;
  suspend.dismiss();

  LpContext context(
      dynamicContainers,
      problem.getDynamicEquivalentSets(dynamicContainers),
      problem.getOrchestrator().getDynamicChildren(dynamicContainers));
  problem.lp_store.reset(
      interface::OptimalSolverPackage::XPRESS, false, context);

  problem.getOrchestrator().buildLp(
      {std::move(sumOfAllContainers)},
      {},
      problem,
      context,
      rebalancer::interface::OptimalSolverSpec());
}

BENCHMARK(ParallelLpBuilding) {
  // This benchmark is to show the usefulness of parallelizing lp building
  folly::BenchmarkSuspender suspend;

  constexpr int containerCount = 100;
  constexpr int objectCount = 1000;

  const auto universe = buildUniverse(objectCount, containerCount);
  const auto dynamicContainers = getDynamicContainers(universe, containerCount);

  auto sumOfAllPairsOfContainers =
      getSumOfPairsOfContainers(universe, objectCount, containerCount);
  auto maxOfAllContainers =
      getMaxOfContainers(universe, objectCount, containerCount);
  auto objectiveExpr =
      std::move(sumOfAllPairsOfContainers) + std::move(maxOfAllContainers);

  auto problemPtr = packer::tests::createTestProblem(
      universe,
      {objectiveExpr},
      const_expr(0, universe),
      /*nonAcceptingContainers=*/{},
      ProblemConfigs{.enableParallelizedLpBuilding = true});
  auto& problem = *problemPtr;
  LpContext context(
      dynamicContainers,
      problem.getDynamicEquivalentSets(dynamicContainers),
      problem.getOrchestrator().getDynamicChildren(dynamicContainers));

  // NOTE: simplify is enabled below because currently parallelizing lp builds
  // is only possible when using GenericProblem (and not when directly building
  // Xpress/Gurobi problem)
  problem.lp_store.reset(
      interface::OptimalSolverPackage::XPRESS, true, context);
  suspend.dismiss();

  problem.getOrchestrator().buildLp(
      {std::move(objectiveExpr)},
      {},
      problem,
      context,
      rebalancer::interface::OptimalSolverSpec());
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
