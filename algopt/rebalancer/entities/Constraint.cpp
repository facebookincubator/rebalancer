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

#include "algopt/rebalancer/entities/Constraint.h"

namespace facebook {
namespace rebalancer {
namespace entities {

using namespace rebalancer::interface;

Constraint::Constraint(
    ConstraintSpecs spec,
    ConstraintPolicy policy,
    double invalidCost,
    double invalidState,
    int tuplePosIfBroken)
    : spec_(std::move(spec)),
      policy_(policy),
      invalidCost_(invalidCost),
      invalidState_(invalidState),
      tuplePosIfBroken_(tuplePosIfBroken) {}

Constraint::Constraint(const thrift::Constraint& constraint)
    : spec_(*constraint.spec()),
      policy_(*constraint.policy()),
      invalidCost_(*constraint.invalidCost()),
      invalidState_(*constraint.invalidState()),
      tuplePosIfBroken_(*constraint.tuplePosIfBroken()) {}

const ConstraintSpecs& Constraint::getSpec() const {
  return spec_;
}

ConstraintPolicy Constraint::getPolicy() const {
  return policy_;
}

double Constraint::getInvalidCost() const {
  return invalidCost_;
}

double Constraint::getInvalidState() const {
  return invalidState_;
}

int Constraint::getTupleIndex() const {
  return tuplePosIfBroken_;
}

thrift::Constraint Constraint::toThrift() const {
  thrift::Constraint constraint;
  constraint.spec() = spec_;
  constraint.policy() = policy_;
  constraint.invalidCost() = invalidCost_;
  constraint.invalidState() = invalidState_;
  constraint.tuplePosIfBroken() = tuplePosIfBroken_;
  return constraint;
}

} // namespace entities
} // namespace rebalancer
} // namespace facebook
