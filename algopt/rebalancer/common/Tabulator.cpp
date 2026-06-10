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

#include "algopt/rebalancer/common/Tabulator.h"

#include <fmt/format.h>
#include <folly/container/irange.h>
#include <folly/json/json.h>

#include <initializer_list>
#include <iterator>

namespace facebook {
namespace rebalancer {
Tabulator& Tabulator::setHeader(const std::vector<std::string>& header) {
  for (const auto pos : folly::irange(0, header.size())) {
    colWidths_[pos] =
        std::max(colWidths_[pos], (uint32_t)header.at(pos).size());
  }
  header_ = header;
  return *this;
}

template <>
std::string Tabulator::toString(const std::string& data) {
  return data;
}

template <>
std::string Tabulator::toString(const folly::dynamic& data) {
  if (data.isString() || data.isNumber() || data.isBool()) {
    return data.asString();
  }
  if (data.empty()) {
    return "";
  }
  return folly::toJson(data);
}

// explicit instantiation of addRow for some most common types
template Tabulator& Tabulator::addRow<std::initializer_list<int>>(
    const std::initializer_list<int>& columns);

template Tabulator& Tabulator::addRow<std::initializer_list<double>>(
    const std::initializer_list<double>& columns);

template Tabulator& Tabulator::addRow<std::initializer_list<std::string>>(
    const std::initializer_list<std::string>& columns);

void Tabulator::tabulate(fmt::memory_buffer& buf) {
  if (rows_.empty()) {
    return;
  }

  uint32_t totalColWidth = 3 * colWidths_.size() + 1;
  for (auto [_, colWidth] : colWidths_) {
    totalColWidth += colWidth;
  }

  auto beginRow = [&buf, totalColWidth, this]() {
    fmt::format_to(std::back_inserter(buf), "\n");
    if (options_.rowSeparator) {
      fmt::format_to(
          std::back_inserter(buf),
          "{}\n",
          std::string(totalColWidth, *options_.rowSeparator));
    }
  };
  auto beginCol = [&buf, this]() {
    if (options_.colSeparator) {
      fmt::format_to(std::back_inserter(buf), "{}", *options_.colSeparator);
    }
  };

  if (header_.size() > 0) {
    beginRow();
    for (const auto pos : folly::irange(header_.size())) {
      beginCol();
      fmt::format_to(
          std::back_inserter(buf),
          "{:<{}}",
          header_.at(pos),
          colWidths_.at(pos) + 2);
    }
    beginCol();
  }

  for (auto& row : rows_) {
    beginRow();
    for (const auto pos : folly::irange(0, row.size())) {
      beginCol();
      fmt::format_to(
          std::back_inserter(buf),
          "{:<{}}",
          row.at(pos),
          colWidths_.at(pos) + 2);
    }
    beginCol();
  }
  beginRow();
}

std::string Tabulator::string() {
  fmt::memory_buffer buf;
  tabulate(buf);
  return fmt::to_string(buf);
}

} // namespace rebalancer
} // namespace facebook
