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
#include "algopt/rebalancer/entities/Partition.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"

#include <optional>

namespace facebook::rebalancer::entities {

class Partitions;

class ObjectDynamicDimension : public ObjectScalarDimension {
 public:
  ObjectDynamicDimension(
      ScopeId scopeId,
      Map<ScopeItemId, std::shared_ptr<const ObjectIdToDoubleMap>> values,
      double defaultValue,
      std::size_t totalObjects);

  ObjectDynamicDimension(
      ScopeId scopeId,
      std::shared_ptr<const Partition> partition,
      const Map<ScopeItemId, std::shared_ptr<const GroupIdToDoubleMap>>& values,
      double defaultValue,
      std::size_t totalObjects,
      PartitionId partitionId);

  ObjectDynamicDimension(
      const thrift::ObjectDynamicDimension& objectDynamicDimension,
      std::size_t totalObjects,
      const Partitions* partitions = nullptr);

  ~ObjectDynamicDimension() override = default;

  // Delete the copy and assignment constructors to prevent accidental copies.
  ObjectDynamicDimension(const ObjectDynamicDimension&) = delete;
  ObjectDynamicDimension& operator=(const ObjectDynamicDimension&) = delete;
  ObjectDynamicDimension(ObjectDynamicDimension&&) = delete;
  ObjectDynamicDimension& operator=(ObjectDynamicDimension&&) = delete;

  // Getters.
  const ObjectValues& values(
      std::optional<ScopeItemId> scopeItemId = std::nullopt) const override;
  double getValue(ObjectId objectId) const override;

  /**  NOTE: The function below returns the correct dimension value only if
  @param scopeItemId is an item of this dimension's scope. If @param scopeItemId
  is not an item of S, the function will return a default value (which is likely
  incorrect). We should modify this to check if scopes match. This is tracked in
  T239951109.
  */
  double getValue(ObjectId objectId, std::optional<ScopeItemId> scopeItemId)
      const override;

  double getValueSafe(
      ObjectId objectId,
      std::optional<ScopeScopeItemPair> scopeScopeItemPair) const override;

  double getDefaultValue() const override;
  bool isDynamic() const override;
  bool isRoutingConfigBased() const override;
  ScopeId getScopeId() const override;

  bool hasNegativeValues() const override;
  bool hasZeroValuedObjects() const override;
  bool hasZeroValuedObjects(ScopeItemId scopeItemId) const override;
  std::optional<double> getMinimumPositiveValue() const override;
  double getMaximumValue() const override;

  thrift::ObjectScalarDimension toThrift() const override;

 private:
  double getValueFor(ObjectId objectId, ScopeItemId scopeItemId) const;

  ScopeId scopeId_;
  Map<ScopeItemId, ObjectValues> objectValues_;
  ObjectValues emptyObjectValues_;
  double defaultValue_;
  bool hasNegativeValues_ = false;
  std::optional<double> minimumPositiveValue_ = std::nullopt;
  double maximumValue_ = std::numeric_limits<double>::lowest();
  bool hasZeroValuedObjects_ = false;
  Map<ScopeItemId, bool> scopeItemToHasZeroValue_;
};

} // namespace facebook::rebalancer::entities
