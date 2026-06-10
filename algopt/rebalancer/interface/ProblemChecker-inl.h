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

#include "algopt/rebalancer/algopt_common/alias.h"

#include <fmt/core.h>
#include <folly/logging/xlog.h>

#include <stdexcept>

namespace facebook {
namespace rebalancer {
namespace interface {

template <class T>
void ProblemChecker::addObjectDimension(
    const std::string& dimensionName,
    const T& objectToValue) {
  checkObjectNameIsSet();
  addDimension(objectName, dimensionName, objects, objectToValue);
}

template <class T>
void ProblemChecker::addContainerDimension(
    const std::string& dimensionName,
    const T& containerToValue) {
  checkContainerNameIsSet();
  addDimension(
      containerName, dimensionName, scopes.at(containerName), containerToValue);
}

template <class T>
void ProblemChecker::addScopeDimension(
    const std::string& dimensionName,
    const std::string& scopeName,
    const T& scopeItemToValue) {
  checkScopeExists(scopeName);
  addDimension(
      scopeName, dimensionName, scopes.at(scopeName), scopeItemToValue);
}

template <class T>
void ProblemChecker::addDimension(
    const std::string& entityType,
    const std::string& dimensionName,
    const algopt::SetImpl<std::string>& entityNames,
    const T& entityToValue) {
  checkIfDimensionExists(dimensionName, entityType);
  for (auto& [entity, value] : entityToValue) {
    if (!entityNames.contains(entity)) {
      throw std::runtime_error(
          fmt::format(
              "dimension {} defined on unknown {} {}",
              dimensionName,
              entityType,
              entity));
    }
  }
  addGlobalName(dimensionName, EntityType::DIMENSION);
  dimensionToEntityTypes[dimensionName].insert(entityType);
}

} // namespace interface
} // namespace rebalancer
} // namespace facebook
