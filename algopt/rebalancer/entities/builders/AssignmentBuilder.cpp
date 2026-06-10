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

#include "algopt/rebalancer/entities/builders/AssignmentBuilder.h"

#include <fmt/core.h>
#include <folly/coro/Collect.h>

namespace facebook::rebalancer::entities {

std::shared_ptr<const ObjectsData> AssignmentBuilder::getObjects() const {
  throwIfObjectsNotSet();
  return objects_;
}

std::shared_ptr<const ContainersData> AssignmentBuilder::getContainers() const {
  throwIfContainersNotSet();
  return containers_;
}

folly::coro::Task<AssignmentBuilderResult> AssignmentBuilder::build() const {
  throwIfObjectsNotSet();
  throwIfContainersNotSet();

  AssignmentBuilderResult result;
  result.numObjects = objects_->numObjects;
  result.containerIdToObjectIds = containers_->containerIdToObjectIds;
  result.objectNameToId = objects_->objectNameToId;
  result.containerNameToId = containers_->containerNameToId;

  co_return result;
}

void AssignmentBuilder::throwIfObjectsNotSet() const {
  if (!objects_) [[unlikely]] {
    throw std::runtime_error(
        "Expect objects to be set. Did you set initialAssignment?");
  }
}

void AssignmentBuilder::throwIfContainersNotSet() const {
  if (!containers_) [[unlikely]] {
    throw std::runtime_error(
        "Expect containers to be set. Did you set initialAssignment?");
  }
}

folly::coro::Task<std::string> AssignmentBuilder::summarize() const {
  throwIfObjectsNotSet();
  throwIfContainersNotSet();

  co_return fmt::format(
      "  #Objects: {}\n  #Containers: {}\n",
      objects_->numObjects,
      containers_->containerIdToObjectIds.size());
}

} // namespace facebook::rebalancer::entities
