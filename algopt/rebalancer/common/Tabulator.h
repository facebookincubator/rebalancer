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

#include <fmt/format.h>
#include <folly/json/dynamic.h>

#include <string>
#include <vector>

namespace facebook {
namespace rebalancer {

class Tabulator {
 public:
  struct Options {
    std::optional<char> rowSeparator;
    std::optional<char> colSeparator;
  };

 public:
  explicit Tabulator(Options options) : options_(std::move(options)) {}

  Tabulator& setHeader(const std::vector<std::string>& header);
  template <typename T>
  Tabulator& addRow(const T& columns);
  void tabulate(fmt::memory_buffer& buf);
  std::string string();

 private:
  template <typename T>
  std::string toString(const T& data);

 private:
  Options options_;
  std::vector<std::string> header_;
  std::vector<std::vector<std::string>> rows_;
  std::unordered_map<int, uint32_t> colWidths_;
};

} // namespace rebalancer
} // namespace facebook

template <>
std::string facebook::rebalancer::Tabulator::toString(const std::string& data);
template <>
std::string facebook::rebalancer::Tabulator::toString(
    const folly::dynamic& data);

#include "algopt/rebalancer/common/Tabulator-inl.h"
