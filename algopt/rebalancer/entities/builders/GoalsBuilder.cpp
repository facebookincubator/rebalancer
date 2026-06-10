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

#include "algopt/rebalancer/entities/builders/GoalsBuilder.h"

#include "algopt/rebalancer/entities/builders/BuilderUtils.h"

namespace facebook::rebalancer::entities {

namespace {
constexpr static std::string_view kMakeGoalId{"makeGoalId"};
}

GoalId GoalsBuilder::makeGoalId(const std::string& goalName) {
  return idBuilder_.makeIdFromName(
      goalName, goalNameToId_, goalIdToData_, kMakeGoalId);
}

folly::coro::Task<GoalsBuilderResult> GoalsBuilder::build() const {
  GoalsBuilderResult result;
  co_await goalIdToData_.waitAndReadFromEach(
      [&](GoalId id, const GoalData& data) {
        result.goalIdToGoal.emplace(id, data.goal);
      });
  result.goalNameToId = *goalNameToId_.rlock();

  co_return result;
}

folly::coro::Task<std::string> GoalsBuilder::summarize() const {
  co_return summarizeNames("Goals", *goalNameToId_.rlock());
}

} // namespace facebook::rebalancer::entities
