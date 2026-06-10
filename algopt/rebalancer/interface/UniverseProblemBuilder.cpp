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

#include "algopt/rebalancer/interface/UniverseProblemBuilder.h"

#include "algopt/rebalancer/entities/RoutingConfig.h"
#include "algopt/rebalancer/interface/Constants.h"

#include <folly/container/irange.h>

#include <string>
#include <unordered_map>

using namespace facebook::rebalancer::entities;

namespace facebook::rebalancer::interface {

AsyncConfig::AsyncConfig(std::shared_ptr<folly::ThreadPoolExecutor> executor)
    : executor_(std::move(executor)),
      asyncScope_(folly::coro::AsyncScope(/*throwOnJoin=*/true)) {
  if (!executor_) [[unlikely]] {
    throw std::runtime_error("Expected executor to be non-null");
  }
}

AsyncConfig::~AsyncConfig() {
  if (!joined_) [[unlikely]] {
    waitForAllPending();
  }
}

void AsyncConfig::add(folly::coro::Task<void>&& task) {
  asyncScope_.add(co_withExecutor(executor_.get(), std::move(task)));
}

void AsyncConfig::waitForAllPending() {
  if (joined_) {
    throw std::runtime_error(
        "Unexpected call to waitForAllPending() when joined_ = true");
  }
  // alway set joined_=true at the end of scope
  const auto guard = folly::makeGuard([this]() { joined_ = true; });

  folly::coro::blockingWait(asyncScope_.joinAsync());
}

void UniverseProblemBuilder::setObjectName(const std::string& objectName) {
  universe_.setObjectTypeName(objectName);
}

void UniverseProblemBuilder::setContainerName(
    const std::string& containerName) {
  universe_.setContainerTypeName(containerName);
}

void UniverseProblemBuilder::addObjectPartitionRoutingDimension(
    const std::string& dimensionName,
    const std::string& partitionName,
    const std::string& routingConfigName,
    const std::unordered_map<std::string, double>& groupToValue,
    double defaultValue,
    const std::unordered_map<std::string, double>& groupToStaticValue,
    double defaultStaticValue) {
  const auto dimensionId = universe_.makeObjectDimensionId(dimensionName);
  const auto partitionId = universe_.getPartitionId(partitionName);
  const auto routingConfigId = universe_.getRoutingConfigId(routingConfigName);

  const auto [partition, routingConfigData] = folly::coro::blockingWait(
      folly::coro::collectAll(
          universe_.getPartition(partitionId),
          universe_.getRoutingConfig(routingConfigId)));

  const auto routingConfigPartitionId =
      routingConfigData->routingConfig->getPartitionId();
  if (routingConfigPartitionId != partitionId) {
    throw std::runtime_error(
        "Routing config and objectPartitionRoutingDimension are defined on different partitions");
  }

  entities::Map<entities::GroupId, double> groupIdToValue;
  groupIdToValue.reserve(
      static_cast<decltype(groupIdToValue)::size_type>(groupToValue.size()));
  for (auto& [groupName, value] : groupToValue) {
    if (value == defaultValue) {
      continue;
    }
    groupIdToValue.emplace(partition->getGroupId(groupName), value);
  }

  entities::Map<entities::GroupId, double> groupIdToStaticValue;
  groupIdToStaticValue.reserve(
      static_cast<decltype(groupIdToStaticValue)::size_type>(
          groupToStaticValue.size()));
  for (auto& [groupName, staticValue] : groupToStaticValue) {
    if (staticValue == defaultStaticValue) {
      continue;
    }
    groupIdToStaticValue.emplace(partition->getGroupId(groupName), staticValue);
  }

  std::shared_ptr<const ObjectDimension> objectDimension =
      std::make_shared<const ObjectDimension>(
          std::move(groupIdToValue),
          defaultValue,
          partitionId,
          routingConfigId,
          std::move(groupIdToStaticValue),
          defaultStaticValue);

  folly::coro::blockingWait(universe_.addObjectDimension(
      dimensionId,
      ObjectDimensionData{.dimension = std::move(objectDimension)}));
}

void UniverseProblemBuilder::addSimilarContainers(
    const std::vector<std::vector<std::string>>& similarContainerClasses) {
  const auto containers = universe_.getContainers();
  std::vector<std::vector<ContainerId>> similarContainers;
  similarContainers.reserve(similarContainerClasses.size());
  for (const auto& containerNames : similarContainerClasses) {
    std::vector<ContainerId> containerIds;
    containerIds.reserve(containerNames.size());
    for (const auto& containerName : containerNames) {
      containerIds.push_back(containers->getId(containerName));
    }
    similarContainers.push_back(std::move(containerIds));
  }

  universe_.setSimilarContainers(std::move(similarContainers));
}

void UniverseProblemBuilder::addRoutingConfig(
    const std::string& configName,
    const std::string& scopeName,
    const std::string& partitionName,
    const std::unordered_map<std::string, interface::GroupRoutingRings>&
        groupToRoutingRings,
    const std::unordered_map<
        std::string,
        std::unordered_map<std::string, double>>& originToDestinationLatency,
    const std::optional<
        std::unordered_map<std::string, std::vector<std::vector<std::string>>>>&
        defaultOriginToDestinationScopeItemSets) {
  const auto routingConfigId = universe_.makeRoutingConfigId(configName);
  const auto scopeId = universe_.getScopeId(scopeName);
  const auto partitionId = universe_.getPartitionId(partitionName);

  const auto [scope, partition] = folly::coro::blockingWait(
      folly::coro::collectAll(
          universe_.getScope(scopeId), universe_.getPartition(partitionId)));

  auto toDestinationScopeItemIdsSets = [&](auto& destinationScopeItemSets) {
    std::vector<std::vector<entities::ScopeItemId>> destinationScopeItemIdsSets;
    destinationScopeItemIdsSets.reserve(destinationScopeItemSets.size());
    for (const auto& destinationScopeItem : destinationScopeItemSets) {
      std::vector<entities::ScopeItemId> destinationScopeItemIds;
      destinationScopeItemIds.reserve(destinationScopeItem.size());
      for (const auto& scopeItemIds : destinationScopeItem) {
        destinationScopeItemIds.push_back(scope->getScopeItemId(scopeItemIds));
      }
      destinationScopeItemIdsSets.push_back(std::move(destinationScopeItemIds));
    }
    return destinationScopeItemIdsSets;
  };

  // create groupIdToRoutingRings
  auto groupIdToRoutingRings =
      entities::Map<entities::GroupId, std::vector<entities::RoutingRing>>();
  for (const auto& [groupName, routingRings] : groupToRoutingRings) {
    const auto groupId = partition->getGroupId(groupName);
    std::vector<entities::RoutingRing> routingRingEntities;
    routingRingEntities.reserve(routingRings.routingRings()->size());
    for (const auto& routingRing : *routingRings.routingRings()) {
      const auto originScopeItem =
          scope->getScopeItemId(*routingRing.originScopeItem());
      const auto originTraffic = *routingRing.originTraffic();
      auto destinationScopeItemIdSets =
          routingRing.destinationScopeItemSets().has_value()
          ? std::make_optional(toDestinationScopeItemIdsSets(
                *routingRing.destinationScopeItemSets()))
          : std::nullopt;
      routingRingEntities.emplace_back(
          originScopeItem,
          originTraffic,
          std::move(destinationScopeItemIdSets));
    }
    groupIdToRoutingRings.emplace(groupId, std::move(routingRingEntities));
  }

  auto latencyTable = std::make_shared<entities::Map<
      entities::ScopeItemId,
      algopt::ValueSortedMap<
          entities::ScopeItemId,
          double,
          CompareScopeItemLatencyPair>>>();
  for (auto& [origin, destinationMap] : originToDestinationLatency) {
    auto originScopeItemId = scope->getScopeItemId(origin);

    algopt::ValueSortedMap<
        entities::ScopeItemId,
        double,
        CompareScopeItemLatencyPair>
        destinationToLatencyMap;
    for (auto& [destination, latency] : destinationMap) {
      const auto destinationScopeItemId = scope->getScopeItemId(destination);
      destinationToLatencyMap.assign(destinationScopeItemId, latency);
    }
    latencyTable->emplace(
        originScopeItemId, std::move(destinationToLatencyMap));
  }

  std::shared_ptr<entities::Map<
      entities::ScopeItemId,
      std::vector<std::vector<entities::ScopeItemId>>>>
      defaultOriginToDestinationScopeItemSetsPtr = nullptr;

  if (defaultOriginToDestinationScopeItemSets.has_value()) {
    defaultOriginToDestinationScopeItemSetsPtr = std::make_shared<entities::Map<
        entities::ScopeItemId,
        std::vector<std::vector<entities::ScopeItemId>>>>();
    for (auto& [origin, destinationScopeItemSets] :
         defaultOriginToDestinationScopeItemSets.value()) {
      defaultOriginToDestinationScopeItemSetsPtr->emplace(
          scope->getScopeItemId(origin),
          toDestinationScopeItemIdsSets(destinationScopeItemSets));
    }
  }

  folly::coro::blockingWait(universe_.addRoutingConfig(
      routingConfigId,
      RoutingConfigData{
          .routingConfig = std::make_shared<RoutingConfig>(
              std::move(groupIdToRoutingRings),
              std::move(latencyTable),
              scopeId,
              partitionId,
              std::move(defaultOriginToDestinationScopeItemSetsPtr))}));
}

/*static*/
interface::CapacitySpec UniverseProblemBuilder::fromThrottlingToCapacitySpec(
    const ThrottlingSpec& spec) {
  if (*spec.limit()->type() != LimitType::RELATIVE) {
    throw std::runtime_error("only relative limit is valid in throttling spec");
  }

  CapacitySpec capacity;
  capacity.name() = *spec.name();
  capacity.scope() = *spec.scope();
  capacity.dimension() = *spec.dimension();
  capacity.limit() = *spec.limit();
  capacity.filter() = *spec.filter();

  switch (*spec.definition()) {
    case ThrottlingSpecDefinition::ANY:
      capacity.definition() = CapacitySpecDefinition::MOVED_DATA;
      break;
    case ThrottlingSpecDefinition::IN:
      capacity.definition() = CapacitySpecDefinition::NEW;
      break;
    case ThrottlingSpecDefinition::OUT:
      capacity.definition() = CapacitySpecDefinition::OLD;
      break;
  }

  return capacity;
}

void UniverseProblemBuilder::setEmptyOrUnsetDimensionField(GenericSpec& spec) {
  switch (spec.getType()) {
    case interface::GenericSpec::Type::logicalOrSpec:
      setEmptyOrUnsetDimensionField(spec.mutable_logicalOrSpec());
      break;
    case interface::GenericSpec::Type::logicalAndSpec:
      setEmptyOrUnsetDimensionField(spec.mutable_logicalAndSpec());
      break;
    case interface::GenericSpec::Type::capacitySpec:
      algopt::utils::setStringFieldToValueIfEmptyOrUnset(
          spec.mutable_capacitySpec(),
          kDimensionFieldName.data(),
          getObjectCountDimensionName());
      break;
    case interface::GenericSpec::Type::groupCountSpec:
      algopt::utils::setStringFieldToValueIfEmptyOrUnset(
          spec.mutable_groupCountSpec(),
          kDimensionFieldName.data(),
          getObjectCountDimensionName());
      break;
    case interface::GenericSpec::Type::groupCapacitySpec:
      algopt::utils::setStringFieldToValueIfEmptyOrUnset(
          spec.mutable_groupCapacitySpec(),
          kDimensionFieldName.data(),
          getObjectCountDimensionName());
      break;
    case interface::GenericSpec::Type::__EMPTY__:
      throw std::runtime_error("Uninitialized generic spec");
  }
}

void UniverseProblemBuilder::setEmptyOrUnsetDimensionField(
    MultipleOrCapacitySpec& spec) {
  for (auto& capacitySpec : *spec.capacitySpecs()) {
    // if dimension field is empty, then use objectCountDimension
    algopt::utils::setStringFieldToValueIfEmptyOrUnset(
        capacitySpec,
        kDimensionFieldName.data(),
        getObjectCountDimensionName());
  }
}

void UniverseProblemBuilder::enableRestrictMovingObjectOnlyOnce() {
  universe_.setMoveObjectsOnce(true);
}

void UniverseProblemBuilder::enableStableAsMuchAsPossible() {
  universe_.setStableOptimization(true);
}

void UniverseProblemBuilder::setFeasibilityTolerance(
    double /* feasibilityTolerance */) {}

void UniverseProblemBuilder::setConstraintPolicy(ConstraintPolicy policy) {
  if (constraintCount_ > 0) {
    throw std::runtime_error(
        "Calling setConstraintPolicy after a addConstraint call leads to undesirable behavior as the policy in setConstraintPolicy only affects future addConstraint calls.");
  }
  globalConstraintPolicy_ = policy;
}

void UniverseProblemBuilder::setDefaultConstraintParams(
    const ConstraintParams& constraintParams) {
  if (constraintCount_ > 0) {
    throw std::runtime_error(
        "Calling setDefaultConstraintParams after a addConstraint call leads to undesirable behavior as the parameters in setDefaultConstraintParams only affect future addConstraint calls.");
  }
  constraintParams_ = constraintParams;
}

void UniverseProblemBuilder::printSummary() const {
  universe_.printSummary();
}

std::string UniverseProblemBuilder::getSummary() const {
  return universe_.getSummary();
}

std::shared_ptr<const entities::Universe> UniverseProblemBuilder::build() {
  if (asyncConfig_) {
    asyncConfig_->waitForAllPending();
    // AsyncConfig (and its underlying folly::coro::AsyncScope) is one-shot:
    // once joined it cannot accept new tasks. Replace it so the builder can
    // be used again for subsequent build() calls.
    asyncConfig_ = std::make_unique<AsyncConfig>(asyncConfig_->executor());
  }
  return folly::coro::blockingWait(universe_.build());
}

ObjectIdToDoubleMap UniverseProblemBuilder::getScaledObjectDimension(
    const ObjectIdToDoubleMap& objectValues,
    double defaultValue,
    const Map<ContainerId, double>& containerUsage) const {
  const auto containers = universe_.getContainers();
  const auto& containerIdToInitialObjectIds =
      containers->containerIdToObjectIds;

  Map<ObjectId, double> scaledObjectValues;
  for (const auto& [containerId, objectIdsInContainer] :
       containerIdToInitialObjectIds) {
    const auto objectCount = objectIdsInContainer.size();

    if (objectCount == 0) {
      continue;
    }

    double total = 0.0;
    std::vector<ObjectId> objectsInContainer;
    for (auto objectId : objectIdsInContainer) {
      objectsInContainer.push_back(objectId);

      total += objectValues.getValue(objectId);
    }

    const double offset =
        total == 0 ? 1.0 / objectCount : total / 100.0 / objectCount;

    total += offset * objectCount;
    auto usagePtr = folly::get_ptr(containerUsage, containerId);
    if (!usagePtr) {
      throw std::runtime_error(
          fmt::format(
              "container '{}' is non-empty, but it does not have an associated dimension value",
              containerId));
    }
    const double coeff = *usagePtr / total;

    for (auto objectId : objectsInContainer) {
      auto dimValue = objectValues.getValue(objectId);
      double scaledValue = (dimValue + offset) * coeff;

      if (scaledObjectValues.contains(objectId)) {
        scaledObjectValues.at(objectId) =
            std::min(scaledObjectValues.at(objectId), scaledValue);
      } else {
        scaledObjectValues.emplace(objectId, scaledValue);
      }
    }
  }

  const auto totalObjects = universe_.getObjects()->numObjects;
  return ObjectIdToDoubleMap(scaledObjectValues, defaultValue, totalObjects);
}

ColocateGroupsSpec UniverseProblemBuilder::toColocateGroupsSpec(
    PairAffinitiesSpec spec) {
  constexpr static std::string_view kPairAffinityStr{"pair_affinity"};
  folly::F14FastMap<std::string, std::vector<std::string>> groupToObjects;
  folly::F14FastMap<std::string, double> groupToWeight;
  for (const auto i : folly::irange(spec.affinities()->size())) {
    auto groupName = fmt::format("{}_{}", kPairAffinityStr, i);
    auto& pairAffinty = spec.affinities()->at(i);
    auto& objectInGroup = groupToObjects[groupName];
    objectInGroup.push_back(std::move(*pairAffinty.pair()->object1()));
    objectInGroup.push_back(std::move(*pairAffinty.pair()->object2()));
    groupToWeight.emplace(std::move(groupName), *pairAffinty.affinity());
  }

  constexpr static std::string_view kPairAffinitiesSpecStr{
      "pair_affinities_spec"};
  static size_t count = 0;
  auto partitionName = fmt::format(
      "{}_{}_{}", kInternalPartitionPrefix, kPairAffinitiesSpecStr, ++count);
  addPartition(partitionName, std::move(groupToObjects));

  ColocateGroupsSpec colocateGroupsSpec;
  colocateGroupsSpec.name() =
      fmt::format("{}_translated_from_pair_affinity_spec", *spec.name());
  colocateGroupsSpec.scope() =
      spec.scope()->empty() ? universe_.getContainerTypeName() : *spec.scope();
  colocateGroupsSpec.partitionName() = std::move(partitionName);
  colocateGroupsSpec.bound() = ColocateGroupsSpecBound::MAX;
  auto& limits = *colocateGroupsSpec.limits();
  limits.type() = LimitType::ABSOLUTE;
  limits.globalLimit() = *spec.limit();
  colocateGroupsSpec.dimension() = getObjectCountDimensionName();
  colocateGroupsSpec.groupToWeight() = std::move(groupToWeight);

  return colocateGroupsSpec;
}

void UniverseProblemBuilder::addGoal(
    std::string name,
    GoalSpecs spec,
    double weight,
    int tuplePos) {
  int goalCount = goalCount_++;
  const std::string goalName =
      name.empty() ? fmt::format("goal_{}", goalCount) : std::move(name);
  const auto goalId = universe_.makeGoalId(goalName);
  folly::coro::blockingWait(universe_.addGoal(
      goalId,
      GoalData{
          .goal = std::make_unique<Goal>(std::move(spec), weight, tuplePos)}));
}

void UniverseProblemBuilder::addConstraint(
    std::string name,
    ConstraintSpecs spec,
    std::optional<ConstraintPolicy> policy,
    std::optional<double> invalidCost,
    std::optional<double> invalidState,
    std::optional<int> tuplePosIfBroken) {
  int constraintCount = constraintCount_++;
  auto finalName = name.empty() ? fmt::format("constraint_{}", constraintCount)
                                : std::move(name);
  auto finalPolicy = policy ? *policy : globalConstraintPolicy_;
  auto finalInvalidCost =
      invalidCost ? *invalidCost : *constraintParams_.invalidCost();
  auto finalInvalidState =
      invalidState ? *invalidState : *constraintParams_.invalidState();
  auto finalTupleIndex = tuplePosIfBroken
      ? *tuplePosIfBroken
      : *constraintParams_.tuplePosIfBroken();

  const auto constraintId = universe_.makeConstraintId(finalName);

  folly::coro::blockingWait(universe_.addConstraint(
      constraintId,
      ConstraintData{
          .constraint = std::make_unique<Constraint>(
              std::move(spec),
              finalPolicy,
              finalInvalidCost,
              finalInvalidState,
              finalTupleIndex)}));
}

std::string UniverseProblemBuilder::getObjectCountDimensionName() const {
  return fmt::format("{}_count", universe_.getObjectTypeName());
}

void UniverseProblemBuilder::overrideContainerHotnessRanking(
    const std::vector<std::string>& descendingHotnessContainers) {
  const auto& containers = universe_.getContainers();
  std::vector<ContainerId> containerIds;
  containerIds.reserve(descendingHotnessContainers.size());
  for (const auto& containerName : descendingHotnessContainers) {
    containerIds.push_back(containers->getId(containerName));
  }

  universe_.setDescendingHotnessContainers(std::move(containerIds));
}

void UniverseProblemBuilder::setObjectOrderingDimension(
    const std::string& dimensionName) {
  const auto dimensionId = universe_.getDimensionId(dimensionName);
  universe_.setObjectOrderingDimensionId(dimensionId);
}

void UniverseProblemBuilder::addDestinationsToExploreOptions(
    const std::string& name,
    DestinationsToExploreOptions destinationsToExploreOptions) {
  universe_.addDestinationsToExploreOptions(
      name, std::move(destinationsToExploreOptions));
}

void UniverseProblemBuilder::setPrecision(
    algopt::common::thrift::PrecisionTolerances& precisionTolerances) {
  universe_.setPrecision(precisionTolerances);
}

} // namespace facebook::rebalancer::interface
