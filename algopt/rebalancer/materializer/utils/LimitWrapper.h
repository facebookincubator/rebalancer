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
#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSolver_types.h"

#include <cstdint>

namespace facebook::rebalancer::materializer {

class LimitWrapper {
 public:
  LimitWrapper(
      std::shared_ptr<const entities::Universe> universe,
      const facebook::rebalancer::interface::Limit& limit,
      entities::ScopeId scopeId);
  LimitWrapper(
      std::shared_ptr<const entities::Universe> universe,
      const facebook::rebalancer::interface::Limit& limit,
      entities::ScopeId scopeId,
      entities::PartitionId partitionId);

  facebook::rebalancer::interface::LimitType getType() const;

  double getLimit(entities::ScopeItemId scopeItemId) const;
  double getLimit(entities::GroupId groupId) const;
  double getLimit(entities::ScopeItemId scopeItemId, entities::GroupId groupId)
      const;

  entities::Map<entities::GroupId, double> getGroupsOverride(
      entities::ScopeItemId scopeItemId) const;

  // returns all the limit values associated with groups in partitionId_ that
  // are either specified using globalLimit or using groupLimits in the
  // LimitsSpec (i.e., interface::Limit)
  entities::Map<entities::GroupId, double> getAllGroupLimitsIndptOfScopeItem()
      const;

  /** get limits for all groups of the partition @param partitionId */
  static entities::Map<entities::GroupId, double> getAllGroupLimits(
      std::shared_ptr<const entities::Universe> universe,
      entities::PartitionId partitionId,
      const interface::Limit& limit);

  double getGlobalLimit() const;

  // Returns true when all limits resolve to globalLimit (no per-scopeItem,
  // per-group, or per-(scopeItem, group) overrides).
  bool onlyHasGlobalLimit() const;

  // checks if scopeItemLimits are positive integrs and returns them as a map
  entities::Map<entities::ScopeItemId, uint32_t>
  checkAndGetPositiveIntegerScopeItemLimits() const;

 private:
  void parseScopeLimit(const interface::Limit& limit);
  void parseGroupLimit(const interface::Limit& limit);
  void parseScopeGroupLimit(const interface::Limit& limit);
  static double parseGlobalLimit(const interface::Limit& limit);

 private:
  std::shared_ptr<const entities::Universe> universe_;
  std::optional<entities::ScopeId> scopeId_;
  std::optional<entities::PartitionId> partitionId_;
  facebook::rebalancer::interface::LimitType type_;
  double globalLimit_;
  entities::Map<entities::ScopeItemId, double> scopeItemLimits_;
  entities::Map<entities::GroupId, double> groupItemLimits_;
  entities::Map<entities::ScopeItemId, entities::Map<entities::GroupId, double>>
      scopeItemToGroupItemLimits_;
};

} // namespace facebook::rebalancer::materializer
