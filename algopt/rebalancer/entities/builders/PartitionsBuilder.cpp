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

#include "algopt/rebalancer/entities/builders/PartitionsBuilder.h"

#include <fmt/core.h>

namespace facebook::rebalancer::entities {

namespace {
constexpr static std::string_view kMakePartitionId{"makePartitionId"};
}

PartitionId PartitionsBuilder::makePartitionId(
    const std::string& partitionName) {
  return idBuilder_.makeIdFromName(
      partitionName, partitionNameToId_, partitionIdToData_, kMakePartitionId);
}

folly::coro::Task<PartitionsBuilderResult> PartitionsBuilder::build() const {
  PartitionsBuilderResult result;
  co_await partitionIdToData_.waitAndReadFromEach(
      [&](PartitionId id, const PartitionData& data) {
        result.partitionIdToGroupNameToId.emplace(id, data.groupNameToId);
        result.partitionIdToPartition.emplace(id, data.partition);
      });
  result.partitionNameToId = *partitionNameToId_.rlock();

  co_return result;
}

folly::coro::Task<std::string> PartitionsBuilder::summarize() const {
  const auto partitionNames = partitionNameToId_.rlock();
  if (partitionNames->empty()) {
    co_return "";
  }
  std::string result = "Partitions:\n";
  for (const auto& [partitionName, partitionId] : *partitionNames) {
    const auto data = co_await partitionIdToData_.get(partitionId);
    result += fmt::format("  {} [", partitionName);
    for (const auto& [groupName, _] : *data->groupNameToId) {
      result += fmt::format(" {}", groupName);
    }
    result += " ]\n";
  }
  co_return result;
}

} // namespace facebook::rebalancer::entities
