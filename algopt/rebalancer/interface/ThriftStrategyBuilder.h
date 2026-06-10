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

#include "algopt/rebalancer/interface/StrategyBuilder.h"
#include "algopt/rebalancer/solver/if/gen-cpp2/packer_types.h"

namespace facebook {
namespace rebalancer {
namespace interface {

class ThriftStrategyBuilder : public StrategyBuilder {
 public:
  StrategyBuilder& addSolver(const LocalSearchSolverSpec& spec) override;
  StrategyBuilder& addSolver(const LocalSearchStageSolverSpec& spec) override;
  StrategyBuilder& addSolver(const OptimalSolverSpec& spec) override;
  StrategyBuilder& addSolver(const OptimalSubsetSolverSpec& spec) override;
  StrategyBuilder& addSolver(const RasHybridSolverSpec& spec) override;
  StrategyT build();

 private:
  StrategyT strategy;
};

} // namespace interface
} // namespace rebalancer
} // namespace facebook
