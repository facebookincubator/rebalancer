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

#include "algopt/rebalancer/entities/builders/IdBuilder.h"
#include "algopt/rebalancer/entities/Identifiers.h"

#include <folly/container/F14Map.h>
#include <folly/container/F14Set.h>
#include <folly/coro/Task.h>
#include <folly/futures/SharedPromise.h>
#include <folly/Synchronized.h>

#include <random>

namespace facebook::rebalancer::entities {

struct ObjectsData {
  ObjectsData(
      EntityIdType numObjects,
      Map<std::string, ObjectId> objectNameToId)
      : numObjects(numObjects), objectNameToId(std::move(objectNameToId)) {}

  ObjectId getId(const std::string& name) const {
    return objectNameToId.at(name);
  }

  EntityIdType numObjects;
  Map<std::string, ObjectId> objectNameToId;
};

struct ContainersData {
  ContainersData(
      Map<ContainerId, std::vector<ObjectId>> containerIdToObjectIds,
      Map<std::string, ContainerId> containerNameToId)
      : containerNameToId(std::move(containerNameToId)),
        containerIdToObjectIds(std::move(containerIdToObjectIds)) {}

  ContainerId getId(const std::string& name) const {
    return containerNameToId.at(name);
  }

  Map<std::string, ContainerId> containerNameToId;
  Map<ContainerId, std::vector<ObjectId>> containerIdToObjectIds;
};

struct AssignmentBuilderResult {
  Map<std::string, ObjectId> objectNameToId;
  Map<std::string, ContainerId> containerNameToId;
  EntityIdType numObjects{};
  Map<ContainerId, std::vector<ObjectId>> containerIdToObjectIds;
};

class AssignmentBuilder {
 public:
  explicit AssignmentBuilder(IdBuilder& idBuilder) : idBuilder_(idBuilder) {}

  template <typename ContainerToObjects>
  void set(const ContainerToObjects& containerNameToObjectNames)
    requires IsIterableOverPairs<
        ContainerToObjects,
        std::string,
        std::vector<std::string>>;

  bool wasPreviouslySet() const {
    return objects_ != nullptr;
  }

  std::shared_ptr<const ObjectsData> getObjects() const;
  std::shared_ptr<const ContainersData> getContainers() const;

  folly::coro::Task<AssignmentBuilderResult> build() const;

  folly::coro::Task<std::string> summarize() const;

 private:
  // Rebuilds the container->objects mapping using the same set of object
  // and container names registered by the initial `set()` call. Existing
  // IDs are reused, `objects_` is left untouched, and only `containers_`
  // gets a new value. Throws if the input does not exactly match that set.
  // Called by `set()` on every call after the first.
  template <typename ContainerToObjects>
  void reassign(const ContainerToObjects& containerNameToObjectNames)
    requires IsIterableOverPairs<
        ContainerToObjects,
        std::string,
        std::vector<std::string>>;

  void throwIfObjectsNotSet() const;
  void throwIfContainersNotSet() const;

