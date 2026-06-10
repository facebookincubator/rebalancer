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

#include "algopt/rebalancer/materializer/spec_builder/SpecBuilder.h"

#include "algopt/rebalancer/solver/expressions/Operators.h"

namespace facebook::rebalancer::materializer {

SpecBuilder::SpecBuilder(std::shared_ptr<const entities::Universe> universe)
    : universe_(std::move(universe)) {}

SpecBuilder::~SpecBuilder() = default;

entities::Map<entities::ObjectId, entities::ContainerId>
SpecBuilder::getUpdatesInInitialAssignment() const {
  /* Return the updates in initial assignment */

  // all spec dont have to override this, hence default there
  // are no updates to initial assignment.
  return {};
}

entities::Set<entities::ObjectId> SpecBuilder::fixedObjects() const {
  return {};
}

entities::Set<entities::ContainerId> SpecBuilder::nonAcceptingContainers()
    const {
  return {};
}

void SpecBuilder::populateInvalidMoveFilter(
    InvalidMoveFilter& /*filter*/) const {}

/*static*/
ExprPtr SpecBuilder::getAggregatedConstraintViolation(
    const std::vector<ConstraintInfo>& constraints,
    std::shared_ptr<const entities::Universe> universe) {
  auto aggregatedViolation = const_expr(0, universe);
  for (auto& constraint : constraints) {
    inplace_add(
        aggregatedViolation, getConstraintViolation(constraint), universe);
  }

  return aggregatedViolation;
}

/*static*/
ExprPtr SpecBuilder::getConstraintViolation(const ConstraintInfo& constraint) {
  auto& [constraintExpr, additionalPenaltyExpr] = constraint;
  if (constraintExpr == nullptr) {
    throw std::runtime_error("Constraint expression is not set");
  }
  auto universe = constraintExpr->getUniversePtr();
  static const auto kZero = const_expr(0, universe);
  if (!additionalPenaltyExpr) {
    return max(kZero, constraintExpr, universe);
  }
  // If additionalPenalty is specified, then total penalty is
  // = max(0, constraintExpr) + step(constraintExpr) * additionalPenalty
  // Also, while we don't expect additionalPenalty to be used when using optimal
  // solver, even if we do, it is fine because the product below can be
  // converted to its lp form since one of the children is binary
  return max(kZero, constraintExpr, universe) +
      product(step(constraintExpr, universe), additionalPenaltyExpr, universe);
}

} // namespace facebook::rebalancer::materializer
