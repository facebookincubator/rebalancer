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

#include "algopt/rebalancer/materializer/spec_builder/CapacityWithSupplyAndDrSpecBuilder.h"

#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/materializer/utils/LimitWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include <algopt/rebalancer/entities/Set.h>

namespace facebook::rebalancer::materializer {

CapacityWithSupplyAndDrSpecBuilder::CapacityWithSupplyAndDrSpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    facebook::rebalancer::interface::CapacityWithSupplyAndDrSpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> CapacityWithSupplyAndDrSpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  co_return getAggregatedConstraintViolation(
      co_await constraints(expressionBuilder), universe_);
}

folly::coro::Task<std::vector<ConstraintInfo>>
CapacityWithSupplyAndDrSpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  std::vector<ConstraintInfo> result;
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto dimensionId = universe_->getDimensionId(*spec_.dimension());
  auto prodScopeId = universe_->getScopeId(*spec_.prodScope());
  auto prodScopeItemId =
      universe_->getScopeItemId(prodScopeId, *spec_.prodItem());
  auto partitionId = universe_->getPartitionId(*spec_.partitionName());
  auto supplyPartitionId = universe_->getPartitionId(*spec_.supplyPartition());
  double defaultRatio = *spec_.ratio();

  const ScopeItemFilterWrapper filter(*universe_, *spec_.filter(), scopeId);
  const LimitWrapper limits(universe_, *spec_.limit(), scopeId, partitionId);

  auto& partition = universe_->getPartition(partitionId);
  auto& supplyPartition = universe_->getPartition(supplyPartitionId);
  auto& objectDimension =
      universe_->getObjects().getDimension(dimensionId).at(0);

  auto& dependencies = *spec_.dependencies();
  auto& drPairs = *spec_.drPairs();
  auto& exceptions = *spec_.exceptions();

  entities::Set<std::string> supplyPartitionGroupNames;
  for (auto groupId : supplyPartition.getGroupIds()) {
    supplyPartitionGroupNames.insert(universe_->getEntityName(groupId));
  }

  for (auto groupId : partition.getGroupIds()) {
    auto& groupName = universe_->getEntityName(groupId);
    auto& objectIds = partition.getObjectIds(groupId);

    ExprPtr supplyCap;
    if (supplyPartitionGroupNames.contains(groupName)) {
      auto supplyGroupId = universe_->getGroupId(supplyPartitionId, groupName);
      supplyCap = co_await expressionBuilder.getAbsoluteUtil(
          UtilMetric::AFTER,
          dimensionId,
          prodScopeId,
          prodScopeItemId,
          supplyPartitionId,
          supplyGroupId);
    } else {
      supplyCap = const_expr(0, universe_);
    }

    double limit = limits.getLimit(groupId);
    auto usage = const_expr(0, universe_);

    // Regular usage.
    for (auto objectId : objectIds) {
      auto isAssigned =
          expressionBuilder.isAssigned(prodScopeId, prodScopeItemId, objectId);
      const double weight = objectDimension.getValue(objectId);
      if (weight != 0) {
        auto objectUsage = const_expr(0, universe_);
        objectUsage += isAssigned;

        auto& objectName = universe_->getEntityName(objectId);
        auto dependentObjectNames = folly::get_ptr(dependencies, objectName);

        if (dependentObjectNames) {
          for (auto& dependendObjectName : *dependentObjectNames) {
            auto dependentObjectId =
                universe_->getObjectId(dependendObjectName);
            objectUsage += expressionBuilder.isAssigned(
                prodScopeId, prodScopeItemId, dependentObjectId);
          }
        }

        usage += weight * objectUsage;
      }
    }

    const entities::Set<entities::ObjectId> objectIdsSet(
        objectIds.begin(), objectIds.end());

    // DR usage.
    auto drUsage = const_expr(0, universe_);
    for (auto scopeItemId : filter.getScopeItemIds()) {
      // DR usage assuming scopeItemId is down.
      auto singleDrUsage = const_expr(0, universe_);
      for (auto& [drObjectName, regularObjectName] : drPairs) {
        auto drObjectId = universe_->getObjectId(drObjectName);
        auto regularObjectId = universe_->getObjectId(regularObjectName);
        if (!objectIdsSet.contains(regularObjectId)) {
          continue;
        }

        const double drWeight = objectDimension.getValue(drObjectId);
        if (drWeight != 0) {
          auto regularAssigned = expressionBuilder.isAssigned(
              scopeId, scopeItemId, regularObjectId);
          auto drAssigned = expressionBuilder.isAssigned(
              prodScopeId, prodScopeItemId, drObjectId);
          // If the regular object is assigned to scopeItemId and the DR
          // object is assigned to prodScopeItemId, then the DR object
          // contributes to the usage of prodScopeItemId when scopeItemId is
          // down.
          auto bothAssigned =
              expressionBuilder.binaryMin(regularAssigned, drAssigned);
          singleDrUsage += drWeight * bothAssigned;
        }

        auto& scpoeItemName = universe_->getEntityName(scopeItemId);
        singleDrUsage->description =
            fmt::format("assuming {} down", scpoeItemName);
        inplace_max(drUsage, singleDrUsage, universe_);
      }
    }
    usage += drUsage;

    double ratio = folly::get_default(exceptions, groupName, defaultRatio);

    auto expr = usage - (limit + supplyCap) * ratio;
    expr->description =
        fmt::format("during time {} <= {} * {}", groupName, limit, ratio);
    result.emplace_back(expr);
  }

  co_return result;
}

std::string CapacityWithSupplyAndDrSpecBuilder::description() const {
  return fmt::format(
      "namespace usage({}) on {} {}",
      *spec_.dimension(),
      *spec_.prodScope(),
      *spec_.prodItem());
}

SpecParameters CapacityWithSupplyAndDrSpecBuilder::getSpecInfo() const {
  return SpecParameters{
      .name = *spec_.name(),
      .scope = *spec_.scope(),
      .partition = *spec_.partitionName(),
      .dimension = *spec_.dimension(),
      .limitType = apache::thrift::util::enumNameSafe(*spec_.limit()->type()),
      .filterAllowListSize = spec_.filter()->itemsWhitelist()
          ? static_cast<int>(spec_.filter()->itemsWhitelist()->size())
          : 0,
      .filterBlockListSize = spec_.filter()->itemsBlacklist()
          ? static_cast<int>(spec_.filter()->itemsBlacklist()->size())
          : 0};
}

} // namespace facebook::rebalancer::materializer
