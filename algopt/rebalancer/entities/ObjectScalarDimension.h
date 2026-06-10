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
#include "algopt/rebalancer/entities/ObjectValueTypes.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"

#include <memory>
#include <optional>

namespace facebook::rebalancer::entities {

struct ScopeScopeItemPair {
  ScopeId scopeId;
  ScopeItemId scopeItemId;
};

class ObjectScalarDimension {
 public:
  virtual ~ObjectScalarDimension() = default;

  ObjectScalarDimension() = default;

  ObjectScalarDimension(const ObjectScalarDimension&) = delete;
  ObjectScalarDimension& operator=(const ObjectScalarDimension&) = delete;
  ObjectScalarDimension(ObjectScalarDimension&&) = delete;
  ObjectScalarDimension& operator=(ObjectScalarDimension&&) = delete;

  virtual const ObjectValues& values(
      std::optional<ScopeItemId> scopeItemId = std::nullopt) const = 0;

  // Getters.
  virtual double getValue(ObjectId objectId) const = 0;
  virtual double getValue(
      ObjectId objectId,
      std::optional<ScopeItemId> scopeItemId) const = 0;

  /**
   * Returns the dimension value for the given object, safely handling scope
   * mismatches.
   *
   * Unlike getValue(ObjectId, std::optional<ScopeItemId>), this method performs
   * a scope check before retrieving the value. If the provided scopeId does not
   * match this dimension's scope, the exception is thrown instead of
   * potentially returning an incorrect default value.
   *
   * @param objectId The object to get the value for.
   * @param scopeScopeItemPair Optional pair containing both the scopeId and
   *        scopeItemId. If nullopt, returns the default value.
   * @return The dimension value for the object if the scope matches, otherwise
   *         the default value.
   */
  virtual double getValueSafe(
      ObjectId objectId,
      std::optional<ScopeScopeItemPair> scopeScopeItemPair) const = 0;
  virtual double getDefaultValue() const = 0;
  virtual bool isDynamic() const = 0;
  virtual bool isRoutingConfigBased() const = 0;
  virtual ScopeId getScopeId() const = 0;

  virtual bool hasNegativeValues() const = 0;
  virtual bool hasZeroValuedObjects() const = 0;
  virtual bool hasZeroValuedObjects(ScopeItemId scopeItemId) const = 0;
  virtual std::optional<double> getMinimumPositiveValue() const = 0;
  virtual double getMaximumValue() const = 0;

  virtual thrift::ObjectScalarDimension toThrift() const = 0;
};

} // namespace facebook::rebalancer::entities
