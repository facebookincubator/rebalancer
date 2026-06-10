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

#include <folly/container/F14Map.h>

#include <memory>
#include <string>
#include <vector>

namespace facebook::rebalancer::interface::benchmarks {

std::unique_ptr<ProblemSolver> createBaseProblemSolver(
    int numContainers,
    int numObjects,
    bool canExecuteAsync);

folly::F14FastMap<std::string, std::vector<std::string>> createPartitionData(
    int numGroups,
    int numObjects);

folly::F14FastMap<std::string, folly::F14FastMap<std::string, double>>
createDynamicDimensionData(
    int numScopeItems,
    int numObjects,
    bool dimensionBasedOnGroups = false);

} // namespace facebook::rebalancer::interface::benchmarks
