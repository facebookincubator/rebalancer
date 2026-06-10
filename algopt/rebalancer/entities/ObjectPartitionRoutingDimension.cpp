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

#include "algopt/rebalancer/entities/ObjectPartitionRoutingDimension.h"

#include "algopt/rebalancer/entities/ObjectDimensionValueStats.h"

#include <folly/container/MapUtil.h>

namespace facebook {
namespace rebalancer {
namespace entities {

ObjectPartitionRoutingDimension::ObjectPartitionRoutingDimension(
    Map<GroupId, double> groupIdToValue,
    double defaultValue,
    PartitionId partitionId,
    RoutingConfigId routingConfigId,
    Map<GroupId, double> groupIdToStaticValue,
    double defaultStaticValue)
    : groupIdToValue_(std::move(groupIdToValue)),
      groupIdToStaticValue_(std::move(groupIdToStaticValue)),
      defaultValue_(defaultValue),
      defaultStaticValue_(defaultStaticValue),
      partitionId_(partitionId),
      routingConfigId_(routingConfigId) {
  ObjectDimensionValueStats stats;
  stats.observe(defaultValue_);
  for (const auto& groupIdAndValue : groupIdToValue_) {
    stats.observe(groupIdAndValue.second);
  }

  hasNegativeValues_ = stats.hasNegative;
  maximumValue_ = stats.maxValue;
  minimumPositiveValue_ = stats.getMinPositive();
}

ObjectPartitionRoutingDimension::ObjectPartitionRoutingDimension(
    const thrift::ObjectPartitionRoutingDimension&
        objectPartitionRoutingDimension)
    : defaultValue_(*objectPartitionRoutingDimension.defaultValue()),
      defaultStaticValue_(
          *objectPartitionRoutingDimension.defaultStaticValue()),
      partitionId_(PartitionId(*objectPartitionRoutingDimension.partitionId())),
      routingConfigId_(
          RoutingConfigId(*objectPartitionRoutingDimension.routingConfigId())) {
  ObjectDimensionValueStats stats;
  groupIdToValue_.reserve(
      static_cast<decltype(groupIdToValue_)::size_type>(
          objectPartitionRoutingDimension.groupIdToValue()->size()));
  for (const auto& [groupId, value] :
       *objectPartitionRoutingDimension.groupIdToValue()) {
    if (value == defaultValue_) {
      continue;
    }
    groupIdToValue_.emplace(GroupId(groupId), value);
    stats.observe(value);
  }
  stats.observe(defaultValue_);
  hasNegativeValues_ = stats.hasNegative;
  maximumValue_ = stats.maxValue;
  minimumPositiveValue_ = stats.getMinPositive();

  groupIdToStaticValue_.reserve(
      static_cast<decltype(groupIdToStaticValue_)::size_type>(
          objectPartitionRoutingDimension.groupIdToStaticValue()->size()));
  for (const auto& [groupId, value] :
       *objectPartitionRoutingDimension.groupIdToStaticValue()) {
    if (value == defaultStaticValue_) {
      continue;
    }
    groupIdToStaticValue_.emplace(GroupId(groupId), value);
  }
}

double ObjectPartitionRoutingDimension::getValue(GroupId groupId) const {
  return folly::get_default(groupIdToValue_, groupId, defaultValue_);
}

double ObjectPartitionRoutingDimension::getStaticValue(GroupId groupId) const {
  return folly::get_default(
      groupIdToStaticValue_, groupId, defaultStaticValue_);
}

RoutingConfigId ObjectPartitionRoutingDimension::getRoutingConfigId() const {
  return routingConfigId_;
}

PartitionId ObjectPartitionRoutingDimension::getPartitionId() const {
  return partitionId_;
}

double ObjectPartitionRoutingDimension::getDefaultValue() const {
  return defaultValue_;
}

bool ObjectPartitionRoutingDimension::isDynamic() const {
  return false;
}

bool ObjectPartitionRoutingDimension::isRoutingConfigBased() const {
  return true;
}

thrift::ObjectScalarDimension ObjectPartitionRoutingDimension::toThrift()
    const {
  thrift::ObjectPartitionRoutingDimension objectPartitionRoutingDimension;
  auto& thriftGroupIdToValue =
      *objectPartitionRoutingDimension.groupIdToValue();
  thriftGroupIdToValue.reserve(groupIdToValue_.size());
  for (const auto& [groupId, value] : groupIdToValue_) {
    thriftGroupIdToValue.emplace(groupId.asInt(), value);
  }
  auto& thriftGroupIdToStaticValue =
      *objectPartitionRoutingDimension.groupIdToStaticValue();
  thriftGroupIdToStaticValue.reserve(groupIdToStaticValue_.size());
  for (const auto& [groupId, value] : groupIdToStaticValue_) {
    thriftGroupIdToStaticValue.emplace(groupId.asInt(), value);
  }
  objectPartitionRoutingDimension.defaultValue() = defaultValue_;
  objectPartitionRoutingDimension.defaultStaticValue() = defaultStaticValue_;
  objectPartitionRoutingDimension.partitionId() = partitionId_.asInt();
  objectPartitionRoutingDimension.routingConfigId() = routingConfigId_.asInt();

  thrift::ObjectScalarDimension scalarDimension;
  scalarDimension.objectPartitionRoutingDimension() =
      std::move(objectPartitionRoutingDimension);
  return scalarDimension;
}

bool ObjectPartitionRoutingDimension::hasNegativeValues() const {
  return hasNegativeValues_;
}

bool ObjectPartitionRoutingDimension::hasZeroValuedObjects(
    ScopeItemId /*scopeItemId*/) const {
  throw std::runtime_error(
      "Unexpected call to hasZeroValuedObjects() w.r.t. ObjectPartitionRoutingDimension");
}

bool ObjectPartitionRoutingDimension::hasZeroValuedObjects() const {
  throw std::runtime_error(
      "Unexpected call to hasZeroValuedObjects() w.r.t. ObjectPartitionRoutingDimension");
}

std::optional<double> ObjectPartitionRoutingDimension::getMinimumPositiveValue()
    const {
  return minimumPositiveValue_;
}

double ObjectPartitionRoutingDimension::getMaximumValue() const {
  return maximumValue_;
}

double ObjectPartitionRoutingDimension::getValue(ObjectId /*unused*/) const {
  throw std::runtime_error(
      "There is no value associated with an ObjectId in an ObjectPartitionRoutingDimension");
}

double ObjectPartitionRoutingDimension::getValue(
    ObjectId /*unused*/,
    std::optional<ScopeItemId> /*unused*/) const {
  throw std::runtime_error(
      "There is no value associated with an ObjectId in an ObjectPartitionRoutingDimension");
}

double ObjectPartitionRoutingDimension::getValueSafe(
    ObjectId /*unused*/,
    std::optional<ScopeScopeItemPair> /*unused*/) const {
  throw std::runtime_error(
      "There is no value associated with an ObjectId in an ObjectPartitionRoutingDimension");
}

const ObjectValues& ObjectPartitionRoutingDimension::values(
    std::optional<ScopeItemId> /*scopeItemId*/) const {
  throw std::runtime_error(
      "There is no value associated with an ObjectId in an ObjectPartitionRoutingDimension");
}

ScopeId ObjectPartitionRoutingDimension::getScopeId() const {
  throw std::runtime_error(
      "There is no scope associated with an ObjectPartitionRoutingDimension");
}

} // namespace entities
} // namespace rebalancer
} // namespace facebook
