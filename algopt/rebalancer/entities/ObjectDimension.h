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

#include <memory>
#include <optional>
#include <vector>

namespace facebook {
namespace rebalancer {
namespace entities {

class Partitions;

class ObjectDimension {
 public:
  explicit ObjectDimension(ObjectIdToDoubleMap values);

  explicit ObjectDimension(std::vector<ObjectIdToDoubleMap> values);

  ObjectDimension(
      ScopeId scopeId,
      Map<ScopeItemId, std::shared_ptr<const ObjectIdToDoubleMap>> values,
      double defaultValue,
      std::size_t totalObjects);

  ObjectDimension(
      ScopeId scopeId,
      std::shared_ptr<const Partition> partition,
      const Map<ScopeItemId, std::shared_ptr<const GroupIdToDoubleMap>>& values,
      double defaultValue,
      std::size_t totalObjects,
      PartitionId partitionId);

  ObjectDimension(
      Map<GroupId, double> groupIdToValue,
      double defaultValue,
      PartitionId partitionId,
      RoutingConfigId routingConfigId,
      Map<GroupId, double> groupIdToStaticValue,
      double defaultStaticValue);

  ObjectDimension(
      const thrift::ObjectDimension& objectDimension,
      std::size_t totalObjects,
      const Partitions* partitions = nullptr);

  ~ObjectDimension() = default;

  // Delete the copy and assignment constructors to prevent accidental copies.
  ObjectDimension(const ObjectDimension&) = delete;
  ObjectDimension& operator=(const ObjectDimension&) = delete;
  ObjectDimension(ObjectDimension&&) = delete;
  ObjectDimension& operator=(ObjectDimension&&) = delete;

  // Getters.
  int size() const;
  const ObjectScalarDimension& at(int index) const;
  const ObjectScalarDimension& only() const;

  // an objectDimension is dynamic if at least one of the scalarDimensions is
  // dynamic
  bool isDynamic() const;

  bool isRoutingConfigBased() const;

  thrift::ObjectDimension toThrift() const;

  // TODO (T180294479): remove this once we prevent negative dimension values
  // using ProblemChecker
  bool hasNegativeValues() const;

  /**
    hasZeroValuedObjects function below are conservative and will return true
    even if default value is zero, but all objects have non-zero values.
    This is because currently object dimensions do not have access to the
    number of objects that exist in the problem (T240118355)
  */
  // checks whether the dimension has zero valued objects bool
  bool hasZeroValuedObjects() const;

  // checks w.r.t. a specific scope item
  bool hasZeroValuedObjects(ScopeItemId scopeItemId) const;

  std::optional<double> getMinimumPositiveValue() const;
  double getMaximumValue() const;

 private:
  std::vector<std::unique_ptr<const ObjectScalarDimension>> scalarDimensions_;
  bool isDynamic_ = false;
};

} // namespace entities
} // namespace rebalancer
} // namespace facebook
