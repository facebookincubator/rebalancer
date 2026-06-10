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
#include "algopt/rebalancer/entities/Universe.h"
#include "algopt/rebalancer/solver/if/gen-cpp2/packer_types.h"
#include "algopt/rebalancer/solver/utils/Util.h"

#include <folly/container/irange.h>

#include <string>

namespace facebook::rebalancer::entities {
class ObjectValues;
}

namespace entities = facebook::rebalancer::entities;

namespace facebook::rebalancer {

class PropertiesHelper {
 public:
  static ExpressionPropertyValue makeDoubleValue(double value);
  static ExpressionPropertyValue makeStringValue(std::string value);
  static ExpressionPropertyValue makeContainerIdListValue(
      PackerSet<entities::ContainerId> value);
  static ExpressionPropertyValue makeIntValue(int value);
  static ExpressionPropertyValue makeBoolValue(bool value);
  static ExpressionPropertyValue makeObjectIdDoubleMapValue(
      const entities::ObjectIdToDoubleMap& value);
  static ExpressionPropertyValue makeObjectIdDoubleMapValue(
      const entities::ObjectValues& value);
  static ExpressionPropertyValue makePoint2dListValue(
      std::vector<std::pair<double, double>> value);
  static ExpressionPropertyValue makeObjectIdValue(entities::ObjectId value);
  static ExpressionPropertyValue makeContainerIdValue(
      entities::ContainerId value);

  template <typename T>
  static ExpressionPropertyValue makeScopeItemNames(
      const T& scopeItemIds,
      const entities::ScopeId scopeId,
      const entities::Universe& universe) {
    const auto& allScopeItems = universe.getScope(scopeId).getScopeItemIds();
    if (scopeItemIds.size() == allScopeItems.size()) {
      return makeStringValue(
          fmt::format(
              "all scope items in scope '{}'",
              universe.getEntityName(scopeId)));
    } else {
      std::string scopeItemNames;
      constexpr size_t maxNamesToShow = 5;
      const size_t numToShow = std::min(maxNamesToShow, scopeItemIds.size());
      for (const auto i : folly::irange(numToShow)) {
        const auto& scopeItemName = universe.getEntityName(scopeItemIds.at(i));
        scopeItemNames.append(scopeItemName);
        if (i != numToShow - 1) {
          scopeItemNames.append(", ");
        }
      }

      if (scopeItemIds.size() > maxNamesToShow) {
        const size_t remaining = scopeItemIds.size() - maxNamesToShow;
        scopeItemNames.append(fmt::format(" ... and {} more", remaining));
      }

      return makeStringValue(std::move(scopeItemNames));
    }
  }
};

} // namespace facebook::rebalancer
