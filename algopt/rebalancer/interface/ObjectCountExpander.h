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

#include "algopt/rebalancer/algopt_common/Concepts.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/Types_types.h"

#include <fmt/core.h>
#include <folly/container/F14Map.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace facebook::rebalancer::interface {

// Bridges between compact assignment (container -> object -> count) and the
// existing per-object assignment API (container -> vector<string>) by
// generating synthetic object names ({name}__0, {name}__1, ...).
//
// Constructed with a ContainerAssignment in compact counted form; expansion
// happens at construction time. All subsequent expand/compress methods use the
// mapping built in the constructor.
class ObjectCountExpander {
 public:
  static constexpr std::string_view kSyntheticDelimiter = "__";

  // Expands compact assignment into per-object assignment at construction.
  // Throws std::invalid_argument if any count <= 0.
  explicit ObjectCountExpander(const ContainerAssignment& compactAssignment);

  // Replace original object names with synthetic names in specs.
  // Expands to ALL synthetic instances of the object. Per-instance pinning
  // (e.g., pin 2 of 4) requires native count support (Phase 4+).
  AvoidMovingSpec expandAvoidMovingSpec(const AvoidMovingSpec& spec) const;
  MovesInProgressSpec expandMovesInProgressSpec(
      const MovesInProgressSpec& spec) const;

  template <typename Spec>
  Spec maybeExpandSpec(Spec spec) const;

  // Translate a single original object name to the only synthetic name that
  // represents it. Throws std::invalid_argument if the object is unknown or if
  // it expands to more than one synthetic object.
  std::string translateSingleObjectName(
      std::string_view objectName,
      std::string_view context) const;

  // Flatten a list of object names into all their synthetic names.
  std::vector<std::string> expandObjectNames(
      const std::vector<std::string>& objectNames,
      std::string_view context) const;

  // Generic expansion: accepts any iterable over (objectName, value) pairs
  // and returns an internal map keyed by synthetic object name.
  template <
      typename ObjectToValue,
      typename Value = typename ObjectToValue::value_type::second_type>
  folly::F14FastMap<std::string, Value> expandObjectMap(
      const ObjectToValue& objectToValue) const
    requires IsIterableOverPairs<ObjectToValue, std::string, Value>
  {
    folly::F14FastMap<std::string, Value> expanded;
    for (const auto& [objectName, value] : objectToValue) {
      for (const auto& syntheticName :
           lookupSyntheticsOrThrow(objectName, "expandObjectMap")) {
        expanded[syntheticName] = value;
      }
    }
    return expanded;
  }

  // Expand a partition definition keyed by group -> original objects into
  // group -> synthetic objects while preserving many-to-many membership.
  template <typename GroupToObjects>
  folly::F14FastMap<std::string, std::vector<std::string>> expandGroupToObjects(
      const GroupToObjects& groupToObjects,
      std::string_view context) const
    requires IsIterableOverPairs<
        GroupToObjects,
        std::string,
        std::vector<std::string>>
  {
    folly::F14FastMap<std::string, std::vector<std::string>> expanded;
    for (const auto& [group, objects] : groupToObjects) {
      expanded[group] = expandObjectNames(objects, context);
    }
    return expanded;
  }

  bool hasObject(std::string_view objectName) const;

  // Compress synthetic names back to compact form on the solution.
  void compressSolution(AssignmentSolution& solution) const;

  const folly::F14FastMap<std::string, std::vector<std::string>>&
  getExpandedAssignment() const;
  const folly::F14FastMap<std::string, std::vector<std::string>>&
  getObjectToSyntheticNames() const;
  const folly::F14FastMap<std::string, std::string>& getSyntheticToObject()
      const;

 private:
  template <typename Spec>
  Spec maybeExpandSpecImpl(Spec spec) const;
  AvoidMovingSpec maybeExpandSpecImpl(const AvoidMovingSpec& spec) const;
  MovesInProgressSpec maybeExpandSpecImpl(
      const MovesInProgressSpec& spec) const;
  AvoidAssignmentsSpec maybeExpandSpecImpl(AvoidAssignmentsSpec spec) const;
  AssignmentAffinitiesSpec maybeExpandSpecImpl(
      AssignmentAffinitiesSpec spec) const;
  ExclusiveSwapsSpec maybeExpandSpecImpl(ExclusiveSwapsSpec spec) const;
  ExclusiveObjectsSpec maybeExpandSpecImpl(ExclusiveObjectsSpec spec) const;
  // NOLINTBEGIN(readability-convert-member-functions-to-static)
  FlowSpec maybeExpandSpecImpl(const FlowSpec& spec) const;
  PairAffinitiesSpec maybeExpandSpecImpl(const PairAffinitiesSpec& spec) const;
  WorkingSetSpec maybeExpandSpecImpl(const WorkingSetSpec& spec) const;
  DisasterRecoveryCapacitySpec maybeExpandSpecImpl(
      const DisasterRecoveryCapacitySpec& spec) const;
  ObjectAffinitiesSpec maybeExpandSpecImpl(
      const ObjectAffinitiesSpec& spec) const;
  // NOLINTEND(readability-convert-member-functions-to-static)

