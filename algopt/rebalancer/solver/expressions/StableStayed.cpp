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

#include "algopt/rebalancer/solver/expressions/StableStayed.h"

#include "algopt/lp/generic/Operators.h"
#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"
#include "algopt/rebalancer/solver/expressions/ObjectVector.h"
#include "algopt/rebalancer/solver/utils/equivalence_sets/EquivalenceSets.h"
#include "algopt/rebalancer/solver/utils/Problem.h"

namespace {
constexpr std::string_view type = "StableStayed";
}

/*
 * StableStayed is a special case of LOOKUP expression with some caveats
 * In contrast to LOOKUP that performs lookup on specified scopeItem (container
 * set C) using an object vector V of dimension values, StableStayed uses two
 * object vectors:
 * - FullObjectVector : V with dimension value d_i of object i
 * - InitialObjectVector : V_0 this is the object vector with value 0 for all
 *                        objects i initially not in C and value v_i otherwise
 *
 * We use the initial vector V_0 to perform LOOKUP operations
 * However, to update the equivalence sets, we use the full object vector V
 * The reason we do this is because had we split the sets using vector V_0,
 * then we will create way too many equivalent sets. Specifically, any
 * equivalent set S that has objects in k scope items will need to be split in k
 * subsets
 */
namespace facebook::rebalancer {

StableStayed::StableStayed(
    std::shared_ptr<ObjectVector> initialObjectVector,
    std::shared_ptr<ObjectVector> fullObjectVector,
    std::shared_ptr<const PackerSet<entities::ContainerId>> containersPtr,
    std::shared_ptr<const entities::Universe> universe,
    const Assignment& initialAssignment)
    : ObjectLookup(
          std::move(initialObjectVector),
          std::move(containersPtr),
          std::move(universe),
          initialAssignment),
      fullObjectVector_(std::move(fullObjectVector)) {}

const std::string_view& StableStayed::getType() const {
  return type;
}

void StableStayed::updateEquivalenceSets(
    EquivalenceSets& equivalenceSets) const {
  // Update equivalence sets using fullObjectVector_. This ensures that the
  // equivalent sets are not split into too many subsets.
  fullObjectVector_->passiveUpdateEquivalenceSets(equivalenceSets);
}

algopt::lp::Expression StableStayed::lp(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& /* configs */) {
  auto& problem = evaluator.getProblem();
  auto expr = evaluator.makeLpExpression();
  const auto equivSetCount =
      object_vector->getEquivSetCount(problem, *containersPtr_);
  for (auto& [equivSet, value] : evaluator.getEquivSetMap(object_vector)) {
    auto afterValue = evaluator.makeLpExpression();
    for (auto container : *containersPtr_) {
      afterValue += evaluator.getAssignmentVar(equivSet, container);
    }
    // get min(afterValue, initialValue)
    //
    // ub == equiv set size
    // initialValue should be initial sum of objects on this container
    const double initialValue = equivSetCount.at(equivSet);
    auto minVar = lp_cont_var(evaluator);
    REBALANCER_NEWCTR(minVar <= initialValue);
    REBALANCER_NEWCTR(minVar <= afterValue);
    expr += value * minVar;

    if (minimizing) {
      auto b = lp_bool_var(evaluator);
      const double ub = problem.getEquivalenceSets().getSet(equivSet).size();
      const double bigM = std::min(-initialValue, initialValue - ub);
      REBALANCER_NEWCTR(minVar >= initialValue + bigM * b);
      REBALANCER_NEWCTR(minVar >= afterValue + bigM * (1 - b));
    }
  }
  return expr;
}

bool StableStayed::hasNoLpIntent() const {
  return true;
}

bool StableStayed::needsEquivalenceSetBasedPostProcessing() const {
  // Since we do not split equivalent sets based on initialObjectvector_ (which
  // is the object vector we use for evaluate, apply and build MIP model), we
  // need to correctly assign objects to containers in a post-processing step.

  // For the postprocessing to work, we need to add C = *containersPtr_ to
  // equivalenceSets as we need to let equivalenceSets know when it is about
  // to derive object to container from object_count_of_equivalent_set to
  // container.
  // 1. it should prefer containers in C, as they are in stablestayed
  // 2. containers in C are equivalent, ok to pick any of initial objects from
  // these containers
  return true;
}

} // namespace facebook::rebalancer
