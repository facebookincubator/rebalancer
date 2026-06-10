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

#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSolver_types.h"

namespace facebook {
namespace rebalancer {
namespace entities {

class Goal {
 public:
  // Constructor.
  Goal(
      facebook::rebalancer::interface::GoalSpecs spec,
      double weight,
      int tupleIndex);
  explicit Goal(const thrift::Goal& goal);

  // Delete the copy and assignment constructors to prevent accidental copies.
  Goal(const Goal&) = delete;
  Goal& operator=(const Goal&) = delete;
  // Other operators are as usual.
  Goal(Goal&&) = default;
  Goal& operator=(Goal&&) = default;
  ~Goal() = default;

  // Getters.
  const facebook::rebalancer::interface::GoalSpecs& getSpec() const;
  double getWeight() const;
  int getTupleIndex() const;

  thrift::Goal toThrift() const;

 private:
  facebook::rebalancer::interface::GoalSpecs spec_;
  double weight_;
  int tupleIndex_;
};

} // namespace entities
} // namespace rebalancer
} // namespace facebook