  [[noreturn]] static void throwUnsupportedSpec(std::string_view specName);

  static std::string makeSyntheticName(
      std::string_view objectName,
      int64_t index);

  // Lookup an object's synthetic names, throwing if not found.
  const std::vector<std::string>& lookupSyntheticsOrThrow(
      std::string_view objectName,
      std::string_view context) const;

  // Compress a single object->container map into a compact ContainerAssignment.
  // Throws std::runtime_error if any name is not a known synthetic.
  ContainerAssignment compressAssignment(
      const folly::F14FastMap<std::string, std::string>& assignment) const;

  folly::F14FastMap<std::string, std::vector<std::string>> expandedAssignment_;
  folly::F14FastMap<std::string, std::vector<std::string>>
      objectToSyntheticNames_;
  folly::F14FastMap<std::string, std::string> syntheticToObject_;
};

template <typename Spec>
Spec ObjectCountExpander::maybeExpandSpec(Spec spec) const {
  return maybeExpandSpecImpl(std::move(spec));
}

template <typename Spec>
Spec ObjectCountExpander::maybeExpandSpecImpl(Spec spec) const {
  return std::move(spec);
}

inline AvoidMovingSpec ObjectCountExpander::maybeExpandSpecImpl(
    const AvoidMovingSpec& spec) const {
  return expandAvoidMovingSpec(spec);
}

inline MovesInProgressSpec ObjectCountExpander::maybeExpandSpecImpl(
    const MovesInProgressSpec& spec) const {
  return expandMovesInProgressSpec(spec);
}

inline AvoidAssignmentsSpec ObjectCountExpander::maybeExpandSpecImpl(
    AvoidAssignmentsSpec spec) const {
  std::vector<AvoidAssignment> expandedAssignments;
  for (const auto& assignment : *spec.assignments()) {
    for (const auto& syntheticName : expandObjectNames(
             std::vector<std::string>{*assignment.object()},
             "AvoidAssignmentsSpec")) {
      AvoidAssignment expandedAssignment = assignment;
      expandedAssignment.object() = syntheticName;
      expandedAssignments.push_back(std::move(expandedAssignment));
    }
  }
  spec.assignments() = std::move(expandedAssignments);
  return spec;
}

inline AssignmentAffinitiesSpec ObjectCountExpander::maybeExpandSpecImpl(
    AssignmentAffinitiesSpec spec) const {
  std::vector<AssignmentAffinity> expandedAffinities;
  for (const auto& affinity : *spec.affinities()) {
    for (const auto& syntheticName : expandObjectNames(
             std::vector<std::string>{*affinity.objectName()},
             "AssignmentAffinitiesSpec")) {
      AssignmentAffinity expandedAffinity = affinity;
      expandedAffinity.objectName() = syntheticName;
      expandedAffinities.push_back(std::move(expandedAffinity));
    }
  }
  spec.affinities() = std::move(expandedAffinities);
  return spec;
}

inline ExclusiveSwapsSpec ObjectCountExpander::maybeExpandSpecImpl(
    ExclusiveSwapsSpec spec) const {
  if (spec.subsetObjects()) {
    spec.subsetObjects() =
        expandObjectNames(*spec.subsetObjects(), "ExclusiveSwapsSpec");
  }
  return spec;
}

inline ExclusiveObjectsSpec ObjectCountExpander::maybeExpandSpecImpl(
    ExclusiveObjectsSpec spec) const {
  for (auto& pair : *spec.pairs()) {
    pair.object1() =
        translateSingleObjectName(*pair.object1(), "ExclusiveObjectsSpec");
    pair.object2() =
        translateSingleObjectName(*pair.object2(), "ExclusiveObjectsSpec");
  }
  return spec;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
inline FlowSpec ObjectCountExpander::maybeExpandSpecImpl(
    const FlowSpec&) const {
  throwUnsupportedSpec("FlowSpec");
}

inline PairAffinitiesSpec ObjectCountExpander::maybeExpandSpecImpl(
    const PairAffinitiesSpec&) const {
  throwUnsupportedSpec("PairAffinitiesSpec");
}

inline WorkingSetSpec ObjectCountExpander::maybeExpandSpecImpl(
    const WorkingSetSpec&) const {
  throwUnsupportedSpec("WorkingSetSpec");
}

inline DisasterRecoveryCapacitySpec ObjectCountExpander::maybeExpandSpecImpl(
    const DisasterRecoveryCapacitySpec&) const {
  throwUnsupportedSpec("DisasterRecoveryCapacitySpec");
}

inline ObjectAffinitiesSpec ObjectCountExpander::maybeExpandSpecImpl(
    const ObjectAffinitiesSpec&) const {
  throwUnsupportedSpec("ObjectAffinitiesSpec");
}
// NOLINTEND(readability-convert-member-functions-to-static)

inline void ObjectCountExpander::throwUnsupportedSpec(
    std::string_view specName) {
  throw std::invalid_argument(
      fmt::format("{} is not supported with compact assignments", specName));
}

} // namespace facebook::rebalancer::interface
