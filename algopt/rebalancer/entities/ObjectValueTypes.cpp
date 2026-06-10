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

#include "algopt/rebalancer/entities/ObjectValueTypes.h"

namespace facebook::rebalancer::entities {

namespace {
std::shared_ptr<const ObjectIdToDoubleMap> makeObjectValues(
    const folly::F14FastMap<int, double>& thriftObjectValues,
    double defaultValue,
    size_t totalObjects) {
  ObjectIdToDoubleMap objectValues(
      totalObjects,
      defaultValue,
      /*expectedNonDefaultSize=*/thriftObjectValues.size());
  for (const auto& [objectId, value] : thriftObjectValues) {
    if (value != defaultValue) {
      objectValues.emplace(ObjectId(objectId), value);
    }
  }
  return std::make_shared<const ObjectIdToDoubleMap>(std::move(objectValues));
}
} // namespace

ObjectValues::ObjectValues(
    const thrift::ObjectValues& thriftValues,
    double defaultValue,
    size_t totalObjects,
    std::shared_ptr<const Partition> partition) {
  switch (thriftValues.getType()) {
    case thrift::ObjectValues::Type::objectValues:
      backing_ = ObjectBacked{makeObjectValues(
          *thriftValues.objectValues(), defaultValue, totalObjects)};
      return;
    case thrift::ObjectValues::Type::groupValues: {
      if (!partition) {
        throw std::runtime_error(
            "Cannot deserialize group-keyed ObjectValues without partition");
      }

      const auto& thriftGroupValues = *thriftValues.groupValues();
      const auto partitionId = PartitionId(*thriftGroupValues.partitionId());
      auto groupValues = std::make_shared<GroupIdToDoubleMap>();
      groupValues->reserve(
          static_cast<GroupIdToDoubleMap::size_type>(
              thriftGroupValues.values()->size()));
      for (const auto& [groupIdInt, value] : *thriftGroupValues.values()) {
        if (value == defaultValue) {
          continue;
        }
        groupValues->emplace(GroupId(groupIdInt), value);
      }
      if (!partition->isDisjoint()) {
        throw std::invalid_argument(
            "ObjectValues group-backed partition must be disjoint");
      }
      backing_ = GroupBacked{
          .groupValues = std::move(groupValues),
          .partition = std::move(partition),
          .partitionId = partitionId,
          .defaultValue_ = defaultValue,
          .totalObjects = totalObjects};
      return;
    }
    case thrift::ObjectValues::Type::__EMPTY__:
      throw std::runtime_error(
          "thrift::ObjectValues is expected to be non-empty");
  }
  throw std::runtime_error("Unhandled thrift::ObjectValues::Type");
}

thrift::ObjectValues ObjectValues::toThrift() const {
  thrift::ObjectValues thriftValues;
  visit(
      [&](const ObjectIdToDoubleMap& objectValues) {
        thriftValues.objectValues() =
            asIntsMap<folly::F14FastMap<int, double>>(objectValues);
      },
      [&](PartitionId partitionId, const GroupIdToDoubleMap& groupValues) {
        thrift::ObjectGroupValues thriftGroupValues;
        thriftGroupValues.partitionId() = partitionId.asInt();
        thriftGroupValues.values() =
            asIntsMap<folly::F14FastMap<int, double>>(groupValues);
        thriftValues.groupValues() = std::move(thriftGroupValues);
      });
  return thriftValues;
}

} // namespace facebook::rebalancer::entities
