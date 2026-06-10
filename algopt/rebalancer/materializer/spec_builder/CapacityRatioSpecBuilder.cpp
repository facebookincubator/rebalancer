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

#include "algopt/rebalancer/materializer/spec_builder/CapacityRatioSpecBuilder.h"

#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::materializer {

CapacityRatioSpecBuilder::CapacityRatioSpecBuilder(
    std::shared_ptr<const Universe> universe,
    interface::CapacityRatioSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> CapacityRatioSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  co_return getAggregatedConstraintViolation(
      co_await constraints(expressionBuilder), universe_);
}

folly::coro::Task<std::vector<ConstraintInfo>>
CapacityRatioSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());
  std::vector<ConstraintInfo> result;
  for (auto& [itemName1, ratios] : *spec_.ratios()) {
    for (auto& [itemName2, ratio] : ratios) {
      auto scopeItemId1 = universe_->getScopeItemId(scopeId, itemName1);
      auto scopeItemId2 = universe_->getScopeItemId(scopeId, itemName2);

      auto after1 = co_await expressionBuilder.getAbsoluteUtil(
          UtilMetric::AFTER, dimensionId, scopeId, scopeItemId1);
      auto after2 = co_await expressionBuilder.getAbsoluteUtil(
          UtilMetric::AFTER, dimensionId, scopeId, scopeItemId2);

      // after1 / after2 <= ratio
      // after1 <= ratio * after2
      result.emplace_back(after1 - ratio * after2);
    }
  }

  co_return result;
}

std::string CapacityRatioSpecBuilder::description() const {
  return fmt::format(
      "Limit capacity ratio ({}) on scope {}",
      *spec_.dimension(),
      *spec_.scope());
}

SpecParameters CapacityRatioSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .dimension = *spec_.dimension()};
}

} // namespace facebook::rebalancer::materializer
