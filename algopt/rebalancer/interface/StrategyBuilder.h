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

#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSolver_types.h"

#include <folly/Optional.h>

#include <map>
#include <string>
#include <vector>

namespace facebook {
namespace rebalancer {
namespace interface {

class StrategyBuilder {
 public:
  StrategyBuilder() = default;
  StrategyBuilder(const StrategyBuilder&) = default;
  StrategyBuilder(StrategyBuilder&&) = default;
  StrategyBuilder& operator=(const StrategyBuilder&) = default;
  StrategyBuilder& operator=(StrategyBuilder&&) = default;
  virtual ~StrategyBuilder();
  virtual StrategyBuilder& addSolver(const LocalSearchSolverSpec& spec) = 0;
  virtual StrategyBuilder& addSolver(
      const LocalSearchStageSolverSpec& spec) = 0;
  virtual StrategyBuilder& addSolver(const OptimalSolverSpec& spec) = 0;
  virtual StrategyBuilder& addSolver(const OptimalSubsetSolverSpec& spec) = 0;
  virtual StrategyBuilder& addSolver(const RasHybridSolverSpec& spec) = 0;
};

} // namespace interface
} // namespace rebalancer
} // namespace facebook
