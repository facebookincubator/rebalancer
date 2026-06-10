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

#include "algopt/rebalancer/entities/ObjectStaticDimension.h"

#include "algopt/rebalancer/entities/ObjectDimensionValueStats.h"

namespace facebook::rebalancer::entities {

namespace {
ObjectIdToDoubleMap fromThrift(
    const thrift::ObjectStaticDimension& objectStaticDimension,
    std::size_t totalObjects) {
  const double defaultValue = *objectStaticDimension.defaultValue();
  ObjectIdToDoubleMap nonDefaultValues(
      totalObjects,
      defaultValue,
      /*expectedNonDefaultSize=*/objectStaticDimension.values()->size());
  for (auto& [objectId, value] : *objectStaticDimension.values()) {
    if (value != defaultValue) {
      nonDefaultValues.emplace(ObjectId(objectId), value);
    }
  }
  return nonDefaultValues;
}
} // namespace

ObjectStaticDimension::ObjectStaticDimension(ObjectIdToDoubleMap values)
    : objectValues_(
          std::make_shared<const ObjectIdToDoubleMap>(std::move(values))) {
  ObjectDimensionValueStats stats;
  stats.observe(objectValues_.defaultValue());
  objectValues_.forEachNonDefault(
      [&](ObjectId, double value) { stats.observe(value); });

  hasNegativeValues_ = stats.hasNegative;
  hasZeroValuedObjects_ = stats.hasZero;
  maximumValue_ = stats.maxValue;
  minimumPositiveValue_ = stats.getMinPositive();
}

ObjectStaticDimension::ObjectStaticDimension(
    const thrift::ObjectStaticDimension& objectStaticDimension,
    std::size_t totalObjects)
    : ObjectStaticDimension(fromThrift(objectStaticDimension, totalObjects)) {}

double ObjectStaticDimension::getValue(ObjectId objectId) const {
  return objectValues_.getObjectValue(objectId);
}

double ObjectStaticDimension::getValue(
    ObjectId objectId,
    std::optional<ScopeItemId> /*scopeItemId*/) const {
  return getValue(objectId);
}

double ObjectStaticDimension::getValueSafe(
    ObjectId objectId,
    std::optional<ScopeScopeItemPair> /*scopeScopeItemPair*/) const {
  return getValue(objectId);
}

const ObjectValues& ObjectStaticDimension::values(
    std::optional<ScopeItemId> /*scopeItemId*/) const {
  return objectValues_;
}

double ObjectStaticDimension::getDefaultValue() const {
  return objectValues_.defaultValue();
}

bool ObjectStaticDimension::isDynamic() const {
  return false;
}

bool ObjectStaticDimension::isRoutingConfigBased() const {
  return false;
}

ScopeId ObjectStaticDimension::getScopeId() const {
  throw std::runtime_error("static dimension is not tied to a scope");
}

bool ObjectStaticDimension::hasNegativeValues() const {
  return hasNegativeValues_;
}

bool ObjectStaticDimension::hasZeroValuedObjects() const {
  return hasZeroValuedObjects_;
}

bool ObjectStaticDimension::hasZeroValuedObjects(
    ScopeItemId /*scopeItemId*/) const {
  return hasZeroValuedObjects_;
}

std::optional<double> ObjectStaticDimension::getMinimumPositiveValue() const {
  return minimumPositiveValue_;
}

double ObjectStaticDimension::getMaximumValue() const {
  return maximumValue_;
}

thrift::ObjectScalarDimension ObjectStaticDimension::toThrift() const {
  thrift::ObjectStaticDimension staticDimension;
  auto& thriftNonDefaultValues = *staticDimension.values();
  thriftNonDefaultValues.reserve(objectValues_.nonDefaultCount());
  objectValues_.forEachNonDefault(
      [&](entities::ObjectId objectId, double value) {
        thriftNonDefaultValues.emplace(objectId.asInt(), value);
      });
  staticDimension.defaultValue() = objectValues_.defaultValue();
  thrift::ObjectScalarDimension scalarDimension;
  scalarDimension.objectStaticDimension() = std::move(staticDimension);
  return scalarDimension;
}

} // namespace facebook::rebalancer::entities
