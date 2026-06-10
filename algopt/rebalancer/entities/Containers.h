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

class Containers {
 public:
  // Constructor.
  explicit Containers(
      Map<ContainerId, std::vector<ObjectId>> initialAssignment);
  explicit Containers(const thrift::Containers& containers);

  // Delete the copy and assignment constructors to prevent accidental copies.
  Containers(const Containers&) = delete;
  Containers& operator=(const Containers&) = delete;
  // Other operators are as usual.
  Containers(Containers&&) = default;
  Containers& operator=(Containers&&) = default;
  ~Containers() = default;

  // Get all containerIds.
  auto getContainerIds() const {
    return std::views::keys(initialAssignment_);
  }

  // Get all objectIds initially assigned to the given containerId.
  const std::vector<ObjectId>& getInitialObjectIds(
      ContainerId containerId) const;

  const Map<ContainerId, std::vector<ObjectId>>& getInitialAssignment() const {
    return initialAssignment_;
  }

  thrift::Containers toThrift() const;

  std::vector<thrift::GraphVertex> toEmbedding() const;

 private:
  Map<ContainerId, std::vector<ObjectId>> initialAssignment_;
};

} // namespace entities
} // namespace rebalancer
} // namespace facebook
