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

#pragma once

#include "algopt/rebalancer/interface/ProblemSolver.h"

#include <folly/container/irange.h>

#include <cstdint>

namespace facebook {
namespace rebalancer {
namespace interface {
namespace benchmarks {

enum class SolverType {
  LOCAL_SEARCH,
  LOCAL_SEARCH_STAGES,
  LP_BUILD_ONLY,
  LP_SUBSET_BUILD_ONLY,
  MATERIALIZE_ONLY,
};

void addSolver(
    ProblemSolver& solver,
    SolverType solverType,
    bool useParallelizedNewMaterializer = false,
    std::optional<uint32_t> maxSolveTimeSeconds = std::nullopt);

} // namespace benchmarks
} // namespace interface
} // namespace rebalancer
} // namespace facebook
