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
#include "algopt/lp/detail/generic/impl/GenericVariableImpl.h"
#include "algopt/lp/generic/Expression.h"
#include "algopt/lp/generic/Problem.h"
#include "algopt/lp/generic/Variable.h"

#include <folly/container/F14Map.h>
#include <folly/Random.h>

#include <optional>

namespace facebook {
enum LpInstanceType {
  RANDOM = 0,
  ODD_EVEN = 1,
};

using PartitionedVariableList = folly::F14FastMap<
    algopt::lp::detail::PartitionId,
    std::vector<algopt::lp::Variable>>;

class LpInstanceGenerator {
 public:
  explicit LpInstanceGenerator(
      algopt::lp::Problem& problem,
      LpInstanceType type,
      std::optional<int> randomNumberSeed = {},
      algopt::lp::detail::GenericVariableImpl::Type varType =
          algopt::lp::detail::GenericVariableImpl::Type::BINARY);

  void addHittingSetInstance(
      int numVars,
      int numConstraints,
      int constraintWidth,
      std::optional<algopt::lp::detail::PartitionId> partitionId = {});

  void addComplicatingConstraints(int numVars, double limit);

  algopt::lp::Expression getRandomExpression(
      const std::vector<algopt::lp::Variable>& vars,
      int numSamples);

  algopt::lp::Expression getRandomOddEvenExpression(
      const std::vector<algopt::lp::Variable>& vars,
      int numSamples);

  PartitionedVariableList& getVariables();

 private:
  void genOddEvenHittingSetInstance(
      std::vector<algopt::lp::Variable>& vars,
      int numConstraints,
      int constraintWidth);

  void genRandomHittingSetInstance(
      std::vector<algopt::lp::Variable>& vars,
      int numConstraints,
      int constraintWidth);

  algopt::lp::Problem& problem_;
  LpInstanceType instanceType_;
  algopt::lp::Expression objective_;
  std::function<algopt::lp::Variable(algopt::lp::Problem*, std::string)>
      makeVarFunc_;
  PartitionedVariableList partitionedVars_;
  folly::Random::DefaultGenerator rng_;
  int availableConstraintIdx = 0;
};

} // end namespace facebook
