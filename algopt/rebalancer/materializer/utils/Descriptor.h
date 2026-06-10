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

#include "algopt/rebalancer/entities/Universe.h"

namespace facebook::rebalancer::materializer {

// The different values of UtilMetric are explained in the Wiki with diagrams:
// https://www.internalfb.com/intern/wiki/ReBalancer/API/Goals_and_constraints/Capacity/Choices_of_definition/
enum UtilMetric {
  AFTER,
  DURING,
  NEW,
  OLD,
  INITIAL,
  STAYED,
  MOVED,
};

struct Descriptor {
  std::optional<entities::DimensionId> dimensionId;
  entities::ScopeId scopeId;
  std::optional<entities::ScopeItemId> scopeItemId;
  std::optional<entities::PartitionId> partitionId;
  std::optional<entities::GroupId> groupId;
  bool outOfScope = false;

  bool operator==(const Descriptor& other) const;
};

inline bool Descriptor::operator==(const Descriptor& other) const {
  return dimensionId == other.dimensionId && scopeId == other.scopeId &&
      scopeItemId == other.scopeItemId && partitionId == other.partitionId &&
      groupId == other.groupId;
}

} // namespace facebook::rebalancer::materializer

namespace std {

template <>
struct hash<facebook::rebalancer::materializer::Descriptor> {
  std::size_t operator()(const facebook::rebalancer::materializer::Descriptor&
                             descriptor) const noexcept {
    return std::hash<std::tuple<
        std::optional<facebook::rebalancer::entities::DimensionId>,
        facebook::rebalancer::entities::ScopeId,
        std::optional<facebook::rebalancer::entities::ScopeItemId>,
        std::optional<facebook::rebalancer::entities::PartitionId>,
        std::optional<facebook::rebalancer::entities::GroupId>,
        bool>>{}(
        std::make_tuple(
            descriptor.dimensionId,
            descriptor.scopeId,
            descriptor.scopeItemId,
            descriptor.partitionId,
            descriptor.groupId,
            descriptor.outOfScope));
  }
};

} // namespace std
