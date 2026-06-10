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

#include "algopt/rebalancer/interface/ObjectCountExpander.h"

#include <fmt/core.h>
#include <folly/container/irange.h>

namespace facebook::rebalancer::interface {

std::string ObjectCountExpander::makeSyntheticName(
    std::string_view objectName,
    int64_t index) {
  return fmt::format("{}{}{}", objectName, kSyntheticDelimiter, index);
}

const std::vector<std::string>& ObjectCountExpander::lookupSyntheticsOrThrow(
    std::string_view objectName,
    std::string_view context) const {
  const auto it = objectToSyntheticNames_.find(objectName);
  if (FOLLY_UNLIKELY(it == objectToSyntheticNames_.end())) {
    throw std::invalid_argument(
        fmt::format("{} references unknown object '{}'", context, objectName));
  }
  return it->second;
}

std::string ObjectCountExpander::translateSingleObjectName(
    std::string_view objectName,
    std::string_view context) const {
  const auto& syntheticNames = lookupSyntheticsOrThrow(objectName, context);
  if (FOLLY_UNLIKELY(syntheticNames.size() != 1)) {
    throw std::invalid_argument(
        fmt::format(
            "{} does not support counted object '{}' with count {}",
            context,
            objectName,
            syntheticNames.size()));
  }
  return syntheticNames.front();
}

std::vector<std::string> ObjectCountExpander::expandObjectNames(
    const std::vector<std::string>& objectNames,
    std::string_view context) const {
  std::vector<std::string> expanded;
  for (const auto& objectName : objectNames) {
    for (const auto& syntheticName :
         lookupSyntheticsOrThrow(objectName, context)) {
      expanded.push_back(syntheticName);
    }
  }
  return expanded;
}

ObjectCountExpander::ObjectCountExpander(
    const ContainerAssignment& compactAssignment) {
  const auto& objectCounts = *compactAssignment.objectsPerContainer();

  // Pass 1: count totals so we can reserve all capacities upfront,
  // avoiding rehashing and reallocation during the build phase.
  size_t totalSynthetics = 0;
  folly::F14FastMap<std::string, int64_t> perObjectTotal;
  folly::F14FastMap<std::string, int64_t> perContainerTotal;

  for (const auto& [container, counts] : objectCounts) {
    for (const auto& [objectName, count] : counts) {
      if (FOLLY_UNLIKELY(count <= 0)) {
        throw std::invalid_argument(
            fmt::format(
                "Object '{}' has invalid count {} on container '{}'",
                objectName,
                count,
                container));
      }
      perObjectTotal[objectName] += count;
      perContainerTotal[container] += count;
      totalSynthetics += static_cast<size_t>(count);
    }
  }

  syntheticToObject_.reserve(totalSynthetics);
  objectToSyntheticNames_.reserve(perObjectTotal.size());
  for (const auto& [objectName, total] : perObjectTotal) {
    objectToSyntheticNames_[objectName].reserve(static_cast<size_t>(total));
  }

  expandedAssignment_.reserve(objectCounts.size());
  for (const auto& [container, total] : perContainerTotal) {
    expandedAssignment_[container].reserve(static_cast<size_t>(total));
  }

  // Pass 2: generate synthetic names with zero rehashing/reallocation.
  for (const auto& [container, counts] : objectCounts) {
    auto& objects = expandedAssignment_[container];
    for (const auto& [objectName, count] : counts) {
      auto& synthetics = objectToSyntheticNames_[objectName];
      const auto previousSize = static_cast<int64_t>(synthetics.size());

      // We may see the same object in multiple containers, so extend
      // the synthetic list to cover [0, max_needed)
      for (const auto i : folly::irange(previousSize, previousSize + count)) {
        auto syntheticName = makeSyntheticName(objectName, i);
        objects.push_back(syntheticName);
        synthetics.push_back(syntheticName);
        syntheticToObject_.emplace(std::move(syntheticName), objectName);
      }
    }
  }

  // Validate: total synthetics per object must match expected totals.
  for (const auto& [objectName, total] : perObjectTotal) {
    const auto actual =
        static_cast<int64_t>(objectToSyntheticNames_[objectName].size());
    if (actual != total) {
      throw std::runtime_error(
          fmt::format(
              "Synthetic count mismatch for '{}': expected {}, got {}",
              objectName,
              total,
              actual));
    }
  }
}

AvoidMovingSpec ObjectCountExpander::expandAvoidMovingSpec(
    const AvoidMovingSpec& spec) const {
  AvoidMovingSpec expanded;
  expanded.name() = *spec.name();
  expanded.objects() = expandObjectNames(*spec.objects(), "AvoidMovingSpec");
  return expanded;
}

MovesInProgressSpec ObjectCountExpander::expandMovesInProgressSpec(
    const MovesInProgressSpec& spec) const {
  MovesInProgressSpec expanded;
  expanded.name() = *spec.name();

  std::vector<MoveInProgress> expandedMoves;
  for (const auto& move : *spec.moves()) {
    const auto& toContainer = *move.toContainer();
    for (const auto& syntheticName :
         lookupSyntheticsOrThrow(*move.objName(), "MovesInProgressSpec")) {
      auto& expandedMove = expandedMoves.emplace_back();
      expandedMove.objName() = syntheticName;
      expandedMove.toContainer() = toContainer;
    }
  }

  expanded.moves() = std::move(expandedMoves);
  return expanded;
}

bool ObjectCountExpander::hasObject(std::string_view objectName) const {
  return objectToSyntheticNames_.contains(objectName);
}

ContainerAssignment ObjectCountExpander::compressAssignment(
    const folly::F14FastMap<std::string, std::string>& assignment) const {
  ContainerAssignment compactAssignment;
  auto& objectCounts = *compactAssignment.objectsPerContainer();
  for (const auto& [syntheticName, container] : assignment) {
    const auto it = syntheticToObject_.find(syntheticName);
    if (FOLLY_UNLIKELY(it == syntheticToObject_.end())) {
      throw std::runtime_error(
          fmt::format(
              "compressAssignment: unknown synthetic name '{}'",
              syntheticName));
    }
    objectCounts[container][it->second] += 1;
  }
  return compactAssignment;
}

void ObjectCountExpander::compressSolution(AssignmentSolution& solution) const {
  solution.compactAssignment() = compressAssignment(*solution.assignment());
  solution.compactAssignmentInitial() =
      compressAssignment(*solution.initialAssignment());

  // Clean up the legacy assignment/initialAssignment fields to remove
  // synthetic names. Downstream consumers (logging, Manifold backup,
  // Explorer) that read these fields should see original object names.
  // We clear them since the authoritative counted data is in
  // compactAssignment and compactAssignmentInitial.
  solution.assignment()->clear();
  solution.initialAssignment()->clear();
}

const folly::F14FastMap<std::string, std::vector<std::string>>&
ObjectCountExpander::getExpandedAssignment() const {
  return expandedAssignment_;
}

const folly::F14FastMap<std::string, std::vector<std::string>>&
ObjectCountExpander::getObjectToSyntheticNames() const {
  return objectToSyntheticNames_;
}

const folly::F14FastMap<std::string, std::string>&
ObjectCountExpander::getSyntheticToObject() const {
  return syntheticToObject_;
}

} // namespace facebook::rebalancer::interface
