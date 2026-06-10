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

#include "algopt/rebalancer/solver/expressions/ObjectLookup.h"

#include "algopt/lp/generic/Operators.h"
#include "algopt/rebalancer/solver/expressions/BottomToTopEvaluator.h"
#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"
#include "algopt/rebalancer/solver/expressions/ObjectVector.h"
#include "algopt/rebalancer/solver/expressions/PropertiesHelper.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/iterators/StlWrapper.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"
#include "algopt/rebalancer/solver/utils/BoundConstraints.h"
#include "algopt/rebalancer/solver/utils/Change.h"
#include "algopt/rebalancer/solver/utils/ChangeSet.h"
#include "algopt/rebalancer/solver/utils/equivalence_sets/EquivalenceSets.h"

#include <fmt/format.h>
#include <folly/container/Enumerate.h>
#include <folly/String.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {
constexpr std::string_view type = "ObjectLookup";
}

/*
 * From mathematical perspective, ObjectLookup is a linearsum
 * The difference is:
 * It is implemented in an efficient way to save memory and complexity
 * among those who share the same linearsum equation
 */
namespace facebook::rebalancer {

ObjectLookup::ObjectLookup(
    std::shared_ptr<Expression> expr,
    std::shared_ptr<const PackerSet<entities::ContainerId>> containersPtr,
    std::shared_ptr<const entities::Universe> universe,
    const Assignment& initialAssignment)
    : Expression(std::move(universe)), containersPtr_(containersPtr) {
  if (containersPtr_ == nullptr) {
    throw std::runtime_error("Expected 'containerPtr' to be non-null");
  }

  add_child(expr);
  object_vector = (ObjectVector*)getOnlyChildRawPtr();
  set_directly_affected_containers();

  setInitialValue(applyAssignment(initialAssignment));
}

const std::string_view& ObjectLookup::getType() const {
  return type;
}

std::optional<AffectedByChange> ObjectLookup::isAffectedByChange(
    const AffectedByChangeDecisionData& data) const {
  const int numTotalObjects = data.numTotalObjects;
  const int numTotalContainers = data.numTotalContainers;
  bool useObjectsForAffectedByChangeIndex = false;
  auto directlyAffectedContainersPtr = directlyAffectedContainers.getSetPtr();
  // indexing by objects is only useful when default value is zero because we
  // can prune out objects that have default value zero as they do not
  // contribute to the lookup.
  if (object_vector->isZeroDefault() && numTotalObjects > 0 &&
      numTotalContainers > 0) {
    // Following code implements a simple heuristic to guess when
    // indexing by objects will be more efficient than indexing by containers
    // 1. Compute the ratio of lookup objects to total number of objects
    // 2. Compute the ratio of lookup containers to total number of containers
    const size_t nonDefaultCount = object_vector->nonDefaultCount();
    const double objectRatio = (nonDefaultCount * 1.0) / numTotalObjects;
    auto containerSize = directlyAffectedContainersPtr->size();
    const double containerRatio = (containerSize * 1.0) / numTotalContainers;
    if (objectRatio < containerRatio) {
      useObjectsForAffectedByChangeIndex = true;
    }
  }

  if (useObjectsForAffectedByChangeIndex) {
    ChangeFilterFn containerFilter = nullptr;
    if (static_cast<int>(directlyAffectedContainersPtr->size()) <
        numTotalContainers) {
      const auto containers = directlyAffectedContainersPtr.get();
      containerFilter = [containers](const Change& change) {
        return containers->contains(change.getContainer());
      };
    }
    return AffectedByChange(
        object_vector->nonDefaultObjectIds(), std::move(containerFilter));
  }

  ChangeFilterFn filter = nullptr;
  if (object_vector->isZeroDefault() &&
      static_cast<int>(object_vector->nonDefaultCount()) < numTotalObjects) {
    const auto* objVec = object_vector;
    filter = [objVec](const Change& change) {
      return objVec->getObjectValue(change.getObject()) != 0.0;
    };
  }
  return AffectedByChange(directlyAffectedContainersPtr, std::move(filter));
}

void ObjectLookup::updateEquivalenceSets(EquivalenceSets& equivSet) const {
  object_vector->passiveUpdateEquivalenceSets(equivSet);
}

std::vector<std::pair<entities::ObjectId, double>>
ObjectLookup::getObjectValuesFromObjectVectorAndUpdateObjectPotentials(
    const Assignment& assignment) {
  std::vector<std::pair<entities::ObjectId, double>> items;
  items.reserve(object_vector->nonDefaultCount());
  object_vector->forEachNonDefaultEntry(
      [&](entities::ObjectId objectId, double objectValue) {
        auto currContainer = assignment.getContainer(objectId);

        if (!containersPtr_->contains(currContainer)) {
          return;
        }

        items.emplace_back(objectId, objectValue);
        objectPotentials_.insert(
            ObjectPotential{.objectId = objectId, .potential = objectValue});
      });
  return items;
}

std::vector<std::pair<entities::ObjectId, double>>
ObjectLookup::getObjectValuesFromAssignmentAndUpdateObjectPotentials(
    const Assignment& assignment) {
  std::vector<std::pair<entities::ObjectId, double>> items;
  for (auto container : *containersPtr_) {
    auto& objects = assignment.getObjects(container);
    for (auto object : objects) {
      auto objectValue = getObjectValue(object);

      if (objectValue == 0.0) {
        continue;
      }

      items.emplace_back(object, objectValue);

      objectPotentials_.insert(
          ObjectPotential{.objectId = object, .potential = objectValue});
    }
  }
  return items;
}

bool ObjectLookup::inner_is_integer(Context& context) {
  return getOnlyChildRawPtr()->inner_is_integer(context);
}

// If the 'defaultValue_' in object_vector is zero, then 'objectValues_' in
// object_vector has all the objectIds we care about. Therefore, when either
// (nObjectsInObjectVector <= containersPtr_->size() OR
// (nObjectsInObjectVector <= nObjectsInContainers), where
// nObjectsInContainers = getCurrObjectsInContainersSize(assignment), then it
// is beneficial to look at the objects from object_vector directly. Note that
// (nObjectsInObjectVector <= containersPtr_->size()) is explicitly checked
// first, since computing nObjectsInContainers can be non-trivially expensive
// if the number of containers is large.
double ObjectLookup::applyAssignment(const Assignment& assignment) {
  objectPotentials_.clear();
  auto nObjectsInObjectVector = object_vector->nonDefaultCount();
  auto items = object_vector->isZeroDefault() &&
          (nObjectsInObjectVector <= containersPtr_->size() ||
           static_cast<int>(nObjectsInObjectVector) <=
               getCurrObjectsInContainersSize(assignment))
      ? getObjectValuesFromObjectVectorAndUpdateObjectPotentials(assignment)
      : getObjectValuesFromAssignmentAndUpdateObjectPotentials(assignment);
  values.init(std::move(items));
  value = values.query();
  return value;
}

double ObjectLookup::innerFullApply(
    const TopToBottomEvaluator& /* evaluator */,
    const Assignment& assignment) {
  return applyAssignment(assignment);
}

PackerMap<entities::ObjectId, int> ObjectLookup::getNetChangePerObject(
    const ChangeSet& changes) {
  PackerMap<entities::ObjectId, int> netChangePerObject;
  // To compute the net changes, we either directly use the changeSet, or
  // compute the changes through objects or containers, whichever is smaller.
  // ChangeSet is directly used only when the number of changes is fewer than
  // the number of relevant objects and containers.
  bool useLookupObjectsToComputeChanges = false;
  bool useChangeSetToComputeChanges = false;
  const auto isObjectVectorSparse = object_vector->isZeroDefault();
  const auto relevantObjectsSize = isObjectVectorSparse
      ? object_vector->nonDefaultCount()
      : object_vector->totalObjectCount();

  if (changes.size() <= relevantObjectsSize &&
      changes.size() <= containersPtr_->size()) {
    useChangeSetToComputeChanges = true;
  } else if (isObjectVectorSparse && changes.size() > relevantObjectsSize) {
    // If the defaultValue associated with the object_vector is zero, then we
    // know all the objects we care about. Therefore,  if changeSizeForObjects
    // <= changeSizeForContainers, then it is beneficial to iterate on the
    // changes w.r.t. objects rather changes w.r.t. containers. Note that we
    // only perform this optimization if change size is greater than the number
    // of relevant objects
    auto changeSizeForObjects = [&]() {
      size_t size = 0;
      object_vector->forEachNonDefaultEntry(
          [&](entities::ObjectId objectId, double /*value*/) {
            size += changes.getChangesByObject(objectId).size();
          });
      return size;
    };
    if (containersPtr_->size() <= relevantObjectsSize) {
      auto changeSizeForContainers =
          changes.getChangeSizeForContainers(*containersPtr_);
      if (static_cast<int>(relevantObjectsSize) < changeSizeForContainers &&
          changeSizeForObjects() <= changeSizeForContainers) {
        useLookupObjectsToComputeChanges = true;
      }
    } else {
      auto sizeForObjects = changeSizeForObjects();
      if (sizeForObjects <= static_cast<int>(containersPtr_->size()) ||
          sizeForObjects <=
              changes.getChangeSizeForContainers(*containersPtr_)) {
        useLookupObjectsToComputeChanges = true;
      }
    }
  }

  if (useChangeSetToComputeChanges) {
    for (auto& change : changes) {
      if (containersPtr_->contains(change.getContainer())) {
        netChangePerObject[change.getObject()] += change.getValue();
      }
    }
  } else if (useLookupObjectsToComputeChanges) {
    object_vector->forEachNonDefaultEntry(
        [&](entities::ObjectId objectId, double /*value*/) {
          auto& objectChanges = changes.getChangesByObject(objectId);
          for (auto& change : objectChanges) {
            if (containersPtr_->contains(change.getContainer())) {
              netChangePerObject[objectId] += change.getValue();
            }
          }
        });
  } else {
    for (auto containerId : *containersPtr_) {
      auto& containerChanges = changes.getChangesByContainer(containerId);
      for (auto& change : containerChanges) {
        netChangePerObject[change.getObject()] += change.getValue();
      }
    }
  }
  return netChangePerObject;
}

double ObjectLookup::innerPartialApply(
    const BottomToTopEvaluator& /* evaluator */,
    const Assignment& /* assignment */,
    const ChangeSet& changes) {
  const auto netChangePerObject = getNetChangePerObject(changes);
  for (auto& [object, changeVal] : netChangePerObject) {
    const double objectValue = getObjectValue(object);
    if (changeVal == 0 || objectValue == 0) {
      // If changeval = 0, then it is an internal change that has no effect on
      // the lookup value. For example, lookup containers are (0,1) and an
      // object moves from container 0 to container 1. No update is needed to
      // lookup value in this case.
      // Also, if the objectValue = 0, then it has no impact on the lookup
      // value.
      continue;
    }

    if (changeVal == 1) {
      values.update(object, objectValue);
      objectPotentials_.emplace(object, objectValue);
    } else if (changeVal == -1) {
      values.remove(object);
      objectPotentials_.erase({object, objectValue});
    } else {
      throw std::runtime_error(
          fmt::format("Invalid change value: {}", changeVal));
    }
  }
  value = values.query();
  return value;
}

Bounds ObjectLookup::innerLowerAndUpperBounds(
    Context& context,
    const BoundConstraints& bc) const {
  ObjectVector* child = (ObjectVector*)getOnlyChildRawPtr();
  if (bc.isEmpty() || child->has_negative_values()) {
    return child->lowerAndUpperBounds(context);
  }

  // NOTE(pavanka): could improve with has_negative_values check that is
  // restricted to this particular lookup instead of looking for having *any*
  // negative value in the object vector itself

  auto [childLb, childUb] = child->lowerAndUpperBounds(context);

  return {
      // bc is set and child does NOT have negative values: in this case, if
      // none of
      // the containers are "giving", then return current value of the lookup
      .lower_bound = bc.anyGiving(*containersPtr_) ? childLb : value,
      // bc is set and child does NOT have negative values: in this case, if
      // none of the containers are "taking", then return current value of the
      // lookup
      .upper_bound = bc.anyTaking(*containersPtr_) ? childUb : value};
}

algopt::lp::Expression ObjectLookup::lp(
    const LpEvaluator& evaluator,
    bool /* minimizing */,
    const interface::OptimalSolverSpec& /* configs */) {
  // Treat this as a constant value if container is static
  bool hasDynamicContainer = false;
  for (auto container : *containersPtr_) {
    if (evaluator.getDynamicContainers().contains(container)) {
      hasDynamicContainer = true;
      break;
    }
  }
  if (!hasDynamicContainer) {
    return evaluator.makeLpExpression(value);
  }

  auto expr = evaluator.makeLpExpression();
  double rawSum = 0;
  for (const auto& [equiv_set, coef] :
       evaluator.getEquivSetMap(object_vector)) {
    for (auto container : *containersPtr_) {
      if (auto fixedValue =
              evaluator.getMaybeFixedAssignmentValue(equiv_set, container)) {
        rawSum += coef * fixedValue.value();
      } else {
        expr += coef * evaluator.getAssignmentVar(equiv_set, container);
      }
    }
  }
  expr += rawSum;
  return expr;
}

std::vector<std::pair<Expression*, double>> ObjectLookup::get_sorted_children(
    bool /* unused */) const {
  return {};
}

bool ObjectLookup::shouldComputeDescendingChildPotentials() const {
  return false;
}

AbstractContainer<ObjectPotential> ObjectLookup::getObjectPotentials(
    bool descending) const {
  using Set = std::set<ObjectPotential>;
  if (descending) {
    return AbstractContainer<ObjectPotential>(
        std::make_shared<
            StlWrapperIterator<Set, typename Set::const_reverse_iterator>>(
            nullptr, objectPotentials_.rbegin()),
        std::make_shared<
            StlWrapperIterator<Set, typename Set::const_reverse_iterator>>(
            nullptr, objectPotentials_.rend()));
  }
  // ascending
  return AbstractContainer<ObjectPotential>(
      std::make_shared<StlWrapperIterator<Set, typename Set::const_iterator>>(
          nullptr, objectPotentials_.begin()),
      std::make_shared<StlWrapperIterator<Set, typename Set::const_iterator>>(
          nullptr, objectPotentials_.end()));
}

ExpressionProperties ObjectLookup::getProperties() const {
  ExpressionProperties properties;
  properties.properties()->emplace(
      "containers",
      PropertiesHelper::makeContainerIdListValue(*containersPtr_));
  return properties;
}

double ObjectLookup::evaluate(
    const BottomToTopEvaluator& /* evaluator */,
    const ChangeSet& changes) const {
  double delta = 0;
  const auto& objVec = *object_vector;
  if (changes.size() <= containersPtr_->size()) {
    for (const auto& change : changes) {
      if (containersPtr_->contains(change.getContainer())) {
        delta += change.getValue() * objVec.getObjectValue(change.getObject());
      }
    }
  } else {
    for (const auto containerId : *containersPtr_) {
      for (const auto& change : changes.getChangesByContainer(containerId)) {
        delta += change.getValue() * objVec.getObjectValue(change.getObject());
      }
    }
  }

  if (getPrecision().compare(value, -delta) == 0) {
    return 0;
  }
  return value + delta;
}

std::string ObjectLookup::innerDigest(size_t /* unused */) const {
  std::vector<std::string_view> names;
  names.reserve(containersPtr_->size());
  for (const auto& container : *containersPtr_) {
    names.push_back(universe_->getEntityName(container));
  }
  std::sort(names.begin(), names.end());
  return fmt::format("on Containers: {}", folly::join(", ", names));
}

void ObjectLookup::set_directly_affected_containers() {
  directlyAffectedContainers.set(containersPtr_);
}

double ObjectLookup::getObjectValue(entities::ObjectId objectId) const {
  return object_vector->getObjectValue(objectId);
}

int ObjectLookup::getCurrObjectsInContainersSize(
    const Assignment& assignment) const {
  int nObjectsInContainers = 0;
  for (auto container : *containersPtr_) {
    auto& objects = assignment.getObjects(container);
    nObjectsInContainers += objects.size();
  }
  return nObjectsInContainers;
}

bool ObjectLookup::hasNoLpIntent() const {
  return true;
}

} // namespace facebook::rebalancer
