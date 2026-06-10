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

#include "algopt/rebalancer/entities/builders/AsyncValueMap.h"
#include "algopt/rebalancer/entities/builders/IdBuilder.h"
#include "algopt/rebalancer/entities/Goal.h"
#include "algopt/rebalancer/entities/Identifiers.h"

#include <folly/container/F14Map.h>
#include <folly/coro/Collect.h>
#include <folly/coro/Task.h>
#include <folly/futures/SharedPromise.h>

namespace facebook::rebalancer::entities {

struct GoalData {
  std::shared_ptr<entities::Goal> goal;
};

struct GoalsBuilderResult {
  Map<std::string, GoalId> goalNameToId;
  std::map<GoalId, std::shared_ptr<Goal>> goalIdToGoal;
};

class GoalsBuilder {
 public:
  explicit GoalsBuilder(IdBuilder& idBuilder) : idBuilder_(idBuilder) {}

  folly::coro::Task<void> add(GoalId goalId, GoalData goalData) {
    co_return goalIdToData_.set(goalId, std::move(goalData));
  }

  GoalId makeGoalId(const std::string& name);

  folly::coro::Task<GoalsBuilderResult> build() const;

  folly::coro::Task<std::string> summarize() const;

 private:
  IdBuilder& idBuilder_;
  AsyncValueMap<GoalId, entities::GoalData> goalIdToData_;
  folly::Synchronized<Map<std::string, GoalId>> goalNameToId_;
};

} // namespace facebook::rebalancer::entities