 private:
  IdBuilder& idBuilder_;
  std::shared_ptr<ObjectsData> objects_{nullptr};
  std::shared_ptr<ContainersData> containers_{nullptr};
};

namespace detail {
template <typename ContainerToObjects>
const std::vector<std::pair<const std::string*, const std::string*>>
getShuffledAssignment(const ContainerToObjects& containerToObjects)
  requires IsIterableOverPairs<
      ContainerToObjects,
      std::string,
      std::vector<std::string>>
{
  std::vector<std::pair<const std::string*, const std::string*>>
      shuffledAssignment;
  for (const auto& [containerName, objectNames] : containerToObjects) {
    if (objectNames.empty()) {
      shuffledAssignment.emplace_back(&containerName, nullptr);
      continue;
    }
    for (auto& objectName : objectNames) {
      shuffledAssignment.emplace_back(&containerName, &objectName);
    }
  }

  std::mt19937 gen;
  std::shuffle(shuffledAssignment.begin(), shuffledAssignment.end(), gen);

  return shuffledAssignment;
}
} // namespace detail

template <typename ContainerToObjects>
void AssignmentBuilder::set(
    const ContainerToObjects& containerNameToObjectNames)
  requires IsIterableOverPairs<
      ContainerToObjects,
      std::string,
      std::vector<std::string>>
{
  if (objects_) {
    reassign(containerNameToObjectNames);
    return;
  }

  const auto shuffledAssignment =
      detail::getShuffledAssignment(containerNameToObjectNames);

  EntityIdType numObjects = 0;
  Map<std::string, ObjectId> objectNameToId;

  const auto containerCount = containerNameToObjectNames.size();
  Map<std::string, ContainerId> containerNameToId;
  containerNameToId.reserve(
      static_cast<decltype(containerNameToId)::size_type>(containerCount));
  Map<ContainerId, std::vector<ObjectId>> containerIdToObjectIds;
  containerIdToObjectIds.reserve(
      static_cast<decltype(containerIdToObjectIds)::size_type>(containerCount));
  for (const auto [containerNamePtr, objectNamePtr] : shuffledAssignment) {
    assert(containerNamePtr);
    const auto containerIdPtr =
        folly::get_ptr(containerNameToId, *containerNamePtr);
    const auto containerId =
        containerIdPtr ? *containerIdPtr : idBuilder_.next<ContainerId>();
    if (!containerIdPtr) {
      containerNameToId.emplace(*containerNamePtr, containerId);
    }

    if (objectNamePtr) {
      const auto objectId = idBuilder_.next<ObjectId>();
      const auto [_, isNew] = objectNameToId.emplace(*objectNamePtr, objectId);
      if (!isNew) [[unlikely]] {
        throw std::runtime_error(
            fmt::format("Duplicate object '{}' in assignment", *objectNamePtr));
      }
      containerIdToObjectIds[containerId].emplace_back(objectId);
      ++numObjects;
    } else {
      containerIdToObjectIds[containerId] = {};
    }
  }

  if (containerIdToObjectIds.size() != containerNameToObjectNames.size()) {
    throw std::runtime_error(
        "Expected 'assignment' to have every container map to a vector of objects");
  }

  objects_ =
      std::make_unique<ObjectsData>(numObjects, std::move(objectNameToId));

  containers_ = std::make_unique<ContainersData>(
      std::move(containerIdToObjectIds), std::move(containerNameToId));
}

template <typename ContainerToObjects>
void AssignmentBuilder::reassign(
    const ContainerToObjects& containerNameToObjectNames)
  requires IsIterableOverPairs<
      ContainerToObjects,
      std::string,
      std::vector<std::string>>
{
  const auto throwSetMismatch = [](std::string_view detail) {
    throw std::runtime_error(
        fmt::format(
            "AssignmentBuilder::set: re-set must use the same set of objects "
            "and containers as the initial call ({})",
            detail));
  };

  const auto& priorObjectNameToId = objects_->objectNameToId;
  const auto& priorContainerNameToId = containers_->containerNameToId;

  // Dedup tracker. ObjectIds are dense from 0, so use a bitset.
  std::vector<bool> seenObjects(objects_->numObjects, false);
  std::size_t totalObjects = 0;

  Map<ContainerId, std::vector<ObjectId>> containerIdToObjectIds;
  containerIdToObjectIds.reserve(priorContainerNameToId.size());

  for (const auto& [containerName, objectNames] : containerNameToObjectNames) {
    const auto containerIdPtr =
        folly::get_ptr(priorContainerNameToId, containerName);
    if (!containerIdPtr) [[unlikely]] {
      throwSetMismatch(fmt::format("unknown container '{}'", containerName));
    }
    const auto [objectsInContainerIt, inserted] =
        containerIdToObjectIds.try_emplace(*containerIdPtr);
    if (!inserted) [[unlikely]] {
      throw std::runtime_error(
          fmt::format("Duplicate container '{}' in assignment", containerName));
    }
    auto& objectsInContainer = objectsInContainerIt->second;
    objectsInContainer.reserve(objectNames.size());

    for (const auto& objectName : objectNames) {
      const auto objectIdPtr = folly::get_ptr(priorObjectNameToId, objectName);
      if (!objectIdPtr) [[unlikely]] {
        throwSetMismatch(fmt::format("unknown object '{}'", objectName));
      }
      const auto objectIndex = objectIdPtr->asIndex();
      if (seenObjects[objectIndex]) [[unlikely]] {
        throw std::runtime_error(
            fmt::format("Duplicate object '{}' in assignment", objectName));
      }
      seenObjects[objectIndex] = true;
      objectsInContainer.emplace_back(*objectIdPtr);
      ++totalObjects;
    }
  }

  // No unknowns + no duplicates + same counts => same set as before.
  if (containerIdToObjectIds.size() != priorContainerNameToId.size() ||
      totalObjects != objects_->numObjects) [[unlikely]] {
    // find a specific missing name for an actionable error.
    if (containerIdToObjectIds.size() != priorContainerNameToId.size()) {
      for (const auto& [name, id] : priorContainerNameToId) {
        if (!containerIdToObjectIds.contains(id)) {
          throwSetMismatch(fmt::format("missing container '{}'", name));
        }
      }
    }
    for (const auto& [name, id] : priorObjectNameToId) {
      if (!seenObjects[id.asIndex()]) {
        throwSetMismatch(fmt::format("missing object '{}'", name));
      }
    }
  }

  containers_ = std::make_shared<ContainersData>(
      std::move(containerIdToObjectIds), priorContainerNameToId);
}

} // namespace facebook::rebalancer::entities
