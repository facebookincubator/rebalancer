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

#include "algopt/rebalancer/benchmarks/utils/LpInstanceGenerator.h"

#include "algopt/lp/detail/generic/impl/GenericVariableImpl.h"
#include "algopt/lp/generic/Expression.h"
#include "algopt/lp/generic/Operators.h"
#include "algopt/lp/generic/Variable.h"

#include <fmt/core.h>
#include <folly/container/irange.h>
#include <folly/logging/xlog.h>
#include <folly/Random.h>

#include <algorithm>
#include <tuple>
#include <vector>

namespace facebook {

using namespace facebook::algopt::lp;
using namespace facebook::algopt::lp::detail;

LpInstanceGenerator::LpInstanceGenerator(
    algopt::lp::Problem& problem,
    LpInstanceType type,
    std::optional<int> randomNumberSeed,
    GenericVariableImpl::Type varType)
    : problem_(problem), instanceType_(type), objective_(0) {
  // create random number generator with specified seed if provides
  // useful to ensure reproducibility in tests
  rng_ = randomNumberSeed ? folly::Random::DefaultGenerator(*randomNumberSeed)
                          : folly::Random::create();
  if (varType == GenericVariableImpl::Type::BINARY) {
    makeVarFunc_ = &Problem::makeBoolVar;
  } else if (varType == GenericVariableImpl::Type::INTEGER) {
    makeVarFunc_ = &Problem::makeIntVar;
  } else {
    makeVarFunc_ = &Problem::makeVar;
  }
}

void LpInstanceGenerator::addHittingSetInstance(
    int numVars,
    int numConstraints,
    int constraintWidth,
    std::optional<PartitionId> partitionId) {
  std::vector<Variable> vars;
  for (const auto idx : folly::irange(numVars)) {
    const std::string varName = partitionId
        ? fmt::format("var_{}_{}", idx, partitionId->asInt())
        : fmt::format("var_{}", idx);
    vars.push_back(makeVarFunc_(&problem_, varName));
    auto& var = vars.at(idx);
    if (partitionId) {
      var.setPartition(*partitionId);
      partitionedVars_[*partitionId].push_back(var);
    } else {
      auto specialPartition = PartitionId(0);
      partitionedVars_[specialPartition].push_back(vars.at(idx));
    }
    objective_ += vars.at(idx);
  }
  problem_.setObjective(objective_);
  if (instanceType_ == RANDOM) {
    genRandomHittingSetInstance(vars, numConstraints, constraintWidth);
  } else if (instanceType_ == ODD_EVEN) {
    genOddEvenHittingSetInstance(vars, numConstraints, constraintWidth);
  }
}

Expression LpInstanceGenerator::getRandomExpression(
    const std::vector<Variable>& vars,
    int numSamples) {
  Expression expr(0);
  for (const auto _ : folly::irange(numSamples)) {
    const int varIdx = folly::Random::rand32(vars.size(), rng_);
    expr += vars.at(varIdx);
  }
  return expr;
}

Expression LpInstanceGenerator::getRandomOddEvenExpression(
    const std::vector<Variable>& vars,
    int numSamples) {
  Expression expr(0);
  for (const auto _ : folly::irange(numSamples / 2)) {
    const int varIdx = folly::Random::rand32(vars.size(), rng_);
    expr += vars.at(varIdx);
    expr += vars.at((varIdx + 1) % vars.size());
  }
  return expr;
}

void LpInstanceGenerator::genOddEvenHittingSetInstance(
    std::vector<Variable>& vars,
    int numConstraints,
    int constraintWidth) {
  auto width = std::min((int)vars.size(), constraintWidth);
  for (const auto _ : folly::irange(numConstraints)) {
    problem_.newConstraint(
        getRandomOddEvenExpression(vars, width) >= 1,
        fmt::format("ctr_{}", availableConstraintIdx++));
  }
}

void LpInstanceGenerator::genRandomHittingSetInstance(
    std::vector<Variable>& vars,
    int numConstraints,
    int constraintWidth) {
  auto width = std::min((int)vars.size(), constraintWidth);
  for (const auto _ : folly::irange(numConstraints)) {
    problem_.newConstraint(
        getRandomExpression(vars, width) >= 1,
        fmt::format("ctr_{}", availableConstraintIdx++));
  }
}

void LpInstanceGenerator::addComplicatingConstraints(
    int numVars,
    double limit) {
  std::vector<Expression> constraints(numVars);
  for (const auto& [id, vars] : partitionedVars_) {
    numVars = std::min((int)vars.size(), numVars);
    for (const auto idx : folly::irange(numVars)) {
      constraints.at(idx) += vars.at(idx);
    }
  }
  for (auto& expr : constraints) {
    problem_.newConstraint(
        expr <= limit,
        fmt::format("complicating_ctr_{}", availableConstraintIdx++));
  }
  XLOG(INFO) << fmt::format(
      "Added {} complicating constraints\n", constraints.size());
}

PartitionedVariableList& LpInstanceGenerator::getVariables() {
  return partitionedVars_;
}

} // namespace facebook
