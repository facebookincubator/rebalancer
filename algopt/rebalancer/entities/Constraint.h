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

class Constraint {
 public:
  // Constructor.
  Constraint(
      facebook::rebalancer::interface::ConstraintSpecs spec,
      facebook::rebalancer::interface::ConstraintPolicy policy,
      double invalidCost,
      double invalidState,
      int tuplePosIfBroken);
  explicit Constraint(const thrift::Constraint& constraint);

  // Delete the copy and assignment constructors to prevent accidental copies.
  Constraint(const Constraint&) = delete;
  Constraint& operator=(const Constraint&) = delete;
  // Other operators are as usual.
  Constraint(Constraint&&) = default;
  Constraint& operator=(Constraint&&) = default;
  ~Constraint() = default;

  // Getters.
  const facebook::rebalancer::interface::ConstraintSpecs& getSpec() const;
  interface::ConstraintPolicy getPolicy() const;
  double getInvalidCost() const;
  double getInvalidState() const;
  int getTupleIndex() const;

  thrift::Constraint toThrift() const;

 private:
  facebook::rebalancer::interface::ConstraintSpecs spec_;
  facebook::rebalancer::interface::ConstraintPolicy policy_;
  double invalidCost_;
  double invalidState_;
  int tuplePosIfBroken_;
};

} // namespace entities
} // namespace rebalancer
} // namespace facebook
