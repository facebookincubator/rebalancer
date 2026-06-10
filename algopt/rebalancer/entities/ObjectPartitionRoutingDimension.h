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

#pragma once

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/ObjectScalarDimension.h"

namespace facebook {
namespace rebalancer {
namespace entities {

class ObjectPartitionRoutingDimension : public ObjectScalarDimension {
 public:
  ObjectPartitionRoutingDimension(
      Map<GroupId, double> groupIdToValue,
      double defaultValue,
      PartitionId partitionId,
      RoutingConfigId routingConfigId,
      Map<GroupId, double> groupIdToStaticValue,
      double defaultStaticValue);

  explicit ObjectPartitionRoutingDimension(
      const thrift::ObjectPartitionRoutingDimension&
          objectPartitionRoutingDimension);

  // Delete the copy and assignment constructors to prevent accidental copies.
  ObjectPartitionRoutingDimension(const ObjectPartitionRoutingDimension&) =
      delete;
  ObjectPartitionRoutingDimension& operator=(
      const ObjectPartitionRoutingDimension&) = delete;
  // Other operators are as usual.
  ObjectPartitionRoutingDimension(ObjectPartitionRoutingDimension&&) = delete;
  ObjectPartitionRoutingDimension& operator=(
      ObjectPartitionRoutingDimension&&) = delete;
  ~ObjectPartitionRoutingDimension() override = default;

  double getValue(GroupId groupId) const;

  double getStaticValue(GroupId groupId) const;

  RoutingConfigId getRoutingConfigId() const;

  PartitionId getPartitionId() const;

  bool isDynamic() const override;

  bool isRoutingConfigBased() const override;

  bool hasNegativeValues() const override;

  bool hasZeroValuedObjects() const override;
  bool hasZeroValuedObjects(ScopeItemId scopeItemId) const override;

  std::optional<double> getMinimumPositiveValue() const override;
  double getMaximumValue() const override;

  thrift::ObjectScalarDimension toThrift() const override;

  // unsupported getters
  const ObjectValues& values(
      std::optional<ScopeItemId> scopeItemId = std::nullopt) const override;
  double getValue(ObjectId objectId) const override;
  double getValue(ObjectId objectId, std::optional<ScopeItemId> scopeItemId)
      const override;
  double getValueSafe(
      ObjectId objectId,
      std::optional<ScopeScopeItemPair> scopeScopeItemPair) const override;
  double getDefaultValue() const override;
  ScopeId getScopeId() const override;

 private:
  // TODO (T180294479): remove this once we prevent negative dimension values
  // using ProblemChecker
  void checkIfNegativeValuesExist();

 private:
  Map<GroupId, double> groupIdToValue_;
  Map<GroupId, double> groupIdToStaticValue_;
  double defaultValue_;
  double defaultStaticValue_;
  PartitionId partitionId_;
  RoutingConfigId routingConfigId_;
  bool hasNegativeValues_ = false;
  std::optional<double> minimumPositiveValue_ = std::nullopt;
  double maximumValue_ = std::numeric_limits<double>::lowest();
};

} // namespace entities
} // namespace rebalancer
} // namespace facebook
