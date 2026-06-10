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

#include "algopt/rebalancer/materializer/spec_builder/MultipleOrCapacitySpecBuilder.h"

#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/materializer/spec_builder/CapacitySpecBuilder.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

#include <folly/String.h>

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

MultipleOrCapacitySpecBuilder::MultipleOrCapacitySpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::MultipleOrCapacitySpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<std::vector<ConstraintInfo>>
MultipleOrCapacitySpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  std::vector<ExprPtr> constraints;
  for (auto& spec : *spec_.capacitySpecs()) {
    const CapacitySpecBuilder builder(universe_, spec);
    constraints.push_back(getAggregatedConstraintViolation(
        co_await builder.constraints(expressionBuilder), universe_));
  }

  // The "or" condition means at least one of capacityConstraints must be
  // satisfied. Given the convention that positive values mean the constraints
  // are broken, this is equivalent to enforcing that the minimum of
  // capacityConstraints is non-positive.
  co_return std::vector<ConstraintInfo>{
      ConstraintInfo(min(constraints, universe_))};
}

folly::coro::Task<ExprPtr> MultipleOrCapacitySpecBuilder::goalCoro(
    ExpressionBuilder& /* expressionBuilder */) const {
  throw std::runtime_error("MultipleOrCapacitySpec not supported as a goal");
}

std::string MultipleOrCapacitySpecBuilder::description() const {
  std::vector<std::string> descriptions;
  for (auto& spec : *spec_.capacitySpecs()) {
    const CapacitySpecBuilder builder(universe_, spec);
    auto description = builder.description();
    descriptions.push_back(description);
  }
  return fmt::format(
      "Multiple capacity specs: {}", folly::join(" OR ", descriptions));
}

SpecParameters MultipleOrCapacitySpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .size = static_cast<int>(spec_.capacitySpecs()->size())};
}

} // namespace facebook::rebalancer::materializer
