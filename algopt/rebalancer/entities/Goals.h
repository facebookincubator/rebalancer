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

#include "algopt/rebalancer/entities/Goal.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"

#include <memory>
#include <vector>

namespace facebook::rebalancer::entities {

class Goals {
 public:
  Goals() = default;
  explicit Goals(std::map<GoalId, std::shared_ptr<Goal>> goals);
  explicit Goals(const thrift::Goals& goals);

  // Delete the copy and assignment constructors to prevent accidental copies.
  Goals(const Goals&) = delete;
  Goals& operator=(const Goals&) = delete;
  // Other operators are as usual.
  Goals(Goals&&) = default;
  Goals& operator=(Goals&&) = default;
  ~Goals() = default;

  // Getters.
  auto getGoalIds() const {
    return std::views::keys(goals_);
  }

  const Goal& getGoal(GoalId goalId) const;
  const std::vector<GoalId>& getGoalIds(int tupleIndex) const;
  int getTupleSize() const;

  thrift::Goals toThrift() const;

 private:
  // maintain std::map to maintain the order of ids (which corresponds to the
  // order in which they were added)
  std::map<GoalId, std::shared_ptr<Goal>> goals_;
  Map<int, std::vector<GoalId>> indexedGoalIds_;
  int tupleSize_ = 0;
};

} // namespace facebook::rebalancer::entities
