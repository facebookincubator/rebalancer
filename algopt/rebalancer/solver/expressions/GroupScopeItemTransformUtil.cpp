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

#include "algopt/rebalancer/solver/expressions/GroupScopeItemTransformUtil.h"

#include "algopt/rebalancer/solver/expressions/LpEvaluator.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/PropertiesHelper.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/utils/Context.h"
#include "algopt/rebalancer/solver/utils/Problem.h"

#include <folly/container/irange.h>

#include <cstdint>
#include <stdexcept>

namespace facebook::rebalancer {

namespace {
constexpr std::string_view kType = "GroupScopeItemTransformUtil";

entities::ScopeItemId checkAndGetScopeItemId(
    const entities::Scope& scope,
    entities::ContainerId containerId) {
  auto maybeScopeItemId = scope.getScopeItemId(containerId);
  if (!maybeScopeItemId) {
    throw std::runtime_error("relevant container must exist in the scope");
  }
  return *maybeScopeItemId;
}

std::string getTransformFunctionDesc(
    GroupScopeItemTransformUtil::TransformFunctionType transform) {
  switch (transform) {
    case GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY:
      return "f(x) = x";
    case GroupScopeItemTransformUtil::TransformFunctionType::SQUARE:
      return "f(x) = x^2";
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP:
      return "step(x)";
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K:
      return "step(x mod k)";
    default:
      throw std::runtime_error("Unhandled TransformFunctionType");
  }
}

std::function<double(double, entities::ScopeItemId)> getTransformFunction(
    GroupScopeItemTransformUtil::TransformFunctionType transform,
    const TransformFunctionData& data,
    const entities::Universe& universe) {
  switch (transform) {
    case GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY:
      return
          [](double value, entities::ScopeItemId /*unused*/) { return value; };
    case GroupScopeItemTransformUtil::TransformFunctionType::SQUARE:
      return [](double value, entities::ScopeItemId /*unused*/) {
        return value * value;
      };
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP:
      return [&universe](double value, entities::ScopeItemId /*unused*/) {
        return universe.getPrecision().isStrictlyGtZero(value);
      };
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K:
      assert(data.kForModKTransform.has_value());
      return [&universe, k = *data.kForModKTransform](
                 double value, entities::ScopeItemId scopeItemId) {
        return universe.getPrecision().isStrictlyGtZero(
            std::fmod(value, k.get(scopeItemId)));
      };
    default:
      throw std::runtime_error("Unhandled TransformFunctionType");
  }
}

} // namespace

ModKTransformData::ModKTransformData(
    uint32_t defaultValue,
    entities::Map<entities::ScopeItemId, uint32_t> perScopeItemValue)
    : defaultK_(defaultValue),
      perScopeItemValue_(std::move(perScopeItemValue)) {
  const auto checkIfPositive = [](uint32_t k) {
    if (k == 0) {
      throw std::runtime_error("k must be non-zero");
    }
  };
  checkIfPositive(defaultValue);
  for (auto& [_, k] : perScopeItemValue_) {
    checkIfPositive(k);
  }
}

uint32_t ModKTransformData::get(entities::ScopeItemId scopeItemId) const {
  return folly::get_default(perScopeItemValue_, scopeItemId, defaultK_);
}

GroupScopeItemTransformUtil::GroupScopeItemTransformUtil(
    std::shared_ptr<const entities::Universe> universe,
    entities::PartitionId partitionId,
    entities::GroupId groupId,
    entities::DimensionId dimensionId,
    entities::ScopeId scopeId,
    const std::vector<entities::ScopeItemId>& allowedScopeItems,
    std::shared_ptr<const entities::Set<entities::ContainerId>> containersPtr,
    const Assignment& initialAssignment,
    folly::F14FastMap<entities::ScopeItemId, double> scopeItemWeights,
    double scopeItemDefaultWeight,
    TransformFunctionType transformType,
    double normalizationConstant,
    TransformFunctionData transformFunctionData)
    : Expression(std::move(universe)),
      universe_(getUniversePtr()),
      scopeId_(scopeId),
      scope_(universe_->getScope(scopeId_)),
      partitionId_(partitionId),
      groupId_(groupId),
      containersPtr_(containersPtr),
      scopeItemWeights_(std::move(scopeItemWeights)),
      scopeItemDefaultWeight_(scopeItemDefaultWeight),
      transformType_(transformType),
      transformFunction_(getTransformFunction(
          transformType,
          transformFunctionData,
          *universe_)),
      dimension_(universe_->getObjects().getDimension(dimensionId).only()),
      allowedScopeItems_(allowedScopeItems),
      transformFunctionData_(std::move(transformFunctionData)) {
  if (dimension_.hasNegativeValues()) {
    throw std::runtime_error(
        "GroupScopeItemTransformUtil does not support dimensions with negative values");
  }

  relevantObjectsPtr_ = std::make_shared<entities::Set<entities::ObjectId>>();
  const auto& group =
      universe_->getPartition(partitionId).getObjectIds(groupId);
  for (auto objectId : group) {
    // representative value for this object
    // - for static dimension, it is the value of the object
    // - for dynamic dimension, it is the maximum over all possible values of
    // the object
    double representativeVal = 0;
    if (dimension_.isDynamic()) {
      const auto& dimensionScope = universe_->getScope(dimension_.getScopeId());
      for (auto scopeItemId : allowedScopeItems) {
        auto& containerIds = scope_.getContainerIds(scopeItemId);
        for (auto containerId : containerIds) {
          auto dimensionScopeItemId =
              dimensionScope.getScopeItemId(containerId);
          auto normalizedVal = getNormalizedValue(
              dimension_.getValue(objectId, dimensionScopeItemId),
              normalizationConstant);
          if (normalizedVal > 0) {
            objectValuesDynamic_[objectId][containerId] = normalizedVal;
            if (dimensionScopeItemId) {
              objectValuesForScopeItem_[*dimensionScopeItemId][objectId] =
                  normalizedVal;
            }
          }
          representativeVal = std::max(representativeVal, normalizedVal);
        }
      }
    } else {
      representativeVal = getNormalizedValue(
          dimension_.getValue(objectId), normalizationConstant);
    }
    if (representativeVal > 0) {
      relevantObjectsPtr_->insert(objectId);
      objectValues_.emplace(objectId, representativeVal);
    }
  }

  maxPossibleValue_ = getMaxPossibleExpressionValue(allowedScopeItems_);

  description = fmt::format(
      "on {} of partition '{}' over scope: '{}' with transform function '{}'{}",
      universe_->getEntityName(groupId),
      universe_->getEntityName(partitionId),
      universe_->getEntityName(scopeId),
      getTransformFunctionDesc(transformType),
      normalizationConstant != 1
          ? fmt::format(" normalizer {}", normalizationConstant)
          : "");
  set_directly_affected_containers();
  // verify that transformFunctionData_ is set correctly
  validateTransformFunctionData();

  setInitialValue(applyAssignment(initialAssignment));
}

const std::string_view& GroupScopeItemTransformUtil::getType() const {
  return kType;
}

ExpressionProperties GroupScopeItemTransformUtil::getProperties() const {
  ExpressionProperties properties;
  auto& propertiesMap = *properties.properties();
  propertiesMap.emplace(
      "partition",
      PropertiesHelper::makeStringValue(
          universe_->getEntityName(partitionId_)));
  propertiesMap.emplace(
      "group",
      PropertiesHelper::makeStringValue(universe_->getEntityName(groupId_)));
  propertiesMap.emplace(
      "scope",
      PropertiesHelper::makeStringValue(universe_->getEntityName(scopeId_)));
  propertiesMap.emplace(
      "allowed scope items",
      PropertiesHelper::makeScopeItemNames(
          allowedScopeItems_, scopeId_, *universe_));
  propertiesMap.emplace(
      "transform_type",
      PropertiesHelper::makeStringValue(
          getTransformFunctionDesc(transformType_)));

  return properties;
}

std::optional<AffectedByChange> GroupScopeItemTransformUtil::isAffectedByChange(
    const AffectedByChangeDecisionData& /* unused */) const {
  return AffectedByChange(relevantObjectsPtr_);
}

void GroupScopeItemTransformUtil::computeDeltaPerContainer(
    const ChangeSet& changes,
    folly::F14FastMap<entities::ContainerId, double>& deltaPerContainer) const {
  deltaPerContainer.clear();
  for (const auto& change : changes) {
    const auto objectId = change.getObject();
    const auto containerId = change.getContainer();
    if (containersPtr_->contains(containerId) &&
        relevantObjectsPtr_->contains(objectId)) {
      deltaPerContainer[containerId] +=
          change.getValue() * getObjectValue(objectId, containerId);
    }
  }
}

void GroupScopeItemTransformUtil::computeDeltaPerScopeItem(
    const ChangeSet& changes,
    folly::F14FastMap<entities::ScopeItemId, double>& deltaPerScopeItem) const {
  deltaPerScopeItem.clear();
  for (const auto& change : changes) {
    const auto objectId = change.getObject();
    const auto containerId = change.getContainer();
    if (containersPtr_->contains(containerId) &&
        relevantObjectsPtr_->contains(objectId)) {
      const auto scopeItemId = checkAndGetScopeItemId(scope_, containerId);
      deltaPerScopeItem[scopeItemId] +=
          change.getValue() * getObjectValue(objectId, containerId);
    }
  }
}

double GroupScopeItemTransformUtil::getWeight(
    entities::ScopeItemId scopeItemId) const {
  return folly::get_default(
      scopeItemWeights_, scopeItemId, scopeItemDefaultWeight_);
}

double GroupScopeItemTransformUtil::evaluate(
    const BottomToTopEvaluator& /*evaluator*/,
    const ChangeSet& changes) const {
  // thread_local retains backing memory across calls, avoiding per-call
  // heap allocations. Each thread gets its own instance.
  thread_local folly::F14FastMap<entities::ScopeItemId, double>
      deltaPerScopeItem;
  computeDeltaPerScopeItem(changes, deltaPerScopeItem);

  double totalDelta = 0;
  const auto& precision = getPrecision();
  for (const auto& [scopeItemId, delta] : deltaPerScopeItem) {
    const auto containerUtilPtr =
        folly::get_ptr(scopeItemsToContainerUtil_, scopeItemId);
    const auto currentContribution =
        containerUtilPtr ? containerUtilPtr->query() : 0;
    const auto newContribution = currentContribution + delta;
    const auto scopeItemWeight = getWeight(scopeItemId);
    if (precision.isStrictlyGtZero(currentContribution)) {
      totalDelta -= scopeItemWeight *
          transformFunction_(currentContribution, scopeItemId);
    }
    if (precision.isStrictlyGtZero(newContribution)) {
      totalDelta +=
          scopeItemWeight * transformFunction_(newContribution, scopeItemId);
    }
  }
  return value + totalDelta;
}

void GroupScopeItemTransformUtil::updateEquivalenceSets(
    EquivalenceSets& equivalenceSets) const {
  if (!dimension_.isDynamic()) {
    // all objects with non-zero dimension value (relevant objects)
    // are identical
    equivalenceSets.mappingMerge(objectValues_);
    return;
  }

  // dynamic dimension
  for (const auto& [scopeItemId, value] : objectValuesForScopeItem_) {
    equivalenceSets.mappingMerge(value);
  }

  return;
}

double GroupScopeItemTransformUtil::applyAssignment(
    const Assignment& assignment) {
  scopeItemsToContainerUtil_.clear();
  scopeItemUtil_.clear();
  for (auto objectId : *relevantObjectsPtr_) {
    auto containerId = assignment.getContainer(objectId);
    if (containersPtr_->contains(containerId)) {
      auto scopeItemId = checkAndGetScopeItemId(scope_, containerId);
      auto& containerUtil = scopeItemsToContainerUtil_[scopeItemId];
      auto curVal = containerUtil.getValueIfExists(containerId).value_or(0);
      containerUtil.update(
          containerId, curVal + getObjectValue(objectId, containerId));
    }
  }
  for (auto& [scopeItemId, containerUtil] : scopeItemsToContainerUtil_) {
    scopeItemUtil_.update(
        scopeItemId,
        getWeight(scopeItemId) *
            transformFunction_(containerUtil.query(), scopeItemId));
  }
  value = scopeItemUtil_.query();
  return value;
}

double GroupScopeItemTransformUtil::innerFullApply(
    const TopToBottomEvaluator& /*evaluator*/,
    const Assignment& assignment) {
  return applyAssignment(assignment);
}

double GroupScopeItemTransformUtil::innerPartialApply(
    const BottomToTopEvaluator& /*evaluator*/,
    const Assignment& /*assignment*/,
    const ChangeSet& changes) {
  // thread_local retains backing memory across calls, avoiding per-call
  // heap allocations. Each thread gets its own instance.
  thread_local folly::F14FastMap<entities::ContainerId, double>
      deltaPerContainer;
  thread_local folly::F14FastSet<entities::ScopeItemId> changedScopeItems;
  computeDeltaPerContainer(changes, deltaPerContainer);
  changedScopeItems.clear();

  const auto& precision = getPrecision();
  for (const auto& [containerId, delta] : deltaPerContainer) {
    const auto scopeItemId = checkAndGetScopeItemId(scope_, containerId);
    auto containerValPtr =
        folly::get_ptr(scopeItemsToContainerUtil_, scopeItemId);
    if (!containerValPtr) {
      scopeItemsToContainerUtil_[scopeItemId].update(containerId, delta);
      changedScopeItems.insert(scopeItemId);
    } else {
      const auto maybeCurVal = containerValPtr->getValueIfExists(containerId);
      const auto newVal = maybeCurVal.value_or(0) + delta;
      if (precision.isStrictlyGtZero(newVal)) {
        containerValPtr->update(containerId, newVal);
        changedScopeItems.insert(scopeItemId);
      } else if (maybeCurVal) {
        // this container will now have a zero util
        containerValPtr->remove(containerId);
        changedScopeItems.insert(scopeItemId);
      }
    }
  }

  for (const auto scopeItemId : changedScopeItems) {
    const auto newUtil = getWeight(scopeItemId) *
        transformFunction_(scopeItemsToContainerUtil_[scopeItemId].query(),
                           scopeItemId);
    const auto maybeCurVal = scopeItemUtil_.getValueIfExists(scopeItemId);
    if (precision.isStrictlyGtZero(newUtil)) {
      scopeItemUtil_.update(scopeItemId, newUtil);
    } else if (maybeCurVal) {
      // this scopeItem will now have a zero util
      scopeItemUtil_.remove(scopeItemId);
    }
  }
  value = scopeItemUtil_.query();
  return value;
}

Bounds GroupScopeItemTransformUtil::innerLowerAndUpperBounds(
    Context& /* context */,
    const BoundConstraints& bc) const {
  double lb = 0;
  double ub = maxPossibleValue_;
  if (bc.isEmpty()) {
    return Bounds({.lower_bound = lb, .upper_bound = ub});
  } else {
    // we can compute better bounds
    // Collect containers that are not giving and taking objects
    folly::F14FastSet<entities::ContainerId> notGivingContainers;
    folly::F14FastSet<entities::ContainerId> notTakingContainers;
    for (auto containerId : *containersPtr_) {
      if (!bc.giving(containerId)) {
        notGivingContainers.insert(containerId);
      }
      if (!bc.taking(containerId)) {
        notTakingContainers.insert(containerId);
      }
    }

    // Case 1: See if we can improve the lower bound
    // Idea: if there are any containers that cannot give up their objects but
    // contribute to the util, then we can improve the lower bound
    folly::F14FastMap<entities::ScopeItemId, double> fixedUtilValuePerScopeItem;
    for (auto containerId : notGivingContainers) {
      auto scopeItemId = checkAndGetScopeItemId(scope_, containerId);
      if (auto containerValuePtr =
              folly::get_ptr(scopeItemsToContainerUtil_, scopeItemId)) {
        fixedUtilValuePerScopeItem[scopeItemId] +=
            containerValuePtr->getValueIfExists(containerId).value_or(0);
      }
    }
    for (auto& [scopeItemId, fixedUtilValue] : fixedUtilValuePerScopeItem) {
      lb += getWeight(scopeItemId) *
          transformFunction_(fixedUtilValue, scopeItemId);
    }

    // Case 2: See if we can improve the upper bound
    folly::F14FastSet<entities::ScopeItemId> scopeItemsMayContributeToUtil;
    for (auto containerId : *containersPtr_) {
      auto scopeItemId = checkAndGetScopeItemId(scope_, containerId);
      if (bc.taking(containerId) ||
          scopeItemsToContainerUtil_.contains(scopeItemId)) {
        scopeItemsMayContributeToUtil.insert(scopeItemId);
      }
    }
    ub = getMaxPossibleExpressionValue(scopeItemsMayContributeToUtil);
    return Bounds({.lower_bound = lb, .upper_bound = ub});
  }
}

std::string GroupScopeItemTransformUtil::innerDigest(
    size_t /*maxChildren*/) const {
  return description;
}

template <typename ScopeItemCollection>
double GroupScopeItemTransformUtil::getMaxPossibleExpressionValue(
    const ScopeItemCollection& allowedScopeItems) const {
  if (allowedScopeItems.empty()) {
    return 0;
  }

  const auto getSortedScopeItems = [&]() {
    std::vector<entities::ScopeItemId> sortedScopeItems(
        allowedScopeItems.begin(), allowedScopeItems.end());
    std::sort(
        sortedScopeItems.begin(),
        sortedScopeItems.end(),
        [&](const entities::ScopeItemId a, const entities::ScopeItemId b) {
          return getWeight(a) > getWeight(b);
        });
    return sortedScopeItems;
  };

  const auto getHeaviestScopeItem = [&]() {
    auto heaviestScopeItemIt = std::max_element(
        allowedScopeItems.begin(),
        allowedScopeItems.end(),
        [&](const entities::ScopeItemId a, const entities::ScopeItemId b) {
          return getWeight(a) < getWeight(b);
        });
    return *heaviestScopeItemIt;
  };

  const bool useDefaultWeights = scopeItemWeights_.empty();
  switch (transformType_) {
    case TransformFunctionType::STEP:
    case TransformFunctionType::STEP_MOD_K: {
      // when step function is used, the max possible value of this expression
      // is when all occupiable scopeItems get some object
      const auto maxNumOccupiedScopeItems = std::min<size_t>(
          relevantObjectsPtr_->size(), allowedScopeItems.size());
      if (useDefaultWeights) {
        return maxNumOccupiedScopeItems * scopeItemDefaultWeight_;
      } else {
        const auto sortedScopeItems = getSortedScopeItems();
        double totalScopeItemWeights = 0;
        for (const auto i : folly::irange(maxNumOccupiedScopeItems)) {
          totalScopeItemWeights += getWeight(sortedScopeItems.at(i));
        }
        return totalScopeItemWeights;
      }
    }

    case TransformFunctionType::IDENTITY:
    case TransformFunctionType::SQUARE: {
      double totalValue = 0;
      for (const auto& [_objectId, value] : objectValues_) {
        totalValue += value;
      }

      if (useDefaultWeights) {
        // Any scope item can be used as the heaviest since they're all equal
        auto heaviestScopeItem = *allowedScopeItems.begin();
        return transformFunction_(totalValue, heaviestScopeItem) *
            scopeItemDefaultWeight_;
      } else {
        const auto heaviestScopeItem = getHeaviestScopeItem();
        return transformFunction_(totalValue, heaviestScopeItem) *
            getWeight(heaviestScopeItem);
      }
    }
    default:
      throw std::runtime_error("Unhandled TransformFunctionType");
  }
}

void GroupScopeItemTransformUtil::set_directly_affected_containers() {
  directlyAffectedContainers.set(containersPtr_);
}

double GroupScopeItemTransformUtil::getObjectValue(
    entities::ObjectId objectId,
    entities::ContainerId containerId) const {
  if (dimension_.isDynamic()) {
    try {
      return objectValuesDynamic_.at(objectId).at(containerId);
    } catch (const std::out_of_range&) {
      XLOG(DBG1) << fmt::format(
          "objectValuesDynamic_ for object {} and containerId {} not found",
          objectId,
          containerId);
      return 0;
    }
  }

  return objectValues_.at(objectId);
}

double GroupScopeItemTransformUtil::getNormalizedValue(
    double val,
    double normalizationConst) {
  if (transformType_ == TransformFunctionType::STEP_MOD_K) {
    if (normalizationConst != 1) {
      throw std::runtime_error(
          "GroupScopeItemTransformUtil does not support normalization for STEP_MOD_K transform");
    }
    return val;
  }

  if (val > 0 && transformType_ == TransformFunctionType::STEP) {
    // when using STEP transformation, the individual values of the objects
    // are irrelevant. As long as it has a non-zero value, the object will
    // contribute towards the utilization
    return 1;
  }
  return val / normalizationConst;
}

ExprPtr GroupScopeItemTransformUtil::getExprForLp(
    const Problem& problem) const {
  struct RepresentativeObjectAndCount {
    entities::ObjectId repObjectId;
    int count;
  };

  PackerMap<entities::EquivalenceSetId, RepresentativeObjectAndCount>
      equivSetIdToRepresentativeObjectAndCount;
  for (auto objectId : *relevantObjectsPtr_) {
    auto equivSetId = problem.getEquivalenceSets().at(objectId);
    // insert once and update count every time
    auto [it, _] = equivSetIdToRepresentativeObjectAndCount.try_emplace(
        equivSetId, objectId, 0);
    it->second.count++;
  }

  const Assignment& lpAssignment = problem.initial_assignment;
  auto expr = const_expr(0, getUniversePtr());
  for (auto scopeItemId : allowedScopeItems_) {
    auto& containerIds = scope_.getContainerIds(scopeItemId);
    auto scopeItemWeight = getWeight(scopeItemId);
    if (scopeItemWeight == 0 || containerIds.empty()) {
      continue;
    }

    auto scopeItemUtil = const_expr(0, getUniversePtr());
    for (auto containerId : containerIds) {
      for (auto& [equivSetId, repObjectIdAndCount] :
           equivSetIdToRepresentativeObjectAndCount) {
        auto [repObjectId, count] = repObjectIdAndCount;
        auto objectValue = getObjectValue(repObjectId, containerId);
        if (objectValue == 0) {
          continue;
        }

        scopeItemUtil += (count * objectValue) *
            variable(repObjectId, containerId, getUniversePtr(), lpAssignment);
      }
    }

    switch (transformType_) {
      case TransformFunctionType::IDENTITY: {
        // Formula: sum_{s in S} w(g, s) * UTIL(g, s)
        expr += scopeItemWeight * scopeItemUtil;
        break;
      }

      case TransformFunctionType::STEP: {
        // Formula: sum_{s in S} w(g, s) * STEP(UTIL(g, s))
        expr += scopeItemWeight * step(scopeItemUtil, getUniversePtr());
        break;
      }

      case TransformFunctionType::SQUARE: {
        expr += scopeItemWeight * square(scopeItemUtil, getUniversePtr());
        break;
      }

      case TransformFunctionType::STEP_MOD_K: {
        assert(transformFunctionData_.kForModKTransform);
        const auto k =
            transformFunctionData_.kForModKTransform->get(scopeItemId);
        expr +=
            scopeItemWeight * step_mod_k(scopeItemUtil, k, getUniversePtr());
      }
    }
  }

  return expr;
}

algopt::lp::Expression GroupScopeItemTransformUtil::lp(
    const LpEvaluator& evaluator,
    bool minimizing,
    const interface::OptimalSolverSpec& configs) {
  auto& problem = evaluator.getProblem();
  return exprForLp_.withWLockPointer([&](auto& wLockedExprForLpPtr) {
    if (!wLockedExprForLpPtr) {
      wLockedExprForLpPtr = getExprForLp(problem);
    }

    return evaluator.lp(wLockedExprForLpPtr.get(), minimizing, configs);
  });
}

bool GroupScopeItemTransformUtil::hasNoLpIntent() const {
  return true;
}

void GroupScopeItemTransformUtil::validateTransformFunctionData() const {
  if (transformType_ == TransformFunctionType::STEP_MOD_K) {
    if (!transformFunctionData_.kForModKTransform) {
      throw std::runtime_error(
          "GroupScopeItemTransformUtil: kForModKTransform is not set for STEP_MOD_K transform");
    }
  }
}

} // namespace facebook::rebalancer
