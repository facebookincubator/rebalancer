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
#include "algopt/rebalancer/entities/ObjectScalarDimension.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"

namespace facebook {
namespace rebalancer {
namespace entities {

class ObjectStaticDimension : public ObjectScalarDimension {
 public:
  // Constructors.
  explicit ObjectStaticDimension(ObjectIdToDoubleMap values);
  ObjectStaticDimension(
      const thrift::ObjectStaticDimension& objectStaticDimension,
      std::size_t totalObjects);

  ~ObjectStaticDimension() override = default;

  // Delete the copy and assignment constructors to prevent accidental copies.
  ObjectStaticDimension(const ObjectStaticDimension&) = delete;
  ObjectStaticDimension& operator=(const ObjectStaticDimension&) = delete;
  ObjectStaticDimension(ObjectStaticDimension&&) = delete;
  ObjectStaticDimension& operator=(ObjectStaticDimension&&) = delete;

  // Getters.
  const ObjectValues& values(
      std::optional<ScopeItemId> scopeItemId = std::nullopt) const override;
  double getValue(ObjectId objectId) const override;
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
  ObjectValues objectValues_;
  bool hasNegativeValues_ = false;
  bool hasZeroValuedObjects_ = false;
  std::optional<double> minimumPositiveValue_ = std::nullopt;
  double maximumValue_ = std::numeric_limits<double>::lowest();
};

} // namespace entities
} // namespace rebalancer
} // namespace facebook
