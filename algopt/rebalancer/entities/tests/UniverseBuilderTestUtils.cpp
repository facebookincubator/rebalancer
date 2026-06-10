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

#include "algopt/rebalancer/entities/tests/UniverseBuilderTestUtils.h"

#include <fmt/format.h>

namespace facebook::rebalancer::entities::tests {

UniverseBuilderTestUtils::UniverseBuilderTestUtils(
    const std::string& objectName,
    const std::string& containerName) {
  universeBuilder_.setObjectTypeName(objectName);
  universeBuilder_.setContainerTypeName(containerName);
}

std::shared_ptr<const Universe> UniverseBuilderTestUtils::buildUniverse() {
  if (universe_) {
    throw std::runtime_error(
        "buildUniverse() is only expected to be called once");
  }

  universe_ = folly::coro::blockingWait(universeBuilder_.build());
  return universe_;
}

std::shared_ptr<const Universe> UniverseBuilderTestUtils::getUniversePtr()
    const {
  if (!universe_) {
    throw std::runtime_error(
        "Expected universe to be set. Did you call buildUniverse()?");
  }

  return universe_;
}

const entities::Universe& UniverseBuilderTestUtils::getUniverse() const {
  return *getUniversePtr();
}

ObjectId UniverseBuilderTestUtils::object(const std::string& name) const {
  return universe_ ? universe_->getObjectId(name)
                   : universeBuilder_.getObjects()->getId(name);
}

ContainerId UniverseBuilderTestUtils::container(const std::string& name) const {
  return universe_ ? universe_->getContainerId(name)
                   : universeBuilder_.getContainers()->getId(name);
}

ObjectId UniverseBuilderTestUtils::object(int i) const {
  const auto& objectTypeName = universe_ ? universe_->getObjectTypeName()
                                         : universeBuilder_.getObjectTypeName();
  const auto name = fmt::format("{}{}", objectTypeName, i);
  return object(name);
}

ContainerId UniverseBuilderTestUtils::container(int i) const {
  const auto& containerTypeName = universe_
      ? universe_->getContainerTypeName()
      : universeBuilder_.getContainerTypeName();
  const auto name = fmt::format("{}{}", containerTypeName, i);
  return container(name);
}

ScopeId UniverseBuilderTestUtils::scopeId(const std::string& name) const {
  if (universe_) {
    return universe_->getScopeId(name);
  }
  return universeBuilder_.getScopeId(name);
}

ScopeItemId UniverseBuilderTestUtils::scopeItemId(
    const ScopeId theScopeId,
    const std::string& name) const {
  if (universe_) {
    return universe_->getScopeItemId(theScopeId, name);
  }
  const auto scopeData =
      folly::coro::blockingWait(universeBuilder_.getScope(theScopeId));
  return scopeData->getScopeItemId(name);
}

ScopeItemId UniverseBuilderTestUtils::scopeItemId(
    const std::string& scopeName,
    const std::string& scopeItemName) const {
  return scopeItemId(scopeId(scopeName), scopeItemName);
}

PartitionId UniverseBuilderTestUtils::partitionId(
    const std::string& name) const {
  if (universe_) {
    return universe_->getPartitionId(name);
  }
  return universeBuilder_.getPartitionId(name);
}

DimensionId UniverseBuilderTestUtils::dimensionId(
    const std::string& name) const {
  if (universe_) {
    return universe_->getDimensionId(name);
  }
  return universeBuilder_.getDimensionId(name);
}

GroupId UniverseBuilderTestUtils::groupId(
    PartitionId partition,
    const std::string& name) const {
  if (universe_) {
    return universe_->getGroupId(partition, name);
  }

  const auto partitionData =
      folly::coro::blockingWait(universeBuilder_.getPartition(partition));
  return partitionData->getGroupId(name);
}

GroupId UniverseBuilderTestUtils::groupId(
    const std::string& partitionName,
    const std::string& groupName) const {
  return groupId(partitionId(partitionName), groupName);
}

RoutingConfigId UniverseBuilderTestUtils::routingConfigId(
    const std::string& name) const {
  if (universe_) {
    return universe_->getRoutingConfigId(name);
  }
  return universeBuilder_.getRoutingConfigId(name);
}

folly::coro::Task<void> UniverseBuilderTestUtils::addPartition(
    const std::string& partitionName,
    const Map<std::string, std::vector<std::string>>& groupToObjects) {
  co_await universeBuilder_.addPartition(
      universeBuilder_.makePartitionId(partitionName), groupToObjects);
}

folly::coro::Task<void> UniverseBuilderTestUtils::addRoutingConfig(
    const std::string& name,
    RoutingConfigData routingConfigData) {
  co_await universeBuilder_.addRoutingConfig(
      universeBuilder_.makeRoutingConfigId(name), std::move(routingConfigData));
}

folly::coro::Task<void> UniverseBuilderTestUtils::addScope(
    const std::string& scopeName,
    const Map<std::string, std::vector<std::string>>& scopeItemToContainers) {
  co_await universeBuilder_.addScope(
      universeBuilder_.makeScopeId(scopeName), scopeItemToContainers);
}

folly::coro::Task<void> UniverseBuilderTestUtils::addObjectDimension(
    const std::string& dimensionName,
    ObjectDimensionData objectDimensionData) {
  co_await universeBuilder_.addObjectDimension(
      universeBuilder_.makeObjectDimensionId(dimensionName),
      std::move(objectDimensionData));
}

folly::coro::Task<void> UniverseBuilderTestUtils::addObjectDimension(
    const std::string& dimensionName,
    const Map<std::string, double>& objectNameToValue,
    const double defaultValue) {
  Map<ObjectId, double> objectToValue;
  for (const auto& [objName, weight] : objectNameToValue) {
    objectToValue[object(objName)] = weight;
  }

  const auto numObjects = universeBuilder_.getObjects()->numObjects;
  co_await universeBuilder_.addObjectDimension(
      universeBuilder_.makeObjectDimensionId(dimensionName),
      ObjectDimensionData{
          .dimension = std::make_shared<const ObjectDimension>(
              ObjectIdToDoubleMap(objectToValue, defaultValue, numObjects))});
}

folly::coro::Task<void> UniverseBuilderTestUtils::addObjectDimension(
    const std::string& dimensionName,
    const Map<ObjectId, double>& objectToValue,
    const double defaultValue) {
  const auto numObjects = universeBuilder_.getObjects()->numObjects;
  co_await universeBuilder_.addObjectDimension(
      universeBuilder_.makeObjectDimensionId(dimensionName),
      ObjectDimensionData{
          .dimension = std::make_shared<const ObjectDimension>(
              ObjectIdToDoubleMap(objectToValue, defaultValue, numObjects))});
}

folly::coro::Task<void> UniverseBuilderTestUtils::addObjectDimension(
    const std::string& dimensionName,
    const std::vector<Map<ObjectId, double>>& values,
    const std::vector<double>& defaultValues) {
  if (values.size() != defaultValues.size() || values.empty()) {
    throw std::runtime_error(
        "values and defaultValues must be same nonempty size");
  }
  const auto numObjects = universeBuilder_.getObjects()->numObjects;
  std::vector<ObjectIdToDoubleMap> valueMaps;
  valueMaps.reserve(values.size());
  for (size_t i = 0; i < values.size(); ++i) {
    valueMaps.emplace_back(values[i], defaultValues[i], numObjects);
  }
  co_await universeBuilder_.addObjectDimension(
      universeBuilder_.makeObjectDimensionId(dimensionName),
      ObjectDimensionData{
          .dimension =
              std::make_shared<const ObjectDimension>(std::move(valueMaps))});
}

folly::coro::Task<void> UniverseBuilderTestUtils::addDynamicObjectDimension(
    const std::string& dimensionName,
    const ScopeId theScopeId,
    const Map<std::string, Map<std::string, double>>&
        scopeItemNameToObjectToValue,
    const double defaultValue) {
  const auto scopeData = co_await universeBuilder_.getScope(theScopeId);
  const auto numObjects = universeBuilder_.getObjects()->numObjects;
  Map<ScopeItemId, std::shared_ptr<const ObjectIdToDoubleMap>> scopeItemValues;
  scopeItemValues.reserve(scopeItemNameToObjectToValue.size());
  for (const auto& [scopeItemName, weights] : scopeItemNameToObjectToValue) {
    Map<ObjectId, double> objectIdToValue;
    objectIdToValue.reserve(weights.size());
    for (const auto& [objName, weight] : weights) {
      objectIdToValue[object(objName)] = weight;
    }
    scopeItemValues[scopeData->getScopeItemId(scopeItemName)] =
        std::make_shared<const ObjectIdToDoubleMap>(
            objectIdToValue, defaultValue, numObjects);
  }
  co_await universeBuilder_.addObjectDimension(
      universeBuilder_.makeObjectDimensionId(dimensionName),
      ObjectDimensionData{
          .dimension = std::make_shared<const ObjectDimension>(
              theScopeId,
              std::move(scopeItemValues),
              defaultValue,
              numObjects)});
}

folly::coro::Task<void> UniverseBuilderTestUtils::addDynamicObjectDimension(
    const std::string& dimensionName,
    const ScopeId theScopeId,
    const Map<std::string, std::shared_ptr<const Map<ObjectId, double>>>&
        scopeItemNameToObjectValues,
    const double defaultValue) {
  const auto scopeData = co_await universeBuilder_.getScope(theScopeId);
  const auto numObjects = universeBuilder_.getObjects()->numObjects;
  Map<ScopeItemId, std::shared_ptr<const ObjectIdToDoubleMap>> scopeItemValues;
  scopeItemValues.reserve(scopeItemNameToObjectValues.size());
  for (const auto& [name, mapPtr] : scopeItemNameToObjectValues) {
    scopeItemValues[scopeData->getScopeItemId(name)] =
        std::make_shared<const ObjectIdToDoubleMap>(
            *mapPtr, defaultValue, numObjects);
  }
  co_await universeBuilder_.addObjectDimension(
      universeBuilder_.makeObjectDimensionId(dimensionName),
      ObjectDimensionData{
          .dimension = std::make_shared<const ObjectDimension>(
              theScopeId,
              std::move(scopeItemValues),
              defaultValue,
              numObjects)});
}

folly::coro::Task<void> UniverseBuilderTestUtils::addScopeDimension(
    const std::string& dimensionName,
    ScopeId scopeId,
    const Map<std::string, double>& scopeItemNameToValue,
    double defaultValue) {
  const auto scopeData = co_await universeBuilder_.getScope(scopeId);
  Map<ScopeItemId, double> scopeItemIdToValue;
  for (const auto& [name, value] : scopeItemNameToValue) {
    scopeItemIdToValue[scopeData->getScopeItemId(name)] = value;
  }
  co_await universeBuilder_.addScopeDimension(
      universeBuilder_.makeScopeDimensionId(dimensionName, scopeId),
      scopeId,
      ScopeDimensionData{
          .dimension = std::make_shared<const ScopeDimension>(
              std::move(scopeItemIdToValue), defaultValue)});
}

void UniverseBuilderTestUtils::addDestinationsToExploreOptions(
    const std::string& name,
    interface::DestinationsToExploreOptions options) {
  universeBuilder_.addDestinationsToExploreOptions(name, std::move(options));
}

folly::coro::Task<void> UniverseBuilderTestUtils::addGoal(
    const std::string& name,
    interface::GoalSpecs spec,
    const double weight,
    const int tupleIndex) {
  co_await universeBuilder_.addGoal(
      universeBuilder_.makeGoalId(name),
      GoalData{
          .goal = std::make_unique<Goal>(std::move(spec), weight, tupleIndex)});
}

folly::coro::Task<void> UniverseBuilderTestUtils::addConstraint(
    const std::string& name,
    interface::ConstraintSpecs spec,
    const interface::ConstraintPolicy policy,
    const double invalidCost,
    const double invalidState,
    const int tupleIndex) {
  co_await universeBuilder_.addConstraint(
      universeBuilder_.makeConstraintId(name),
      ConstraintData{
          .constraint = std::make_unique<Constraint>(
              std::move(spec), policy, invalidCost, invalidState, tupleIndex)});
}

} // namespace facebook::rebalancer::entities::tests
