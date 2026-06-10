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

#include "algopt/rebalancer/interface/ThriftStrategyBuilder.h"

#include "algopt/rebalancer/solver/if/gen-cpp2/packer_types.h"
#include "algopt/rebalancer/solver/moves/MoveTypeFactory.h"

#include <folly/logging/xlog.h>

namespace facebook {
namespace rebalancer {
namespace interface {

StrategyBuilder& ThriftStrategyBuilder::addSolver(
    const LocalSearchSolverSpec& spec) {
  SolverT solver;
  solver.localSearchSolverSpec() = spec;
  strategy.solvers()->push_back(std::move(solver));
  return *this;
}

StrategyBuilder& ThriftStrategyBuilder::addSolver(
    const LocalSearchStageSolverSpec& spec) {
  SolverT solver;
  solver.localSearchStageSolverSpec() = spec;
  strategy.solvers()->push_back(std::move(solver));
  return *this;
}

StrategyBuilder& ThriftStrategyBuilder::addSolver(
    const OptimalSolverSpec& spec) {
  SolverT solver;
  OptimalSolverSpec optimal = spec;
  auto solveTime = spec.solveTime() ? *spec.solveTime() : 0;
  if (solveTime < 0) {
    XLOG(CRITICAL) << "The solve time should not be negative.";
    optimal.solveTime() = std::abs(solveTime);
  }
  solver.optimalSolverSpec() = std::move(optimal);
  strategy.solvers()->push_back(std::move(solver));
  return *this;
}

StrategyBuilder& ThriftStrategyBuilder::addSolver(
    const OptimalSubsetSolverSpec& spec) {
  SolverT solver;
  solver.optimalSubsetSolverSpec() = spec;
  strategy.solvers()->push_back(std::move(solver));
  return *this;
}

StrategyBuilder& ThriftStrategyBuilder::addSolver(
    const RasHybridSolverSpec& spec) {
  SolverT solver;
  solver.rasHybridSolverSpec() = spec;
  strategy.solvers()->push_back(std::move(solver));
  return *this;
}

StrategyT ThriftStrategyBuilder::build() {
  if (strategy.solvers()->empty()) {
    throw std::runtime_error(
        "Expected at least one solver to be added using addSolver()");
  }
  return strategy;
}

} // namespace interface
} // namespace rebalancer
} // namespace facebook
