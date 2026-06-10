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

#include "algopt/rebalancer/materializer/spec_builder/RoutingLatencySpecBuilder.h"

#include "algopt/rebalancer/interface/thrift/ThriftUtils.h"
#include "algopt/rebalancer/materializer/utils/FilterWrapper.h"
#include "algopt/rebalancer/materializer/utils/LimitWrapper.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"

namespace facebook::rebalancer::materializer {

RoutingLatencySpecBuilder::RoutingLatencySpecBuilder(
    std::shared_ptr<const entities::Universe> universe,
    interface::RoutingLatencySpec spec)
    : SpecBuilder(std::move(universe)), spec_(std::move(spec)) {}

folly::coro::Task<ExprPtr> RoutingLatencySpecBuilder::goalCoro(
    ExpressionBuilder& expressionBuilder) const {
  co_return getAggregatedConstraintViolation(
      co_await constraints(expressionBuilder), universe_);
}

folly::coro::Task<std::vector<ConstraintInfo>>
RoutingLatencySpecBuilder::constraints(
    ExpressionBuilder& expressionBuilder) const {
  auto scopeId = universe_->getScopeId(*spec_.scope());
  auto partitionId = universe_->getPartitionId(*spec_.partition());
  auto routingConfigId =
      universe_->getRoutingConfigId(*spec_.routingConfigName());
  auto& routingConfig = universe_->getRoutingConfig(routingConfigId);
  auto routingConfigScopeId = routingConfig.getScopeId();
  auto routingConfigPartitionId = routingConfig.getPartitionId();

  // ensure that the scope and partition specified in the spec are the same
  // scope and partition as in the routingConfig
  if (scopeId != routingConfigScopeId) {
    throw std::runtime_error(
        fmt::format(
            "RoutingLatencySpec is defined on scope '{}', but routing config is defined on scope '{}'",
            universe_->getEntityName(scopeId),
            universe_->getEntityName(routingConfigScopeId)));
  }
  if (partitionId != routingConfigPartitionId) {
    throw std::runtime_error(
        fmt::format(
            "RoutingLatencySpec is defined on partition '{}', but routing config is defined on partition '{}'",
            universe_->getEntityName(partitionId),
            universe_->getEntityName(routingConfigPartitionId)));
  }

  auto& groupToRoutingRings = routingConfig.getGroupToRoutingRings();
  const LimitWrapper limits(universe_, *spec_.limit(), scopeId, partitionId);
  const GroupFilterWrapper filter(*universe_, *spec_.filter(), partitionId);

  std::vector<ConstraintInfo> components;
  for (auto groupId : filter.getGroupIds()) {
    if (!groupToRoutingRings.contains(groupId)) {
      throw std::runtime_error(
          fmt::format(
              "There is no routing ring defined for group '{}' in routing config '{}'",
              universe_->getEntityName(groupId),
              universe_->getEntityName(routingConfigId)));
    }

    auto groupLatency = expressionBuilder.getGroupRoutingLatencyLookup(
        routingConfigId, groupId, *spec_.latencyMetric());

    // The constraint component for this group is broken if its aggregated group
    // latency exceeds the given limit.
    double limit = limits.getLimit(groupId);
    auto groupConstraint = groupLatency - limit;
    groupConstraint->description = fmt::format(
        "{} latency of group '{}' w.r.t. routing config '{}' <= {}",
        interface::thriftUtils::toString(*spec_.latencyMetric()),
        universe_->getEntityName(groupId),
        *spec_.routingConfigName(),
        limit);

    if (spec_.includeWeightedAvgLatencyMetricIfLimitViolated() &&
        *spec_.latencyMetric()->type() !=
            interface::RoutingLatencyMetric::AVG) {
      // add an extra additive term corresponding to the average latency of the
      // group if the limit is violated; useful to guide local search
      interface::RoutingLatencyMetricInfo metric;
      metric.type() = interface::RoutingLatencyMetric::AVG;
      auto avgGroupLatency = expressionBuilder.getGroupRoutingLatencyLookup(
          routingConfigId, groupId, std::move(metric));
      auto additiveTerm = product(
          step(groupConstraint, universe_),
          *spec_.includeWeightedAvgLatencyMetricIfLimitViolated() *
              avgGroupLatency,
          universe_);

      components.emplace_back(groupConstraint + additiveTerm);
    } else {
      components.emplace_back(groupConstraint);
    }
  }

  co_return components;
}

std::string RoutingLatencySpecBuilder::description() const {
  return fmt::format(
      "{} routing latency on scope '{}', partition '{}', and routing config '{}'",
      interface::thriftUtils::toString(*spec_.latencyMetric()),
      *spec_.scope(),
      *spec_.partition(),
      *spec_.routingConfigName());
}

SpecParameters RoutingLatencySpecBuilder::getSpecInfo() const {
  SpecParameters info;
  info.name = fmt::format(
      "{}_{}",
      *spec_.name(),
      interface::thriftUtils::toString(*spec_.latencyMetric()));
  info.scope = *spec_.scope();
  info.partition = *spec_.partition();
  return info;
}

} // namespace facebook::rebalancer::materializer
