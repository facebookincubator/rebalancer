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
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/Partition.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"

#include <memory>

namespace facebook::rebalancer::entities {

class Partitions {
 public:
  Partitions() = default;
  explicit Partitions(Map<PartitionId, std::shared_ptr<Partition>> partitions);
  explicit Partitions(const thrift::Partitions& partitions);

  // Delete the copy and assignment constructors to prevent accidental copies.
  Partitions(const Partitions&) = delete;
  Partitions& operator=(const Partitions&) = delete;
  // Other operators are as usual.
  Partitions(Partitions&&) = default;
  Partitions& operator=(Partitions&&) = default;
  ~Partitions() = default;

  auto getPartitionIds() const {
    return std::views::keys(partitions_);
  }

  const Partition& getPartition(PartitionId partitionId) const;
  std::shared_ptr<const Partition> getPartitionPtr(
      PartitionId partitionId) const;

  thrift::Partitions toThrift() const;

 private:
  Map<PartitionId, std::shared_ptr<Partition>> partitions_;
};

} // namespace facebook::rebalancer::entities
