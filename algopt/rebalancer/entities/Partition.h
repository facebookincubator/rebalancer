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
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/GraphState_types.h"

#include <vector>

namespace facebook {
namespace rebalancer {
namespace entities {

class Partition {
 public:
  // Constructor.
  explicit Partition(Map<GroupId, std::vector<ObjectId>> groups);
  explicit Partition(const thrift::Partition& partition);

  // Delete the copy and assignment constructors to prevent accidental copies.
  Partition(const Partition&) = delete;
  Partition& operator=(const Partition&) = delete;
  // Other operators are as usual.
  Partition(Partition&&) = default;
  Partition& operator=(Partition&&) = default;
  ~Partition() = default;

  const std::vector<GroupId>& getGroupIds() const;

  const std::vector<ObjectId>& getObjectIds(GroupId groupId) const;

  const Map<ObjectId, std::vector<GroupId>>& getObjectIdToGroupIds() const;

  const std::shared_ptr<const Map<ObjectId, std::vector<GroupId>>>&
  getObjectIdToGroupIdsPtr() const;

  const Map<GroupId, std::vector<ObjectId>>& getGroupToObjectIds() const;

  thrift::Partition toThrift() const;

  std::vector<thrift::GraphVertex> toEmbedding() const;

  bool isDisjoint() const;

 private:
  Map<GroupId, std::vector<ObjectId>> groups_;
  std::vector<GroupId> groupIds_;
  std::shared_ptr<const Map<ObjectId, std::vector<GroupId>>>
      objectIdToGroupIds_;
  bool isDisjoint_{false};
};

} // namespace entities
} // namespace rebalancer
} // namespace facebook
