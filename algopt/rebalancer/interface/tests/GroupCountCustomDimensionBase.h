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
#include "algopt/rebalancer/interface/tests/utils.h"

// Generates a problem of the given dimensions with a single GroupCountSpec
// goal using a custom dimension.
std::unique_ptr<facebook::rebalancer::interface::ProblemSolver> buildProblem(
    int objectCount,
    int containerCount,
    int groupCount,
    bool absoluteLimit,
    int executorThreadCount =
        facebook::rebalancer::interface::defaultExecutorThreadCount);
