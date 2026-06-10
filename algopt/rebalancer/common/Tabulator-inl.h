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

namespace facebook {
namespace rebalancer {

template <typename T>
Tabulator& Tabulator::addRow(const T& columns) {
  int pos = 0;
  auto& row = rows_.emplace_back();
  for (const auto& col : columns) {
    auto data = toString(col);
    const uint32_t size = data.size();
    row.push_back(std::move(data));
    colWidths_[pos] = std::max(colWidths_[pos], size);
    ++pos;
  }
  return *this;
}

template <typename T>
std::string Tabulator::toString(const T& data) {
  return fmt::format("{}", data);
}

} // namespace rebalancer
} // namespace facebook
