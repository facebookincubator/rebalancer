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

#include "algopt/rebalancer/solver/moves/InvalidMoveFilter.h"

namespace facebook::rebalancer {

InvalidMoveFilter::InvalidMoveFilter(size_t numObjects, size_t numContainers)
    : containerToInvalidObjects_(numContainers), numObjects_(numObjects) {}

void InvalidMoveFilter::markInvalid(
    entities::ObjectId objectId,
    entities::ContainerId containerId) {
  auto& row = containerToInvalidObjects_[containerId.asIndex()];
  if (!row.has_value()) {
    row.emplace(numObjects_);
  }
  row->set(objectId.asIndex());
  isEmpty_ = false;
}

bool InvalidMoveFilter::empty() const {
  return isEmpty_;
}

} // namespace facebook::rebalancer
