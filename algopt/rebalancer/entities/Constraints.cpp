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

#include "algopt/rebalancer/entities/Constraints.h"

namespace facebook::rebalancer::entities {

using namespace rebalancer::interface;

Constraints::Constraints(
    std::map<ConstraintId, std::shared_ptr<Constraint>> constraints)
    : constraints_(std::move(constraints)) {}

Constraints::Constraints(const thrift::Constraints& constraints) {
  for (auto& [constraintId, constraint] : *constraints.constraints()) {
    constraints_.emplace(
        ConstraintId(constraintId), std::make_unique<Constraint>(constraint));
  }
}

const Constraint& Constraints::getConstraint(ConstraintId constraintId) const {
  return *constraints_.at(constraintId);
}

thrift::Constraints Constraints::toThrift() const {
  thrift::Constraints constraints;
  auto& thriftConstraints = *constraints.constraints();
  thriftConstraints.reserve(constraints_.size());
  for (const auto& [constraintId, constraint] : constraints_) {
    thriftConstraints.emplace(constraintId.asInt(), constraint->toThrift());
  }
  return constraints;
}

} // namespace facebook::rebalancer::entities
