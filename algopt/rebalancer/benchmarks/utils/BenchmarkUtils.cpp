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

#include <folly/Benchmark.h>

#include <cstdint>

namespace facebook {
namespace rebalancer {
namespace interface {
namespace benchmarks {

void addSolver(
    ProblemSolver& solver,
    SolverType solverType,
    bool useParallelizedNewMaterializer,
    std::optional<uint32_t> maxSolveTimeSeconds) {
  if (useParallelizedNewMaterializer) {
    solver.useParallelizedNewMaterializer();
  }

  if (solverType == SolverType::LOCAL_SEARCH) {
    interface::LocalSearchSolverSpec spec;
    spec.moveTypeList()->push_back(
        interface::ProblemSolver::makeMoveTypeSpec(
            interface::SingleMoveTypeSpec()));
    if (maxSolveTimeSeconds.has_value()) {
      spec.solveTime() = maxSolveTimeSeconds.value();
    }
    solver.addSolver(spec);
    return;
  }
  if (solverType == SolverType::LP_BUILD_ONLY) {
    interface::OptimalSolverSpec spec;
    spec.skipMipSolveForTesting() = true;
    solver.addSolver(spec);
    return;
  }
  if (solverType == SolverType::LP_SUBSET_BUILD_ONLY) {
    interface::OptimalSubsetSolverSpec optimalSubsetSolverSpec;
    optimalSubsetSolverSpec.containerChoice() = {{"HOT", 5}, {"COLD", 5}};
    optimalSubsetSolverSpec.maxSubsetRuns() = 10;
    optimalSubsetSolverSpec.maxSubproblemErrors() = 0;

    interface::OptimalSolverSpec optimalSolverSpec;
    optimalSolverSpec.skipMipSolveForTesting() = true;

    optimalSubsetSolverSpec.optimalConfig() = optimalSolverSpec;

    solver.addSolver(optimalSubsetSolverSpec);
    return;
  }
  if (solverType == SolverType::MATERIALIZE_ONLY) {
    interface::LocalSearchSolverSpec spec;
    spec.moveTypeList()->push_back(
        interface::ProblemSolver::makeMoveTypeSpec(
            interface::SingleMoveTypeSpec()));
    spec.stopAfterMoves() = 0;
    solver.addSolver(spec);
    return;
  }
  throw std::runtime_error("unknown solver type");
}

} // namespace benchmarks
} // namespace interface
} // namespace rebalancer
} // namespace facebook
