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

#include "algopt/rebalancer/entities/Partitions.h"

#include "algopt/rebalancer/common/CoroUtils.h"

#include <folly/coro/BlockingWait.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/system/HardwareConcurrency.h>

namespace facebook::rebalancer::entities {

Partitions::Partitions(Map<PartitionId, std::shared_ptr<Partition>> partitions)
    : partitions_(std::move(partitions)) {}

Partitions::Partitions(const thrift::Partitions& partitions) {
  const auto& thriftPartitions = *partitions.partitions();
  partitions_.reserve(thriftPartitions.size());
  folly::coro::blockingWait(
      CoroUtils::runEachFuncAndUpdate(
          thriftPartitions.begin(),
          thriftPartitions.end(),
          [](const auto& iter) {
            return std::make_unique<Partition>(iter->second);
          },
          [&](auto&& partition, const auto& iter) {
            partitions_.emplace(PartitionId(iter->first), std::move(partition));
          },
          std::make_shared<folly::CPUThreadPoolExecutor>(
              folly::available_concurrency())));
}

const Partition& Partitions::getPartition(PartitionId partitionId) const {
  return *partitions_.at(partitionId);
}

std::shared_ptr<const Partition> Partitions::getPartitionPtr(
    PartitionId partitionId) const {
  return partitions_.at(partitionId);
}

thrift::Partitions Partitions::toThrift() const {
  thrift::Partitions partitions;
  auto& thriftPartitions = *partitions.partitions();
  thriftPartitions.reserve(partitions_.size());
  for (const auto& [partitionId, partition] : partitions_) {
    thriftPartitions.emplace(partitionId.asInt(), partition->toThrift());
  }
  return partitions;
}

} // namespace facebook::rebalancer::entities
