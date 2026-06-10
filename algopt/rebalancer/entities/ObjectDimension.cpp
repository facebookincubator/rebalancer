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

#include "algopt/rebalancer/entities/ObjectDimension.h"

#include "algopt/rebalancer/entities/ObjectDynamicDimension.h"
#include "algopt/rebalancer/entities/ObjectPartitionRoutingDimension.h"
#include "algopt/rebalancer/entities/ObjectScalarDimension.h"
#include "algopt/rebalancer/entities/ObjectStaticDimension.h"
#include "algopt/rebalancer/entities/Partitions.h"

namespace facebook {
namespace rebalancer {
namespace entities {

ObjectDimension::ObjectDimension(ObjectIdToDoubleMap values) {
  scalarDimensions_.push_back(
      std::make_unique<ObjectStaticDimension>(std::move(values)));
}

ObjectDimension::ObjectDimension(std::vector<ObjectIdToDoubleMap> values) {
  if (values.empty()) {
    throw std::runtime_error(
        "Unexpected call to ObjectDimension(values) when objectValues is empty");
  }
  scalarDimensions_.reserve(values.size());
  for (auto& v : values) {
    scalarDimensions_.push_back(
        std::make_unique<ObjectStaticDimension>(std::move(v)));
  }
}

ObjectDimension::ObjectDimension(
    ScopeId scopeId,
    Map<ScopeItemId, std::shared_ptr<const ObjectIdToDoubleMap>> values,
    double defaultValue,
    std::size_t totalObjects) {
  scalarDimensions_.push_back(
      std::make_unique<ObjectDynamicDimension>(
          scopeId, std::move(values), defaultValue, totalObjects));

  isDynamic_ = true;
}

ObjectDimension::ObjectDimension(
    ScopeId scopeId,
    std::shared_ptr<const Partition> partition,
    const Map<ScopeItemId, std::shared_ptr<const GroupIdToDoubleMap>>& values,
    double defaultValue,
    std::size_t totalObjects,
    PartitionId partitionId) {
  scalarDimensions_.push_back(
      std::make_unique<ObjectDynamicDimension>(
          scopeId,
          std::move(partition),
          values,
          defaultValue,
          totalObjects,
          partitionId));

  isDynamic_ = true;
}

ObjectDimension::ObjectDimension(
    Map<GroupId, double> groupIdToValue,
    double defaultValue,
    PartitionId partitionId,
    RoutingConfigId routingConfigId,
    Map<GroupId, double> groupIdToStaticValue,
    double defaultStaticValue) {
  scalarDimensions_.push_back(
      std::make_unique<ObjectPartitionRoutingDimension>(
          std::move(groupIdToValue),
          defaultValue,
          partitionId,
          routingConfigId,
          std::move(groupIdToStaticValue),
          defaultStaticValue));
}

ObjectDimension::ObjectDimension(
    const thrift::ObjectDimension& objectDimension,
    std::size_t totalObjects,
    const Partitions* partitions) {
  for (const auto& scalarDimension : *objectDimension.scalarDimensions()) {
    std::unique_ptr<const ObjectScalarDimension> dimension;
    switch (scalarDimension.getType()) {
      case thrift::ObjectScalarDimension::Type::objectStaticDimension:
        dimension = std::make_unique<ObjectStaticDimension>(
            *scalarDimension.objectStaticDimension(), totalObjects);
        break;
      case thrift::ObjectScalarDimension::Type::objectDynamicDimension:
        dimension = std::make_unique<ObjectDynamicDimension>(
            *scalarDimension.objectDynamicDimension(),
            totalObjects,
            partitions);
        break;
      case thrift::ObjectScalarDimension::Type::objectPartitionRoutingDimension:
        dimension = std::make_unique<ObjectPartitionRoutingDimension>(
            *scalarDimension.objectPartitionRoutingDimension());
        break;
      case thrift::ObjectScalarDimension::Type::__EMPTY__:
        throw std::runtime_error(
            "thrift::ObjectScalarDimension is expected to be non-empty");
    }
    scalarDimensions_.push_back(std::move(dimension));
  }
  isDynamic_ = *objectDimension.isDynamic();
}

int ObjectDimension::size() const {
  return scalarDimensions_.size();
}

const ObjectScalarDimension& ObjectDimension::at(int index) const {
  return *scalarDimensions_.at(index);
}

const ObjectScalarDimension& ObjectDimension::only() const {
  if (scalarDimensions_.size() != 1) {
    throw std::runtime_error("expected single scalar dimension");
  }
  return *scalarDimensions_.at(0);
}

bool ObjectDimension::isDynamic() const {
  return isDynamic_;
}

bool ObjectDimension::isRoutingConfigBased() const {
  for (auto& scalarDimension : scalarDimensions_) {
    if (scalarDimension->isRoutingConfigBased()) {
      return true;
    }
  }
  return false;
}

bool ObjectDimension::hasNegativeValues() const {
  for (auto& scalarDimension : scalarDimensions_) {
    if (scalarDimension->hasNegativeValues()) {
      return true;
    }
  }
  return false;
}

bool ObjectDimension::hasZeroValuedObjects() const {
  for (auto& scalarDimension : scalarDimensions_) {
    if (scalarDimension->hasZeroValuedObjects()) {
      return true;
    }
  }
  return false;
}

bool ObjectDimension::hasZeroValuedObjects(ScopeItemId scopeItemId) const {
  for (auto& scalarDimension : scalarDimensions_) {
    if (scalarDimension->hasZeroValuedObjects(scopeItemId)) {
      return true;
    }
  }
  return false;
}

std::optional<double> ObjectDimension::getMinimumPositiveValue() const {
  double minimumPositiveValue = std::numeric_limits<double>::infinity();
  for (auto& scalarDimension : scalarDimensions_) {
    auto value = scalarDimension->getMinimumPositiveValue();
    if (value.has_value()) {
      minimumPositiveValue = std::min(minimumPositiveValue, value.value());
    }
  }
  return minimumPositiveValue == std::numeric_limits<double>::infinity()
      ? std::nullopt
      : std::make_optional(minimumPositiveValue);
}

double ObjectDimension::getMaximumValue() const {
  double maximumValue = std::numeric_limits<double>::lowest();
  for (auto& scalarDimension : scalarDimensions_) {
    maximumValue = std::max(maximumValue, scalarDimension->getMaximumValue());
  }
  return maximumValue;
}

thrift::ObjectDimension ObjectDimension::toThrift() const {
  thrift::ObjectDimension objectDimension;
  auto& thriftScalarDimensions = *objectDimension.scalarDimensions();
  thriftScalarDimensions.reserve(scalarDimensions_.size());
  for (const auto& scalarDimension : scalarDimensions_) {
    thriftScalarDimensions.push_back(scalarDimension->toThrift());
  }
  objectDimension.isDynamic() = isDynamic_;
  return objectDimension;
}

} // namespace entities
} // namespace rebalancer
} // namespace facebook
