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

#include "algopt/rebalancer/solver/expressions/PropertiesHelper.h"

#include "algopt/rebalancer/entities/ObjectValueTypes.h"

#include <map>

namespace facebook::rebalancer {
namespace {

template <typename ObjectValuesLike>
ExpressionPropertyValue makeObjectIdDoubleMapValueImpl(
    const ObjectValuesLike& value) {
  ExpressionPropertyValue property;
  ExpressionPropertyValueObjectIdDoubleMap valueObjectIdDoubleMap;
  std::map<int, double> intToValue;
  value.forEachNonDefault([&](entities::ObjectId objectId, double objectValue) {
    intToValue.emplace(objectId.asInt(), objectValue);
  });
  valueObjectIdDoubleMap.value() = std::move(intToValue);

  property.valueObjectIdDoubleMap() = std::move(valueObjectIdDoubleMap);
  return property;
}

} // namespace

ExpressionPropertyValue PropertiesHelper::makeDoubleValue(double value) {
  ExpressionPropertyValue property;
  ExpressionPropertyValueDouble valueDouble;
  valueDouble.value() = value;
  property.valueDouble() = std::move(valueDouble);
  return property;
}

ExpressionPropertyValue PropertiesHelper::makeStringValue(std::string value) {
  ExpressionPropertyValue property;
  ExpressionPropertyValueString valueString;
  valueString.value() = std::move(value);
  property.valueString() = std::move(valueString);
  return property;
}

ExpressionPropertyValue PropertiesHelper::makeContainerIdListValue(
    PackerSet<entities::ContainerId> value) {
  ExpressionPropertyValue property;
  ExpressionPropertyValueContainerIdList valueContainerIdList;
  valueContainerIdList.value() = entities::asIntsVec(value);
  property.valueContainerIdList() = std::move(valueContainerIdList);
  return property;
}

ExpressionPropertyValue PropertiesHelper::makeIntValue(int value) {
  ExpressionPropertyValue property;
  ExpressionPropertyValueInt valueInt;
  valueInt.value() = value;
  property.valueInt() = std::move(valueInt);
  return property;
}

ExpressionPropertyValue PropertiesHelper::makeBoolValue(bool value) {
  ExpressionPropertyValue property;
  ExpressionPropertyValueBool valueBool;
  valueBool.value() = value;
  property.valueBool() = std::move(valueBool);
  return property;
}

ExpressionPropertyValue PropertiesHelper::makeObjectIdDoubleMapValue(
    const entities::ObjectIdToDoubleMap& value) {
  return makeObjectIdDoubleMapValueImpl(value);
}

ExpressionPropertyValue PropertiesHelper::makeObjectIdDoubleMapValue(
    const entities::ObjectValues& value) {
  return makeObjectIdDoubleMapValueImpl(value);
}

ExpressionPropertyValue PropertiesHelper::makePoint2dListValue(
    std::vector<std::pair<double, double>> value) {
  ExpressionPropertyValue property;
  ExpressionPropertyValuePoint2dList valuePoint2dList;
  for (auto& [x, y] : value) {
    Point2d point;
    point.x() = x;
    point.y() = y;
    valuePoint2dList.value()->push_back(std::move(point));
  }
  property.valuePoint2dList() = std::move(valuePoint2dList);
  return property;
}

ExpressionPropertyValue PropertiesHelper::makeObjectIdValue(
    entities::ObjectId value) {
  ExpressionPropertyValue property;
  ExpressionPropertyValueObjectId valueObjectId;
  valueObjectId.value() = value.asInt();
  property.valueObjectId() = std::move(valueObjectId);
  return property;
}

ExpressionPropertyValue PropertiesHelper::makeContainerIdValue(
    entities::ContainerId value) {
  ExpressionPropertyValue property;
  ExpressionPropertyValueContainerId valueContainerId;
  valueContainerId.value() = value.asInt();
  property.valueContainerId() = std::move(valueContainerId);
  return property;
}

} // namespace facebook::rebalancer
