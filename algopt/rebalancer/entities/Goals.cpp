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

#include "algopt/rebalancer/entities/Goals.h"

#include <folly/container/MapUtil.h>

#include <algorithm>

namespace facebook::rebalancer::entities {

const std::vector<GoalId> kEmptyVector;

using namespace facebook::rebalancer::interface;

Goals::Goals(std::map<GoalId, std::shared_ptr<Goal>> goals)
    : goals_(std::move(goals)) {
  for (const auto& [goalId, goal] : goals_) {
    const auto tupleIndex = goal->getTupleIndex();
    indexedGoalIds_[tupleIndex].push_back(goalId);
    tupleSize_ = std::max(tupleSize_, tupleIndex + 1);
  }
}

Goals::Goals(const thrift::Goals& goals) {
  for (auto& [goalId, goal] : *goals.goals()) {
    goals_.emplace(GoalId(goalId), std::make_unique<Goal>(goal));
    auto tupleIndex = *goal.tupleIndex();
    indexedGoalIds_[tupleIndex].emplace_back(goalId);
    tupleSize_ = std::max(tupleSize_, tupleIndex + 1);
  }
}

const std::vector<GoalId>& Goals::getGoalIds(int tupleIndex) const {
  return folly::get_ref_default(indexedGoalIds_, tupleIndex, kEmptyVector);
}

const Goal& Goals::getGoal(GoalId goalId) const {
  return *goals_.at(goalId);
}

int Goals::getTupleSize() const {
  return tupleSize_;
}

thrift::Goals Goals::toThrift() const {
  thrift::Goals goals;
  auto& thriftGoals = *goals.goals();
  thriftGoals.reserve(goals_.size());
  for (const auto& [goalId, goal] : goals_) {
    thriftGoals.emplace(goalId.asInt(), goal->toThrift());
  }
  return goals;
}

} // namespace facebook::rebalancer::entities
