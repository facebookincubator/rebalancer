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

// Synthetic LP benchmarks for native Xpress operator paths versus PLF/Big-M
// approximations. Run locally:
//   buck2 run @mode/opt
//   //algopt/rebalancer/benchmarks/solver_benchmarks:native_operator_lp_benchmark

#include "algopt/lp/environment/Environment.h"
#include "algopt/rebalancer/entities/tests/UniverseBuilderTestUtils.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/SolverSpecs_types.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/Orchestrator.h"
#include "algopt/rebalancer/solver/solvers/OptimalSolver.h"
#include "algopt/rebalancer/solver/tests/ExprProblemCreation.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"
#include "algopt/rebalancer/solver/utils/Problem.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>
#include <folly/ScopeGuard.h>

#include <cmath>
#include <cstdio>

namespace algopt = facebook::algopt;

namespace facebook::rebalancer {

namespace {

struct BenchmarkUniverse : public entities::tests::UniverseBuilderTestUtils {
  std::shared_ptr<const entities::Universe> build(
      int nObjects,
      int nContainers) {
    entities::Map<std::string, std::vector<std::string>> init;
    for (const auto i : folly::irange(nContainers)) {
      init[fmt::format("container{}", i)] = {};
    }
    for (const auto i : folly::irange(nObjects)) {
      init["container0"].push_back(fmt::format("object{}", i));
    }
    setInitialAssignment(init);
    return buildUniverse();
  }
};

// Returns sum of assignment vars for the first nObjects objects in container k.
ExprPtr containerLoad(
    const std::shared_ptr<const entities::Universe>& uni,
    const Assignment& assignment,
    int containerIdx,
    int nObjects) {
  auto load = const_expr(0, uni);
  const auto contId =
      uni->getContainerId(fmt::format("container{}", containerIdx));
  for (const auto i : folly::irange(nObjects)) {
    const auto objId = uni->getObjectId(fmt::format("object{}", i));
    load += variable(objId, contId, uni, assignment);
  }
  return load;
}

interface::OptimalSolverSpec makeSilentXpressSpec() {
  interface::OptimalSolverSpec spec;
  *spec.suppressLogs() = true;
  *spec.skipInitialAssignmentHint() = true;
  return spec;
}

// Builds the LP model and solves it via OptimalSolver; returns the best
// objective value. The globals controlling native operators must be set before
// calling this — they are read during LP model construction inside solve().
double buildAndSolve(Problem& p) {
  OptimalSolver solver(makeSilentXpressSpec());
  solver.solve(p);
  const auto result = p.lp_store.getLpProblem().getOnlyResult();
  const auto status = *result.status();
  if (status != algopt::lp::thrift::ProblemStatus::OPTIMAL_FOUND &&
      status != algopt::lp::thrift::ProblemStatus::SOLUTION_FOUND) {
    throw std::runtime_error(
        "Solve produced no incumbent; cannot report benchmark objective");
  }
  return static_cast<double>(*result.bestObjective());
}

// ---- Square ----

static constexpr int kN = 100;

// Last-iteration objective values, communicated from the benchmarks to main()
// for deviance reporting. Exposed via reference-returning accessors (rather
// than mutable globals) to satisfy the non-const-global lint while retaining
// process-wide state through the function-local statics.
double& gSquareBaseline() {
  static double value = NAN;
  return value;
}
double& gSquareNative() {
  static double value = NAN;
  return value;
}

BENCHMARK(SquareLpSolve_baseline) {
  std::unique_ptr<Problem> pPtr;
  BENCHMARK_SUSPEND {
    algopt::useXpressNativeQuadratic() = false;
    auto uni = BenchmarkUniverse().build(kN, 2);
    const Assignment assignment(uni->getContainers().getInitialAssignment());
    auto obj = square(containerLoad(uni, assignment, 0, kN), uni);
    pPtr = packer::tests::createTestProblem(uni, {obj}, const_expr(0, uni));
  }
  gSquareBaseline() = buildAndSolve(*pPtr);
}

BENCHMARK(SquareLpSolve_native) {
  std::unique_ptr<Problem> pPtr;
  BENCHMARK_SUSPEND {
    algopt::useXpressNativeQuadratic() = true;
    auto uni = BenchmarkUniverse().build(kN, 2);
    const Assignment assignment(uni->getContainers().getInitialAssignment());
    auto obj = square(containerLoad(uni, assignment, 0, kN), uni);
    pPtr = packer::tests::createTestProblem(uni, {obj}, const_expr(0, uni));
  }
  SCOPE_EXIT {
    algopt::useXpressNativeQuadratic() = false;
  };
  gSquareNative() = buildAndSolve(*pPtr);
}

// ---- Piecewise ----

double& gPwlBaseline() {
  static double value = NAN;
  return value;
}
double& gPwlNative() {
  static double value = NAN;
  return value;
}

// Breakpoints approximating x^2 / (kN*kN) over [0, kN] with 11 points.
static std::vector<std::pair<double, double>> makePwlPoints() {
  std::vector<std::pair<double, double>> pts;
  pts.reserve(11);
  for (const auto i : folly::irange(11)) {
    const double x = i * (kN / 10.0);
    pts.emplace_back(x, x * x);
  }
  return pts;
}

BENCHMARK(PiecewiseLpSolve_baseline) {
  std::unique_ptr<Problem> pPtr;
  BENCHMARK_SUSPEND {
    algopt::useXpressNativePwl() = false;
    auto uni = BenchmarkUniverse().build(kN, 2);
    const Assignment assignment(uni->getContainers().getInitialAssignment());
    auto obj =
        piecewise(makePwlPoints(), containerLoad(uni, assignment, 0, kN), uni);
    pPtr = packer::tests::createTestProblem(uni, {obj}, const_expr(0, uni));
  }
  gPwlBaseline() = buildAndSolve(*pPtr);
}

BENCHMARK(PiecewiseLpSolve_native) {
  std::unique_ptr<Problem> pPtr;
  BENCHMARK_SUSPEND {
    algopt::useXpressNativePwl() = true;
    auto uni = BenchmarkUniverse().build(kN, 2);
    const Assignment assignment(uni->getContainers().getInitialAssignment());
    auto obj =
        piecewise(makePwlPoints(), containerLoad(uni, assignment, 0, kN), uni);
    pPtr = packer::tests::createTestProblem(uni, {obj}, const_expr(0, uni));
  }
  SCOPE_EXIT {
    algopt::useXpressNativePwl() = false;
  };
  gPwlNative() = buildAndSolve(*pPtr);
}

// ---- Max ----

double& gMaxBaseline() {
  static double value = NAN;
  return value;
}
double& gMaxNative() {
  static double value = NAN;
  return value;
}

BENCHMARK(MaxLpSolve_baseline) {
  std::unique_ptr<Problem> pPtr;
  BENCHMARK_SUSPEND {
    algopt::useXpressNativeMax() = false;
    auto uni = BenchmarkUniverse().build(kN, 2);
    const Assignment assignment(uni->getContainers().getInitialAssignment());
    auto obj =
        max(containerLoad(uni, assignment, 0, kN),
            containerLoad(uni, assignment, 1, kN),
            uni);
    pPtr = packer::tests::createTestProblem(uni, {obj}, const_expr(0, uni));
  }
  gMaxBaseline() = buildAndSolve(*pPtr);
}

BENCHMARK(MaxLpSolve_native) {
  std::unique_ptr<Problem> pPtr;
  BENCHMARK_SUSPEND {
    algopt::useXpressNativeMax() = true;
    auto uni = BenchmarkUniverse().build(kN, 2);
    const Assignment assignment(uni->getContainers().getInitialAssignment());
    auto obj =
        max(containerLoad(uni, assignment, 0, kN),
            containerLoad(uni, assignment, 1, kN),
            uni);
    pPtr = packer::tests::createTestProblem(uni, {obj}, const_expr(0, uni));
  }
  SCOPE_EXIT {
    algopt::useXpressNativeMax() = false;
  };
  gMaxNative() = buildAndSolve(*pPtr);
}

// ---- Step ----
//
// Bin-packing formulation: minimize the number of active containers (each
// measured by a step expression) subject to a per-container capacity.
// MIP optimal: ceil(kStepN / kStepCap) = ceil(200/12) = 17 containers.
//
// Timing is roughly equal between Big-M and indicator at this scale. The
// theoretical LP bound with Big-M ≈ 2N is 0.5 (very weak), while the
// indicator tightens this to N/kStepCap ≈ 16.7 (near MIP optimal). However,
// Xpress's presolve closes this LP gap for both formulations before branching,
// so neither generates a large B&B tree. The indicator advantage is real in
// large production models with complex constraint interactions that resist
// presolve — where the tighter per-node LP bounds produce measurably fewer
// B&B nodes. This benchmark exists to verify correctness (zero deviance) and
// provides a realistic problem structure for profiling at larger scale.

static constexpr int kStepN = 200; // objects
static constexpr int kStepK = 20; // number of containers
static constexpr int kStepCap =
    12; // max objects per container; MIP needs ceil(200/12)=17

// Returns the Step benchmark problem: minimize # active containers subject to
// per-container capacity. We use step(load_k) which fires when container k has
// any objects, so the objective equals the number of occupied containers.
std::unique_ptr<Problem> makeStepProblem(
    const std::shared_ptr<const entities::Universe>& uni,
    const Assignment& assignment) {
  auto obj = const_expr(0, uni);
  std::vector<ExprPtr> excesses;
  for (const auto k : folly::irange(kStepK)) {
    auto load = containerLoad(uni, assignment, k, kStepN);
    obj = obj + step(load, uni);
    excesses.push_back(load - kStepCap);
  }
  // Hard constraint: every container load ≤ kStepCap, expressed as
  // max(load_k - kStepCap) ≤ 0.
  auto constraint = max(excesses, uni);
  return packer::tests::createTestProblem(uni, {obj}, constraint);
}

double& gStepBaseline() {
  static double value = NAN;
  return value;
}
double& gStepNative() {
  static double value = NAN;
  return value;
}

BENCHMARK(StepLpSolve_baseline) {
  std::unique_ptr<Problem> pPtr;
  BENCHMARK_SUSPEND {
    algopt::useXpressIndicatorConstraints() = false;
    auto uni = BenchmarkUniverse().build(kStepN, kStepK);
    const Assignment assignment(uni->getContainers().getInitialAssignment());
    pPtr = makeStepProblem(uni, assignment);
  }
  gStepBaseline() = buildAndSolve(*pPtr);
}

BENCHMARK(StepLpSolve_native) {
  std::unique_ptr<Problem> pPtr;
  BENCHMARK_SUSPEND {
    algopt::useXpressIndicatorConstraints() = true;
    auto uni = BenchmarkUniverse().build(kStepN, kStepK);
    const Assignment assignment(uni->getContainers().getInitialAssignment());
    pPtr = makeStepProblem(uni, assignment);
  }
  SCOPE_EXIT {
    algopt::useXpressIndicatorConstraints() = false;
  };
  gStepNative() = buildAndSolve(*pPtr);
}

} // namespace

} // namespace facebook::rebalancer

static void printDeviance(const char* name, double baseline, double native) {
  if (std::isnan(baseline) || std::isnan(native)) {
    std::printf("[%s] not run (filtered out — no deviance to report)\n", name);
    return;
  }
  const double absDev = std::abs(native - baseline);
  const double relDev =
      std::abs(baseline) > 0.0 ? absDev / std::abs(baseline) : 0.0;
  std::printf(
      "[%s] baseline=%.6f native=%.6f abs_dev=%.8f rel_dev=%.6f%%\n",
      name,
      baseline,
      native,
      absDev,
      relDev * 100.0);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  std::puts("\n--- Objective deviance (last iteration of each pair) ---");
  printDeviance(
      "Square   ",
      facebook::rebalancer::gSquareBaseline(),
      facebook::rebalancer::gSquareNative());
  printDeviance(
      "Piecewise",
      facebook::rebalancer::gPwlBaseline(),
      facebook::rebalancer::gPwlNative());
  printDeviance(
      "Max      ",
      facebook::rebalancer::gMaxBaseline(),
      facebook::rebalancer::gMaxNative());
  printDeviance(
      "Step     ",
      facebook::rebalancer::gStepBaseline(),
      facebook::rebalancer::gStepNative());
  return 0;
}
