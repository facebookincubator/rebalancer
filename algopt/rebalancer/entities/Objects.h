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
#include "algopt/rebalancer/entities/ObjectDimension.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/GraphState_types.h"

namespace facebook {
namespace rebalancer {
namespace entities {

class Partitions;

class Objects {
 public:
  Objects(
      EntityIdType numObjects,
      Map<DimensionId, std::shared_ptr<const ObjectDimension>> dimensions);

  explicit Objects(
      const thrift::Objects& objects,
      const Partitions* partitions = nullptr);

  ~Objects() = default;

  // Delete the copy and assignment constructors to prevent accidental copies.
  Objects(const Objects&) = delete;
  Objects& operator=(const Objects&) = delete;
  Objects(Objects&&) = delete;
  Objects& operator=(Objects&&) = delete;

  // Object IDs are dense and contiguous from 0: the universe always has
  // {ObjectId(0), ..., ObjectId(numObjects - 1)}.
  EntityIdType getNumObjects() const {
    return numObjects_;
  }

  auto getObjectIds() const {
    return std::views::iota(EntityIdType{0}, numObjects_) |
        std::views::transform([](EntityIdType i) { return ObjectId(i); });
  }

  auto getDimensionIds() const {
    return std::views::keys(dimensions_);
  }

  // Dimension getter.
  const ObjectDimension& getDimension(DimensionId dimensionId) const;

  thrift::Objects toThrift() const;

  std::vector<thrift::GraphVertex> toEmbedding(
      const std::vector<DimensionId>& dimIds) const;

  bool hasNegativeDimensions() const;

  size_t getDimensionCount() const;

 private:
  EntityIdType numObjects_{0};
  Map<DimensionId, std::shared_ptr<const ObjectDimension>> dimensions_;

  std::shared_ptr<const ObjectDimension> defaultDimension_;
};

} // namespace entities
} // namespace rebalancer
} // namespace facebook
