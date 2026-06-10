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

#include "algopt/rebalancer/entities/ObjectDynamicDimension.h"

#include "algopt/rebalancer/entities/ObjectDimensionValueStats.h"
#include "algopt/rebalancer/entities/Partitions.h"

#include <folly/container/MapUtil.h>

namespace facebook {
namespace rebalancer {
namespace entities {

namespace {
std::shared_ptr<const ObjectIdToDoubleMap> makeEmptyValues(
    std::size_t totalObjects,
    double defaultValue) {
  return std::make_shared<const ObjectIdToDoubleMap>(
      totalObjects, defaultValue, /*expectedNonDefaultSize=*/0);
}

const std::shared_ptr<const GroupIdToDoubleMap> kEmptyGroupValues =
    std::make_shared<const GroupIdToDoubleMap>();

} // namespace

ObjectDynamicDimension::ObjectDynamicDimension(
    ScopeId scopeId,
    Map<ScopeItemId, std::shared_ptr<const ObjectIdToDoubleMap>>
        nonDefaultValues,
    double defaultValue,
    std::size_t totalObjects)
    : scopeId_(scopeId),
      emptyObjectValues_(makeEmptyValues(totalObjects, defaultValue)),
      defaultValue_(defaultValue) {
  objectValues_.reserve(nonDefaultValues.size());
  ObjectDimensionValueStats stats;
  stats.observe(defaultValue);
  for (auto& [scopeItemId, objectIdToValuePtr] : nonDefaultValues) {
    bool scopeItemHasZeroValue = false;
    objectIdToValuePtr->forEachNonDefault([&](ObjectId, double value) {
      scopeItemHasZeroValue |= stats.observe(value);
    });
    objectValues_.emplace(
        scopeItemId, ObjectValues(std::move(objectIdToValuePtr)));
    if (scopeItemHasZeroValue) {
      scopeItemToHasZeroValue_.emplace(scopeItemId, true);
    }
  }
  hasNegativeValues_ = stats.hasNegative;
  hasZeroValuedObjects_ = stats.hasZero;
  maximumValue_ = stats.maxValue;
  minimumPositiveValue_ = stats.getMinPositive();
}

ObjectDynamicDimension::ObjectDynamicDimension(
    ScopeId scopeId,
    std::shared_ptr<const Partition> partition,
    const Map<ScopeItemId, std::shared_ptr<const GroupIdToDoubleMap>>&
        nonDefaultGroupValues,
    double defaultValue,
    std::size_t totalObjects,
    PartitionId partitionId)
    : scopeId_(scopeId),
      emptyObjectValues_(
          kEmptyGroupValues,
          partition,
          defaultValue,
          totalObjects,
          partitionId),
      defaultValue_(defaultValue) {
  objectValues_.reserve(nonDefaultGroupValues.size());
  ObjectDimensionValueStats stats;
  stats.observe(defaultValue);
  for (const auto& [scopeItemId, groupValuesPtr] : nonDefaultGroupValues) {
    bool scopeItemHasZeroValue = false;
    // Every object in a group shares the group's value, so the stats need each
    // group value only once -- O(groups), never expanding to objects. Groups
    // that map to no objects contribute no object values, so skip them.
    for (const auto& [groupId, value] : *groupValuesPtr) {
      if (partition->getObjectIds(groupId).empty()) {
        continue;
      }
      scopeItemHasZeroValue |= stats.observe(value);
    }
    objectValues_.emplace(
        scopeItemId,
        ObjectValues(
            groupValuesPtr,
            partition,
            defaultValue,
            totalObjects,
            partitionId));
    if (scopeItemHasZeroValue) {
      scopeItemToHasZeroValue_.emplace(scopeItemId, true);
    }
  }
  hasNegativeValues_ = stats.hasNegative;
  hasZeroValuedObjects_ = stats.hasZero;
  maximumValue_ = stats.maxValue;
  minimumPositiveValue_ = stats.getMinPositive();
}

ObjectDynamicDimension::ObjectDynamicDimension(
    const thrift::ObjectDynamicDimension& objectDynamicDimension,
    std::size_t totalObjects,
    const Partitions* partitions)
    : scopeId_(ScopeId(*objectDynamicDimension.scopeId())),
      emptyObjectValues_(makeEmptyValues(
          totalObjects,
          *objectDynamicDimension.defaultValue())),
      defaultValue_(*objectDynamicDimension.defaultValue()) {
  ObjectDimensionValueStats stats;
  stats.observe(defaultValue_);
  auto addValues = [&](ScopeItemId scopeItemId, const ObjectValues& values) {
    bool scopeItemHasZeroValue = false;
    values.forEachNonDefault([&](ObjectId, double value) {
      scopeItemHasZeroValue |= stats.observe(value);
    });
    objectValues_.emplace(scopeItemId, values);
    if (scopeItemHasZeroValue) {
      scopeItemToHasZeroValue_.emplace(scopeItemId, true);
    }
  };

  const auto& scopedValues = *objectDynamicDimension.scopedValues();
  objectValues_.reserve(
      static_cast<decltype(objectValues_)::size_type>(scopedValues.size()));
  for (const auto& [scopeItemIdInt, thriftValues] : scopedValues) {
    std::shared_ptr<const Partition> partition;
    if (thriftValues.getType() == thrift::ObjectValues::Type::groupValues) {
      if (partitions == nullptr) {
        throw std::runtime_error(
            "Cannot deserialize group-keyed ObjectValues without partitions");
      }
      partition = partitions->getPartitionPtr(
          PartitionId(*thriftValues.groupValues()->partitionId()));
    }
    addValues(
        ScopeItemId(scopeItemIdInt),
        ObjectValues(
            thriftValues, defaultValue_, totalObjects, std::move(partition)));
  }

  hasNegativeValues_ = stats.hasNegative;
  hasZeroValuedObjects_ = stats.hasZero;
  maximumValue_ = stats.maxValue;
  minimumPositiveValue_ = stats.getMinPositive();
}

double ObjectDynamicDimension::getValue(ObjectId /* objectId */) const {
  throw std::runtime_error(
      "values of an object dynamic dimension depend on the scope item");
}

const ObjectValues& ObjectDynamicDimension::values(
    std::optional<ScopeItemId> scopeItemId) const {
  if (!scopeItemId) {
    throw std::runtime_error(
        "values of an object dynamic dimension depend on the scope item");
  }
  return folly::get_ref_default(
      objectValues_, *scopeItemId, emptyObjectValues_);
}

double ObjectDynamicDimension::getValue(
    ObjectId objectId,
    std::optional<ScopeItemId> scopeItemId) const {
  if (!scopeItemId) {
    return defaultValue_;
  }
  return getValueFor(objectId, *scopeItemId);
}

double ObjectDynamicDimension::getValueSafe(
    ObjectId objectId,
    std::optional<ScopeScopeItemPair> scopeScopeItemPair) const {
  if (!scopeScopeItemPair) {
    // The object gets the default value when placed out-of-scope.
    return defaultValue_;
  }
  // Check if scopeItemId is in dimension's scopeId. Throw error if it does not
  // match
  if (scopeScopeItemPair->scopeId != scopeId_) {
    throw std::runtime_error("scopeItemId is out of scope");
  }

  return getValueFor(objectId, scopeScopeItemPair->scopeItemId);
}

double ObjectDynamicDimension::getValueFor(
    ObjectId objectId,
    ScopeItemId scopeItemId) const {
  return folly::get_ref_default(objectValues_, scopeItemId, emptyObjectValues_)
      .getObjectValue(objectId);
}

double ObjectDynamicDimension::getDefaultValue() const {
  return defaultValue_;
}

bool ObjectDynamicDimension::isDynamic() const {
  return true;
}

bool ObjectDynamicDimension::isRoutingConfigBased() const {
  return false;
}

ScopeId ObjectDynamicDimension::getScopeId() const {
  return scopeId_;
}

bool ObjectDynamicDimension::hasNegativeValues() const {
  return hasNegativeValues_;
}

bool ObjectDynamicDimension::hasZeroValuedObjects() const {
  return hasZeroValuedObjects_;
}

bool ObjectDynamicDimension::hasZeroValuedObjects(
    ScopeItemId scopeItemId) const {
  // Check if default value is zero. Note that ideally we should check if all
  // the objects have non-zero values explicitly specified. However,
  // objectDimensions currently do not have access to the number of objects in
  // the problem (T240118355)
  if (defaultValue_ == 0.0) {
    return true;
  }
  return folly::get_default(scopeItemToHasZeroValue_, scopeItemId, false);
}

std::optional<double> ObjectDynamicDimension::getMinimumPositiveValue() const {
  return minimumPositiveValue_;
}

double ObjectDynamicDimension::getMaximumValue() const {
  return maximumValue_;
}

thrift::ObjectScalarDimension ObjectDynamicDimension::toThrift() const {
  thrift::ObjectDynamicDimension dynamicDimension;
  dynamicDimension.scopeId() = scopeId_.asInt();
  folly::F14FastMap<int, thrift::ObjectValues> thriftValues;
  thriftValues.reserve(objectValues_.size());
  for (const auto& [scopeItemId, objValues] : objectValues_) {
    thriftValues.emplace(scopeItemId.asInt(), objValues.toThrift());
  }
  dynamicDimension.scopedValues() = std::move(thriftValues);
  dynamicDimension.defaultValue() = defaultValue_;
  thrift::ObjectScalarDimension scalarDimension;
  scalarDimension.objectDynamicDimension() = std::move(dynamicDimension);
  return scalarDimension;
}

} // namespace entities
} // namespace rebalancer
} // namespace facebook
