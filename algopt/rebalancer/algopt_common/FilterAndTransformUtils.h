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
#include "algopt/rebalancer/algopt_common/thrift/gen-cpp2/Types_types.h"
#include "algopt/rebalancer/algopt_common/Utils.h"

#include <functional>
#include <stdexcept>
#include <vector>

namespace facebook::algopt::common {

namespace FilterAndTransformUtils {

template <typename InElementType, typename OutElementType>
std::vector<OutElementType> transformAndFilterWithBlockList(
    const std::vector<OutElementType>& elements,
    const std::vector<InElementType>& blockList,
    std::function<OutElementType(const InElementType&)> transformFunc) {
  auto excluded = blockList | std::ranges::views::transform(transformFunc) |
      utils::to<folly::F14FastSet>;

  return elements |
      std::ranges::views::filter([&excluded](const auto& element) {
           return !excluded.contains(element);
         }) |
      utils::to<std::vector>;
}

template <typename OutElementType, typename InElementType>
std::vector<OutElementType> transformAndFilter(
    const std::vector<OutElementType>& origElements,
    const std::vector<InElementType>& elementsToFilter,
    common::thrift::FilterType filterType,
    std::function<OutElementType(const InElementType&)> transformFunc) {
  if (filterType == common::thrift::FilterType::BLOCKLIST) {
    return transformAndFilterWithBlockList(
        origElements, elementsToFilter, transformFunc);
  } else if (filterType == common::thrift::FilterType::ALLOWLIST) {
    return elementsToFilter | std::ranges::views::transform(transformFunc) |
        utils::to<std::vector>;
  } else {
    throw std::runtime_error(
        fmt::format("Unexpected filterType {}", fmt::underlying(filterType)));
  }
}

template <typename ItemType>
std::vector<ItemType> selectAllowedItems(
    const std::vector<ItemType>& allItems,
    const thrift::StringListFilterConfig& filterConfig,
    const std::function<ItemType(std::string)>& getItemFromName =
        [](const auto& name) {
          return name; /* identity function */
        }) {
  return transformAndFilter<ItemType, std::string>(
      allItems,
      *filterConfig.stringsToFilter(),
      *filterConfig.filterType(),
      getItemFromName);
}

} // namespace FilterAndTransformUtils

} // namespace facebook::algopt::common
