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

#include <fmt/core.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace facebook::rebalancer::entities {

template <typename NameToIdMap>
inline std::string summarizeNames(
    std::string_view header,
    const NameToIdMap& names) {
  if (names.empty()) {
    return "";
  }
  std::vector<std::string_view> sortedNames;
  sortedNames.reserve(names.size());
  for (const auto& [name, _] : names) {
    sortedNames.push_back(name);
  }
  std::sort(sortedNames.begin(), sortedNames.end());
  std::string result = fmt::format("{}:\n", header);
  for (const auto& name : sortedNames) {
    result += fmt::format("  {}\n", name);
  }
  return result;
}

} // namespace facebook::rebalancer::entities
