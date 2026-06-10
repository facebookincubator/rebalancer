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

#include "algopt/rebalancer/solver/expressions/Variable.h"

#include "algopt/lp/generic/Operators.h"
#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"
#include "algopt/rebalancer/solver/expressions/PropertiesHelper.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/iterators/StlWrapper.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/Change.h"
#include "algopt/rebalancer/solver/utils/Problem.h"

#include <sstream>

namespace {
constexpr std::string_view type = "Variable";
}

namespace facebook::rebalancer {

Variable::Variable(
    entities::ObjectId obj,
    entities::ContainerId con,
    std::shared_ptr<const entities::Universe> universe,
    const Assignment& initialAssignment)
    : Expression(std::move(universe)), object(obj), container(con) {
  set_directly_affected_containers();
  setInitialValue(applyAssignment(initialAssignment));
}

double Variable::applyAssignment(const Assignment& assignment) const {
  return assignment.getContainer(object) == container ? 1.0 : 0.0;
}

const std::string_view& Variable::getType() const {
  return type;
}

void Variable::updateEquivalenceSets(EquivalenceSets& /* unused */) const {}

Bounds Variable::innerLowerAndUpperBounds(
    Context& /* unused */,
    const BoundConstraints& bc) const {
  if (bc.isEmpty()) {
    return {.lower_bound = 0, .upper_bound = 1};
  }

  return {
      // bc is set, so if the container is not "giving", return current value
      .lower_bound = bc.giving(container) ? 0 : value,
      // bc is set, so if the container is not "taking", return current value
      .upper_bound = bc.taking(container) ? 1 : value};
}

std::optional<std::pair<entities::ObjectId, entities::ContainerId>>
Variable::getVar() const {
  return std::make_pair(object, container);
}

bool Variable::inner_is_integer(Context& /* not used */) {
  return true;
}

double Variable::evaluate(
    const BottomToTopEvaluator& /* evaluator */,
    const ChangeSet& changes) const {
  double val = value;
  for (auto& change : changes) {
    if (change.getObject() == object && change.getContainer() == container) {
      val += change.getValue();
    }
  }
  return val;
}

double Variable::innerFullApply(
    const TopToBottomEvaluator& /* evaluator */,
    const Assignment& assignment) {
  return applyAssignment(assignment);
}

double Variable::innerPartialApply(
    const BottomToTopEvaluator& /* unused */,
    const Assignment& assignment,
    const ChangeSet& /* unused */) {
  value = assignment.getContainer(object) == container ? 1.0 : 0.0;
  return value;
}

algopt::lp::Expression Variable::lp(
    const LpEvaluator& evaluator,
    bool /* minimizing */,
    const interface::OptimalSolverSpec& /* configs */) {
  auto& problem = evaluator.getProblem();
  auto equivSetId = problem.getEquivalenceSets().at(object);
  auto numObjects = problem.getEquivalenceSets().getSet(equivSetId).size();

  // (DEF) Two objects are equivalent if they have the same coefficient
  // everywhere in the generated LP model.
  // Therefore, we can assume that every constraint or objective expression must
  // either have all or none of the objects for any given equivalent set.
  // Because if not, then we will have a constraint where some objects do not
  // exist (coef zero) and some do exist (coef non-zero), which will violate the
  // definition DEF of equivalent sets.

  // Since all objects must exist in the expression wherever this variable is
  // used we can assume that contribution of each object (coef) is the same.
  const double coef = 1.0 / numObjects;
  return evaluator.getAssignmentVar(equivSetId, container) * coef;
}

std::string Variable::innerDigest(size_t /*maxChildren*/) const {
  return fmt::format(
      "object:{}, container:{}",
      universe_->getEntityName(object),
      universe_->getEntityName(container));
}

void Variable::set_directly_affected_containers() {
  directlyAffectedContainers.set(
      std::make_shared<const PackerSet<entities::ContainerId>>(
          PackerSet<entities::ContainerId>{container}));
}

AbstractContainer<ObjectPotential> Variable::getObjectPotentials(
    bool /* descending */) const {
  std::vector<ObjectPotential> potentials;
  if (value != 0) {
    potentials.push_back(
        ObjectPotential{.objectId = object, .potential = value});
  }
  return makeStlWrapperContainer(std::move(potentials));
}

ExpressionProperties Variable::getProperties() const {
  ExpressionProperties properties;
  properties.properties()->emplace(
      "object", PropertiesHelper::makeObjectIdValue(object));
  properties.properties()->emplace(
      "container", PropertiesHelper::makeContainerIdValue(container));
  return properties;
}

std::optional<AffectedByChange> Variable::isAffectedByChange(
    const AffectedByChangeDecisionData& /*data*/) const {
  return AffectedByChange(
      directlyAffectedContainers.getSetPtr(),
      std::make_shared<const entities::Set<entities::ObjectId>>(
          entities::Set<entities::ObjectId>{object}));
}

bool Variable::hasNoLpIntent() const {
  return true;
}

} // namespace facebook::rebalancer
