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

#include "algopt/rebalancer/materializer/spec_builder/LogicalOrSpecBuilder.h"

#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/materializer/spec_builder/GenericSpecBuilder.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <folly/String.h>

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

LogicalOrSpecBuilder::LogicalOrSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::LogicalOrSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<std::vector<ConstraintInfo>>
LogicalOrSpecBuilder::constraints(ExpressionBuilder& expressionBuilder) const {
  std::vector<ExprPtr> constraintExprs;
  for (auto& spec : *spec_.genericSpecs()) {
    const GenericSpecBuilder builder(universe_, spec);
    constraintExprs.push_back(getAggregatedConstraintViolation(
        co_await builder.constraints(expressionBuilder), universe_));
  }

  // The "or" condition means at least one of genericConstraints must be
  // satisfied. Given the convention that positive values mean the constraints
  // are broken, this is equivalent to enforcing that the minimum of
  // genericConstraints is non-positive.
  co_return std::vector<ConstraintInfo>{
      ConstraintInfo(min(constraintExprs, universe_))};
}

folly::coro::Task<ExprPtr> LogicalOrSpecBuilder::goalCoro(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("LogicalOrSpec not supported as a goal");
}

std::string LogicalOrSpecBuilder::description() const {
  std::vector<std::string> descriptions;
  for (auto& spec : *spec_.genericSpecs()) {
    const GenericSpecBuilder builder(universe_, spec);
    auto description = builder.description();
    descriptions.push_back(description);
  }
  return fmt::format(
      "ORed generic specs: {}", folly::join(" OR ", descriptions));
}

SpecParameters LogicalOrSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .size = static_cast<int>(spec_.genericSpecs()->size())};
}

} // namespace facebook::rebalancer::materializer
