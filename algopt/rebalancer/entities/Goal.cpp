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

#include "algopt/rebalancer/entities/Goal.h"

namespace facebook {
namespace rebalancer {
namespace entities {

using namespace facebook::rebalancer::interface;

Goal::Goal(GoalSpecs spec, double weight, int tupleIndex)
    : spec_(std::move(spec)), weight_(weight), tupleIndex_(tupleIndex) {}

Goal::Goal(const thrift::Goal& goal)
    : spec_(*goal.spec()),
      weight_(*goal.weight()),
      tupleIndex_(*goal.tupleIndex()) {}

const GoalSpecs& Goal::getSpec() const {
  return spec_;
}

double Goal::getWeight() const {
  return weight_;
}

int Goal::getTupleIndex() const {
  return tupleIndex_;
}

thrift::Goal Goal::toThrift() const {
  thrift::Goal goal;
  goal.spec() = spec_;
  goal.weight() = weight_;
  goal.tupleIndex() = tupleIndex_;
  return goal;
}

} // namespace entities
} // namespace rebalancer
} // namespace facebook
