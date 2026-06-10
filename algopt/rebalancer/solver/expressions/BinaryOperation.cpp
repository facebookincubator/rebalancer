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

#include "algopt/rebalancer/solver/expressions/BinaryOperation.h"

#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/equivalence_sets/EquivalenceSets.h"

namespace facebook::rebalancer {

BinaryOperation::BinaryOperation(
    std::shared_ptr<Expression> expr1,
    std::shared_ptr<Expression> expr2,
    std::shared_ptr<const entities::Universe> universe)
    : Expression(std::move(universe)) {
  child1st = expr1;
  child2nd = expr2;
  add_child(std::move(expr1));
  add_child(std::move(expr2));
}

void BinaryOperation::updateEquivalenceSets(
    EquivalenceSets& equivalenceSets) const {
  for (const auto& child : children()) {
    if (auto var = child->getVar()) {
      auto [obj, cont] = *var;
      equivalenceSets.mappingMerge(
          PackerMap<entities::ObjectId, int>({{obj, 1}}));
    }
  }
}

Bounds BinaryOperation::innerLowerAndUpperBounds(
    Context& context,
    const BoundConstraints& bc) const {
  auto candidates = bound_candidates(context, bc);
  return {
      .lower_bound = *std::min_element(candidates.begin(), candidates.end()),
      .upper_bound = *std::max_element(candidates.begin(), candidates.end())};
}

} // namespace facebook::rebalancer
