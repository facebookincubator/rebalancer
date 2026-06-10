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

#include "algopt/rebalancer/entities/Constraint.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/thrift/gen-cpp2/Entities_types.h"

#include <memory>

namespace facebook::rebalancer::entities {

class Constraints {
 public:
  Constraints() = default;
  explicit Constraints(
      std::map<ConstraintId, std::shared_ptr<Constraint>> constraints);

  explicit Constraints(const thrift::Constraints& constraints);

  // Delete the copy and assignment constructors to prevent accidental copies.
  Constraints(const Constraints&) = delete;
  Constraints& operator=(const Constraints&) = delete;
  // Other operators are as usual.
  Constraints(Constraints&&) = default;
  Constraints& operator=(Constraints&&) = default;
  ~Constraints() = default;

  // Getters.
  auto getConstraintIds() const {
    return std::views::keys(constraints_);
  }

  const Constraint& getConstraint(ConstraintId constraintId) const;
  int getMaxTupleIndex() const;

  thrift::Constraints toThrift() const;

 private:
  // maintain std::map to maintain the order of ids (which corresponds to the
  // order in which they were added)
  std::map<ConstraintId, std::shared_ptr<Constraint>> constraints_;
};

} // namespace facebook::rebalancer::entities
