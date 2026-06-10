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

#include "algopt/rebalancer/materializer/utils/LimitWrapper.h"

#include "algopt/rebalancer/entities/Identifiers.h"

#include <cstdint>
#include <optional>

namespace facebook::rebalancer::materializer {

using namespace facebook::rebalancer::interface;
using namespace facebook::rebalancer::entities;

namespace {

Map<GroupId, double> extractGroupLimits(
    std::shared_ptr<const Universe> universe,
    entities::PartitionId partitionId,
    const Limit& limit) {
  Map<GroupId, double> groupLimits;
  for (auto& [groupName, groupLimit] : *limit.groupLimits()) {
    auto groupId = universe->getGroupId(partitionId, groupName);
    groupLimits.emplace(groupId, groupLimit);
  }
  return groupLimits;
}

} // namespace

LimitWrapper::LimitWrapper(
    std::shared_ptr<const entities::Universe> universe,
    const Limit& limit,
    ScopeId scopeId)
    : universe_(std::move(universe)),
      scopeId_(scopeId),
      type_(*limit.type()),
      globalLimit_(parseGlobalLimit(limit)) {
  parseScopeLimit(limit);
}

LimitWrapper::LimitWrapper(
    std::shared_ptr<const entities::Universe> universe,
    const facebook::rebalancer::interface::Limit& limit,
    ScopeId scopeId,
    PartitionId partitionId)
    : universe_(universe),
      scopeId_(scopeId),
      partitionId_(partitionId),
      type_(*limit.type()),
      globalLimit_(parseGlobalLimit(limit)) {
  parseScopeLimit(limit);
  parseGroupLimit(limit);
  parseScopeGroupLimit(limit);
}

void LimitWrapper::parseScopeLimit(const Limit& limit) {
  for (auto& [scopeItemName, limit_2] : *limit.scopeItemLimits()) {
    auto scopeItemId =
        universe_->getScopeItemId(scopeId_.value(), scopeItemName);
    scopeItemLimits_.emplace(scopeItemId, limit_2);
  }
}

void LimitWrapper::parseGroupLimit(const Limit& limit) {
  groupItemLimits_ = extractGroupLimits(universe_, partitionId_.value(), limit);
}

void LimitWrapper::parseScopeGroupLimit(const Limit& limit) {
  for (auto& [scopeItemName, groupLimits] : *limit.scopeItemToGroupLimits()) {
    auto scopeItemId =
        universe_->getScopeItemId(scopeId_.value(), scopeItemName);
    Map<GroupId, double> groupLimit;
    for (auto& [groupName, limit_2] : groupLimits) {
      groupLimit[universe_->getGroupId(partitionId_.value(), groupName)] =
          limit_2;
    }
    scopeItemToGroupItemLimits_.emplace(scopeItemId, std::move(groupLimit));
  }
}

LimitType LimitWrapper::getType() const {
  return type_;
}

double LimitWrapper::getGlobalLimit() const {
  return globalLimit_;
}

entities::Map<entities::ScopeItemId, uint32_t>
LimitWrapper::checkAndGetPositiveIntegerScopeItemLimits() const {
  entities::Map<entities::ScopeItemId, uint32_t> integralScopeItemLimits;
  const auto& precision = universe_->getPrecision();
  for (auto& [scopeItemId, limit] : scopeItemLimits_) {
    if (precision.isInteger(limit) && precision.isStrictlyGtZero(limit)) {
      integralScopeItemLimits.emplace(
          scopeItemId, static_cast<uint32_t>(limit));
    } else {
      throw std::runtime_error(
          fmt::format(
              "Limit {} for scopeItem {} is not a positive integer",
              limit,
              universe_->getEntityName(scopeItemId)));
    }
  }
  return integralScopeItemLimits;
}

double LimitWrapper::parseGlobalLimit(const Limit& limit) {
  if (*limit.isDefaultLimitUnbounded()) {
    return std::numeric_limits<double>::infinity();
  }
  return *limit.globalLimit();
}

double LimitWrapper::getLimit(ScopeItemId scopeItemId) const {
  return folly::get_default(scopeItemLimits_, scopeItemId, globalLimit_);
}

double LimitWrapper::getLimit(GroupId groupId) const {
  return folly::get_default(groupItemLimits_, groupId, globalLimit_);
}

double LimitWrapper::getLimit(ScopeItemId scopeItemId, GroupId groupId) const {
  // if scopeItem-group override exist then use that
  if (auto limits = folly::get_ptr(scopeItemToGroupItemLimits_, scopeItemId)) {
    if (auto limit = folly::get_ptr(*limits, groupId)) {
      return *limit;
    }
  }
  // if scopeItem override exist then use that
  if (auto limit = folly::get_ptr(scopeItemLimits_, scopeItemId)) {
    return *limit;
  }
  // if groupItem override exist then use that, else return global limit
  return folly::get_default(groupItemLimits_, groupId, globalLimit_);
}

Map<GroupId, double> LimitWrapper::getGroupsOverride(
    ScopeItemId scopeItemId) const {
  if (!partitionId_.has_value()) {
    throw std::runtime_error("partitionId not available");
  }

  if (!scopeItemToGroupItemLimits_.contains(scopeItemId) &&
      !scopeItemLimits_.contains(scopeItemId) && groupItemLimits_.empty()) {
    return {};
  }

  entities::Map<entities::GroupId, double> nonDefaultLimits;
  for (auto groupId : universe_->getPartition(*partitionId_).getGroupIds()) {
    const auto limit = getLimit(scopeItemId, groupId);
    if (limit != globalLimit_) {
      nonDefaultLimits.emplace(groupId, limit);
    }
  }
  return nonDefaultLimits;
}

entities::Map<entities::GroupId, double>
LimitWrapper::getAllGroupLimitsIndptOfScopeItem() const {
  if (!partitionId_.has_value()) {
    throw std::runtime_error("Expected partitionId to be specified");
  }

  entities::Map<entities::GroupId, double> groupLimitsIndptOfScopeItem;
  for (auto groupId :
       universe_->getPartition(partitionId_.value()).getGroupIds()) {
    groupLimitsIndptOfScopeItem[groupId] = getLimit(groupId);
  }

  return groupLimitsIndptOfScopeItem;
}

Map<GroupId, double> LimitWrapper::getAllGroupLimits(
    std::shared_ptr<const Universe> universe,
    entities::PartitionId partitionId,
    const Limit& limit) {
  auto groupLimits = extractGroupLimits(universe, partitionId, limit);
  // use globalLimit for groups where limit was not specified
  for (auto groupId : universe->getPartition(partitionId).getGroupIds()) {
    if (!groupLimits.contains(groupId)) {
      groupLimits.emplace(groupId, parseGlobalLimit(limit));
    }
  }
  return groupLimits;
}

bool LimitWrapper::onlyHasGlobalLimit() const {
  return scopeItemLimits_.empty() && groupItemLimits_.empty() &&
      scopeItemToGroupItemLimits_.empty();
}

} // namespace facebook::rebalancer::materializer
