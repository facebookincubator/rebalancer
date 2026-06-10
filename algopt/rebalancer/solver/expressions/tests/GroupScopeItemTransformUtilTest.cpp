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

#include "algopt/rebalancer/entities/tests/Utils.h"
#include "algopt/rebalancer/solver/expressions/GroupScopeItemTransformUtil.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"

#include <folly/container/irange.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class GroupScopeItemTransformUtilTest
    : public ExpressionTestsBase,
      public ::testing::WithParamInterface<std::tuple<
          GroupScopeItemTransformUtil::TransformFunctionType,
          bool,
          bool>> {
 protected:
  struct ExprParams {
    entities::GroupId groupId;
    entities::DimensionId dimensionId;
    std::vector<entities::ScopeItemId> allowedScopeItems;
    folly::F14FastMap<entities::ScopeItemId, double> scopeItemWeights = {};
    double scopeItemDefaultWeight = 1.0;
    double normalizationConstant = 1.0;
  };

  folly::coro::Task<std::shared_ptr<const entities::Universe>> setUpUniverse();

  std::shared_ptr<GroupScopeItemTransformUtil> makeGroupUtilExpr(
      const ExprParams& params,
      const std::shared_ptr<const entities::Universe>& universe);

  // transforms the raw scope utilization as per the transformType
  double getExpectedValue(
      folly::F14FastMap<entities::ScopeItemId, int> scopeItemUtilRaw,
      folly::F14FastMap<entities::ScopeItemId, int> scopeItemWeights = {},
      double normalizationConstant = 1);

  void checkExpectedValues(
      folly::F14FastMap<
          entities::GroupId,
          folly::F14FastMap<
              entities::ScopeItemId,
              std::vector<entities::ObjectId>>> expectedObjectsPerGroup,
      std::optional<ChangeSet> changeSet = std::nullopt);

  void checkExpectedValuesWithDynamicDimensions(
      folly::F14FastMap<
          GroupScopeItemTransformUtil::TransformFunctionType,
          folly::F14FastMap<std::string, double>> expectedValues,
      std::optional<ChangeSet> changeSet = std::nullopt);

  entities::GroupId group(int i) {
    return groupId(partitionId("partition"), fmt::format("group{}", i));
  }
  entities::ScopeItemId scopeItem(int i) {
    return scopeItemId(scopeId("scope"), fmt::format("scopeItem{}", i));
  }
  GroupScopeItemTransformUtil::TransformFunctionType
      transformFunctionTypeParam = std::get<0>(GetParam());
  bool isDynamicDimensionTest = std::get<1>(GetParam());
  bool isDimensionWithDifferentScope = std::get<2>(GetParam());

  int getNormalizationConstant() {
    // STEP_MOD_K transform does not support normalization value other than 1
    // because then normalized util mod k would yield unexpected results
    return transformFunctionTypeParam ==
            GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K
        ? 1
        : nonDefaultNormalizationConst;
  }

  std::shared_ptr<GroupScopeItemTransformUtil> group0ScopeItemUtil_ = nullptr;
  std::shared_ptr<GroupScopeItemTransformUtil> group1ScopeItemUtil_ = nullptr;
  std::shared_ptr<GroupScopeItemTransformUtil> group0ScopeItemUtilWeighted_ =
      nullptr;
  std::shared_ptr<GroupScopeItemTransformUtil> group0ScopeItemUtilNormalized_ =
      nullptr;
  // create a root expression that is the sum of all group scope item util
  // expressions
  std::shared_ptr<Expression> rootExpr_;
  entities::Map<facebook::rebalancer::entities::ObjectId, double> objectValues_;
  int defaultValue = 1;
  double nonDefaultNormalizationConst = 2;
  TransformFunctionData transformFunctionData_;
};

std::shared_ptr<GroupScopeItemTransformUtil>
GroupScopeItemTransformUtilTest::makeGroupUtilExpr(
    const ExprParams& params,
    const std::shared_ptr<const entities::Universe>& universe) {
  const auto partitionId = ExpressionTestsBase::partitionId("partition");
  const auto scopeId = ExpressionTestsBase::scopeId("scope");
  auto containersPtr = std::make_shared<entities::Set<entities::ContainerId>>();
  for (const auto& allowedScopeItem : params.allowedScopeItems) {
    const auto& scopeItemContainers =
        universe->getScope(scopeId).getContainerIds(allowedScopeItem);
    containersPtr->insert(
        scopeItemContainers.begin(), scopeItemContainers.end());
  }

  auto normalizationConstant = params.normalizationConstant;
  if (transformFunctionTypeParam ==
      GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K) {
    // STEP_MOD_K assumes that no normalization is applied because
    // normalization factor will change the value of utilization mod k
    // so we override the normalization constant to 1 for this case
    normalizationConstant = 1;
  }

  Assignment assignment(universe->getContainers().getInitialAssignment());
  return std::make_shared<GroupScopeItemTransformUtil>(
      universe,
      partitionId,
      params.groupId,
      params.dimensionId,
      scopeId,
      params.allowedScopeItems,
      containersPtr,
      assignment,
      params.scopeItemWeights,
      params.scopeItemDefaultWeight,
      transformFunctionTypeParam,
      normalizationConstant,
      transformFunctionData_);
}

folly::coro::Task<std::shared_ptr<const entities::Universe>>
GroupScopeItemTransformUtilTest::setUpUniverse() {
  // Sets up a problem with 7 objects, 6 containers, and 2 groups.
  // object7 has dimension value 0, so it does not exist for all
  // practical purposes, so we will not consider it in the test directly.
  // It is there to make sure that the implementation accounts for zero weight
  // objects

  constexpr int numContainers = 6;
  entities::Map<std::string, std::vector<std::string>> initialAssignment;
  for (const auto i : folly::irange(numContainers)) {
    // initially object_i is in container_i
    initialAssignment[fmt::format("container{}", i)] = {
        fmt::format("object{}", i)};
  }
  // Add object6 to container4
  initialAssignment["container4"].emplace_back("object6");
  setInitialAssignment(initialAssignment);

  co_await addPartition(
      "partition",
      entities::Map<std::string, std::vector<std::string>>{
          {"group0", {"object0", "object1", "object2"}},
          {"group1", {"object3", "object4", "object6"}},
          {"group2", {"object5"}}});

  co_await addScope(
      "scope",
      entities::Map<std::string, std::vector<std::string>>{
          {"scopeItem0", {"container0", "container1", "container2"}},
          {"scopeItem1", {"container3", "container4"}}});

  if (isDynamicDimensionTest) {
    const auto numObjects = getNumObjects();
    if (isDimensionWithDifferentScope) {
      // add dimension with scope different from Expr scope
      co_await addScope(
          "scope2",
          entities::Map<std::string, std::vector<std::string>>{
              {"scope2Item0", {"container0", "container1", "container2"}},
              {"scope2Item1", {"container3", "container4"}}});

      const auto scope2Id = scopeId("scope2");
      const auto scope2Item0Id = scopeItemId(scope2Id, "scope2Item0");
      const auto scope2Item1Id = scopeItemId(scope2Id, "scope2Item1");

      co_await addObjectDimension(
          "dimension",
          entities::ObjectDimensionData{
              std::make_unique<const entities::ObjectDimension>(
                  scope2Id,
                  entities::Map<
                      entities::ScopeItemId,
                      std::shared_ptr<const entities::ObjectIdToDoubleMap>>{
                      {scope2Item0Id,
                       entities::tests::makeSharedObjectIdToDoubleMap(
                           {{object(0), 2}, {object(1), 1}, {object(2), 2}},
                           /*defaultValue=*/0,
                           numObjects)},
                      {scope2Item1Id,
                       entities::tests::makeSharedObjectIdToDoubleMap(
                           {{object(3), 1}, {object(4), 2}, {object(2), 1}},
                           /*defaultValue=*/0,
                           numObjects)}},
                  0,
                  numObjects)});
    } else {
      const auto mainScopeId = scopeId("scope");
      const auto scopeItem0Id = scopeItemId(mainScopeId, "scopeItem0");
      const auto scopeItem1Id = scopeItemId(mainScopeId, "scopeItem1");

      co_await addObjectDimension(
          "dimension",
          entities::ObjectDimensionData{
              std::make_unique<const entities::ObjectDimension>(
                  mainScopeId,
                  entities::Map<
                      entities::ScopeItemId,
                      std::shared_ptr<const entities::ObjectIdToDoubleMap>>{
                      {scopeItem0Id,
                       entities::tests::makeSharedObjectIdToDoubleMap(
                           {{object(0), 2}, {object(1), 1}, {object(2), 2}},
                           /*defaultValue=*/0,
                           numObjects)},
                      {scopeItem1Id,
                       entities::tests::makeSharedObjectIdToDoubleMap(
                           {{object(3), 1}, {object(4), 2}, {object(2), 1}},
                           /*defaultValue=*/0,
                           numObjects)}},
                  0,
                  numObjects)});
    }
  } else {
    // all even objects have weight 2, all odd objects have weight 1, object6
    // has weight 0
    objectValues_ = {
        {object(0), 2}, {object(2), 2}, {object(4), 2}, {object(6), 0}};

    co_await addObjectDimension(
        "dimension",
        entities::ObjectDimensionData{
            std::make_unique<const entities::ObjectDimension>(
                entities::tests::makeObjectIdToDoubleMap(
                    objectValues_, defaultValue, getNumObjects()))});
  }

  const auto scopeId = ExpressionTestsBase::scopeId("scope");
  const auto dimensionId = ExpressionTestsBase::dimensionId("dimension");

  // Set up transform function data for STEP_MOD_K
  if (transformFunctionTypeParam ==
      GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K) {
    transformFunctionData_.kForModKTransform =
        ModKTransformData(1, {{scopeItem(0), 3}, {scopeItem(1), 2}});
  }

  auto universe = buildUniverse();

  // Get allowed scope items for expression creation
  const auto defaultAllowedScopeItems =
      universe->getScope(scopeId).getScopeItemIds();

  group0ScopeItemUtil_ = makeGroupUtilExpr(
      {.groupId = group(0),
       .dimensionId = dimensionId,
       .allowedScopeItems = defaultAllowedScopeItems},
      universe);

  group1ScopeItemUtil_ = makeGroupUtilExpr(
      {.groupId = group(1),
       .dimensionId = dimensionId,
       .allowedScopeItems = defaultAllowedScopeItems},
      universe);

  group0ScopeItemUtilWeighted_ = makeGroupUtilExpr(
      {.groupId = group(0),
       .dimensionId = dimensionId,
       .allowedScopeItems = defaultAllowedScopeItems,
       .scopeItemWeights =
           folly::F14FastMap<entities::ScopeItemId, double>{
               {scopeItem(0), 2}, {scopeItem(1), 1}}},
      universe);

  group0ScopeItemUtilNormalized_ = makeGroupUtilExpr(
      {.groupId = group(0),
       .dimensionId = dimensionId,
       .allowedScopeItems = defaultAllowedScopeItems,
       .normalizationConstant = nonDefaultNormalizationConst},
      universe);

  rootExpr_ = group0ScopeItemUtil_ + group1ScopeItemUtil_ +
      group0ScopeItemUtilWeighted_ + group0ScopeItemUtilNormalized_;

  co_return universe;
}

// transforms the raw scope utilization as per the transformType

double GroupScopeItemTransformUtilTest::getExpectedValue(
    folly::F14FastMap<entities::ScopeItemId, int> scopeItemUtilRaw,
    folly::F14FastMap<entities::ScopeItemId, int> scopeItemWeights,
    double normalizationConstant) {
  const double scopeItem0UtilRaw =
      folly::get_default(scopeItemUtilRaw, scopeItem(0), 0) /
      normalizationConstant;
  const double scopeItem1UtilRaw =
      folly::get_default(scopeItemUtilRaw, scopeItem(1), 0) /
      normalizationConstant;
  auto scopeItem0Weight = folly::get_default(scopeItemWeights, scopeItem(0), 1);
  auto scopeItem1Weight = folly::get_default(scopeItemWeights, scopeItem(1), 1);
  switch (transformFunctionTypeParam) {
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP:
      return scopeItem0Weight * (scopeItem0UtilRaw > 0) +
          scopeItem1Weight * (scopeItem1UtilRaw > 0);
    case GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY:
      return scopeItem0Weight * scopeItem0UtilRaw +
          scopeItem1Weight * scopeItem1UtilRaw;
    case GroupScopeItemTransformUtil::TransformFunctionType::SQUARE:
      return scopeItem0Weight * scopeItem0UtilRaw * scopeItem0UtilRaw +
          scopeItem1Weight * scopeItem1UtilRaw * scopeItem1UtilRaw;
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K: {
      // For testing purposes, we'll use k=3 as the default value
      assert(transformFunctionData_.kForModKTransform.has_value());
      auto k = transformFunctionData_.kForModKTransform.value();
      return scopeItem0Weight *
          (std::fmod(scopeItem0UtilRaw, k.get(scopeItem(0))) > 0) +
          scopeItem1Weight *
          (std::fmod(scopeItem1UtilRaw, k.get(scopeItem(1))) > 0);
    }
    default:
      throw std::invalid_argument("unknown transform function");
  }
}

void GroupScopeItemTransformUtilTest::checkExpectedValues(
    folly::F14FastMap<
        entities::GroupId,
        folly::
            F14FastMap<entities::ScopeItemId, std::vector<entities::ObjectId>>>
        expectedObjectsPerGroup,
    std::optional<ChangeSet> changeSet) {
  // Compute raw utils based on dimension values
  folly::F14FastMap<entities::ScopeItemId, int> group0UtilRaw;
  folly::F14FastMap<entities::ScopeItemId, int> group1UtilRaw;
  auto totalWeight = [&](auto objectIds) {
    double total = 0;
    for (const auto& objectId : objectIds) {
      total += folly::get_default(objectValues_, objectId, defaultValue);
    }
    return total;
  };
  for (const auto& [scopeItemId, objects] :
       expectedObjectsPerGroup.at(group(0))) {
    group0UtilRaw[scopeItemId] += totalWeight(objects);
  }
  for (const auto& [scopeItemId, objects] :
       expectedObjectsPerGroup.at(group(1))) {
    group1UtilRaw[scopeItemId] += totalWeight(objects);
  }
  auto group0Expected = getExpectedValue(group0UtilRaw);
  auto group1Expected = getExpectedValue(group1UtilRaw);
  auto group0ExpectedWithWeights =
      getExpectedValue(group0UtilRaw, {{scopeItem(0), 2}});
  auto group0ExpectedNormalized =
      getExpectedValue(group0UtilRaw, {}, getNormalizationConstant());

  EXPECT_EQ(
      group0Expected,
      changeSet ? evaluate(*group0ScopeItemUtil_, *changeSet)
                : group0ScopeItemUtil_->value);
  EXPECT_EQ(
      group1Expected,
      changeSet ? evaluate(*group1ScopeItemUtil_, *changeSet)
                : group1ScopeItemUtil_->value);
  // all group0 objects are in scopeItem0 that has weight 2
  EXPECT_EQ(
      group0ExpectedWithWeights,
      changeSet ? evaluate(*group0ScopeItemUtilWeighted_, *changeSet)
                : group0ScopeItemUtilWeighted_->value);

  auto eps = 1e-4;
  EXPECT_NEAR(
      group0ExpectedNormalized,
      changeSet ? evaluate(*group0ScopeItemUtilNormalized_, *changeSet)
                : group0ScopeItemUtilNormalized_->value,
      eps);
}

void GroupScopeItemTransformUtilTest::checkExpectedValuesWithDynamicDimensions(
    folly::F14FastMap<
        GroupScopeItemTransformUtil::TransformFunctionType,
        folly::F14FastMap<std::string, double>> expectedValues,
    std::optional<ChangeSet> changeSet) {
  EXPECT_EQ(
      expectedValues[transformFunctionTypeParam]["group0ScopeItemUtil_"],
      changeSet ? evaluate(*group0ScopeItemUtil_, *changeSet)
                : group0ScopeItemUtil_->value);
  EXPECT_EQ(
      expectedValues[transformFunctionTypeParam]["group1ScopeItemUtil_"],
      changeSet ? evaluate(*group1ScopeItemUtil_, *changeSet)
                : group1ScopeItemUtil_->value);
  EXPECT_EQ(
      expectedValues[transformFunctionTypeParam]
                    ["group0ScopeItemUtilWeighted_"],
      changeSet ? evaluate(*group0ScopeItemUtilWeighted_, *changeSet)
                : group0ScopeItemUtilWeighted_->value);
  EXPECT_EQ(
      expectedValues[transformFunctionTypeParam]
                    ["group0ScopeItemUtilNormalized_"],
      changeSet ? evaluate(*group0ScopeItemUtilNormalized_, *changeSet)
                : group0ScopeItemUtilNormalized_->value);
}

INSTANTIATE_TEST_CASE_P(
    ScopeItemCount,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::STEP,
            false,
            false)));
INSTANTIATE_TEST_CASE_P(
    ScopeItemCountWithDynamicDimension,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::STEP,
            true,
            false)));
INSTANTIATE_TEST_CASE_P(
    ScopeItemCountWithDynamicDimensionDifferentScope,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::STEP,
            true,
            true)));

INSTANTIATE_TEST_CASE_P(
    ScopeItemSum,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY,
            false,
            false)));
INSTANTIATE_TEST_CASE_P(
    ScopeItemSumWithDynamicDimension,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY,
            true,
            false)));
INSTANTIATE_TEST_CASE_P(
    ScopeItemSumWithDynamicDimensionDifferentScope,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY,
            true,
            true)));

INSTANTIATE_TEST_CASE_P(
    ScopeItemSquaredSum,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::SQUARE,
            false,
            false)));
INSTANTIATE_TEST_CASE_P(
    ScopeItemSquaredSumWithDynamicDimension,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::SQUARE,
            true,
            false)));
INSTANTIATE_TEST_CASE_P(
    ScopeItemSquaredSumWithDynamicDimensionDifferentScope,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::SQUARE,
            true,
            true)));

INSTANTIATE_TEST_CASE_P(
    ScopeItemDivisibleByK,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K,
            false,
            false)));

INSTANTIATE_TEST_CASE_P(
    ScopeItemDivisibleByKWithDynamicDimension,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K,
            true,
            false)));

INSTANTIATE_TEST_CASE_P(
    ScopeItemDivisibleByKWithDynamicDimensionDifferentScope,
    GroupScopeItemTransformUtilTest,
    ::testing::Values(
        std::make_tuple(
            GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K,
            true,
            true)));

CO_TEST_P(GroupScopeItemTransformUtilTest, testApply) {
  const auto universe = co_await setUpUniverse();
  if (isDynamicDimensionTest) {
    co_return;
  }

  Context context;
  const TopToBottomEvaluator evaluator(context);
  // 1. Test fullApply
  // container0: {object0}  - scopeItem0
  // container1: {object1}  - scopeItem0
  // container2: {object2}  - scopeItem0
  // container3: {object3}  * scopeItem1
  // container4 : {object4} * scopeItem1
  auto initialAssignment =
      Assignment(universe->getContainers().getInitialAssignment());
  rootExpr_->fullApply(evaluator, initialAssignment);
  // Initially, rawUtil of group 0 in scopeItem0 is 3, rawUtil of group1
  // scopeItem1 is 2 everything else is zero
  XLOG(INFO) << "1. Verifying full apply";
  checkExpectedValues(
      {{group(0), {{scopeItem(0), {object(0), object(1), object(2)}}}},
       {group(1), {{scopeItem(1), {object(3), object(4)}}}}});

  // 2. Test partialApply
  // Apply moves: (object1, container1 -> container3)
  // (object2, container2 -> container3)
  // After applying this change, we have: container0: {object0},
  // container3 : {object1, object2, object3}, container4: {object4}
  const ChangeSet changeSet1(
      {Change(object(1), container(1), -1),
       Change(object(1), container(3), 1),
       Change(object(2), container(2), -1),
       Change(object(2), container(3), 1)});
  context.clear();
  context.changes() = changeSet1;
  packer::tests::getOrchestrator(*rootExpr_).apply(context, initialAssignment);

  XLOG(INFO) << "2. Verifying partial apply";
  checkExpectedValues(
      {{group(0),
        {{scopeItem(0), {object(0)}}, {scopeItem(1), {object(1), object(2)}}}},
       {group(1), {{scopeItem(1), {object(3), object(4)}}}}});

  // 3. Test another partialApply
  // Apply moves: (object0, container0 -> container4)
  // (object3, container3 -> container4)
  // After applying this change, we have:
  // container3: {object1, object2}, container4 : {object0 , object3, object4}
  const ChangeSet changeSet2(
      {Change(object(0), container(0), -1),
       Change(object(0), container(4), 1),
       Change(object(3), container(3), -1),
       Change(object(3), container(4), 1)});
  context.clear();
  context.changes() = changeSet2;
  packer::tests::getOrchestrator(*rootExpr_).apply(context, initialAssignment);
  XLOG(INFO) << "3. Verifying another partial apply";
  checkExpectedValues(
      {{group(0), {{scopeItem(1), {object(0), object(1), object(2)}}}},
       {group(1), {{scopeItem(1), {object(3), object(4)}}}}});

  // 3. Try full applying another assignment
  // After applying this change, we have:
  // container0: {object0}
  // container1: {object3}
  // container2: {object2}
  // container3 : {object1}
  // container4 : {object4}
  const ChangeSet changeSet3(
      {Change(object(1), container(1), -1),
       Change(object(1), container(3), 1),
       Change(object(3), container(3), -1),
       Change(object(3), container(1), 1)});
  auto newAssignment =
      packer::tests::getModifiedAssignment(initialAssignment, changeSet3);
  context.clear();
  rootExpr_->fullApply(evaluator, newAssignment);
  XLOG(INFO) << "4. Verifying another full apply";
  checkExpectedValues(
      {{group(0),
        {{scopeItem(0), {object(0), object(2)}}, {scopeItem(1), {object(1)}}}},
       {group(1), {{scopeItem(0), {object(3)}}, {scopeItem(1), {object(4)}}}}});
}

CO_TEST_P(GroupScopeItemTransformUtilTest, testEvaluate) {
  const auto universe = co_await setUpUniverse();
  if (isDynamicDimensionTest) {
    co_return;
  }

  Context context;
  const TopToBottomEvaluator evaluator(context);
  // 1. Apply initial assignment
  auto initialAssignment =
      Assignment(universe->getContainers().getInitialAssignment());

  rootExpr_->fullApply(evaluator, initialAssignment);
  XLOG(INFO) << "1. Verifying full apply";
  checkExpectedValues(
      {{group(0), {{scopeItem(0), {object(0), object(1), object(2)}}}},
       {group(1), {{scopeItem(1), {object(3), object(4)}}}}});

  // 2. Test evaluate
  // Evaluating scenario:
  // container0: {object0}, container2: {object2},
  // container3 : {object1, object3}, container4: {object4}
  ChangeSet changeSet1(
      {Change(object(1), container(1), -1),
       Change(object(1), container(3), 1)});
  XLOG(INFO) << "2. Verifying evaluate";
  checkExpectedValues(
      {{group(0),
        {{scopeItem(0), {object(0), object(2)}}, {scopeItem(1), {object(1)}}}},
       {group(1), {{scopeItem(1), {object(3), object(4)}}}}},
      changeSet1);

  // 2. Test evaluate
  // Evaluating scenario:
  // container0: {object0}, container3 : {object1, object2, object3},
  // container4: {object4}
  auto changeSet2 = changeSet1;
  changeSet2.insert(Change(object(2), container(2), -1));
  changeSet2.insert(Change(object(2), container(3), 1));
  XLOG(INFO) << "3. Verifying evaluate";
  checkExpectedValues(
      {{group(0),
        {{scopeItem(0), {object(0)}}, {scopeItem(1), {object(1), object(2)}}}},
       {group(1), {{scopeItem(1), {object(3), object(4)}}}}},
      changeSet2);

  // 3. Test another evaluate
  // Evaluating scenario:
  // container0: {object0, object4}, container3 : {object1, object2, object3}
  auto changeSet3 = changeSet2;
  XLOG(INFO) << "4. Verifying evaluate";
  changeSet3.insert(Change(object(4), container(4), -1));
  changeSet3.insert(Change(object(4), container(0), 1));
  checkExpectedValues(
      {{group(0),
        {{scopeItem(0), {object(0)}}, {scopeItem(1), {object(1), object(2)}}}},
       {group(1), {{scopeItem(0), {object(4)}}, {scopeItem(1), {object(3)}}}}},
      changeSet3);
}

CO_TEST_P(
    GroupScopeItemTransformUtilTest,
    testEvaluateGroupsInSingleContainer) {
  const auto universe = co_await setUpUniverse();
  if (isDynamicDimensionTest) {
    co_return;
  }

  Context context;
  const TopToBottomEvaluator evaluator(context);
  // 1. Apply assignment where all group objects are in same container
  auto initialAssignment = Assignment(
      {{container(0), {object(0), object(1), object(2)}},
       {container(4), {object(3), object(4)}}});

  rootExpr_->fullApply(evaluator, initialAssignment);
  checkExpectedValues(
      {{group(0), {{scopeItem(0), {object(0), object(1), object(2)}}}},
       {group(1), {{scopeItem(1), {object(3), object(4)}}}}});

  // 2. Test evaluate
  // Evaluating scenario:
  // container0: {object0, object1, object3}
  // container4: {object2, object4}
  ChangeSet changeSet(
      {Change(object(2), container(0), -1),
       Change(object(2), container(4), 1),
       Change(object(3), container(4), -1),
       Change(object(3), container(0), 1)});
  checkExpectedValues(
      {{group(0),
        {{scopeItem(0), {object(0), object(1)}}, {scopeItem(1), {object(2)}}}},
       {group(1), {{scopeItem(0), {object(3)}}, {scopeItem(1), {object(4)}}}}},
      changeSet);
}

CO_TEST_P(GroupScopeItemTransformUtilTest, testApplyWithDynamicDimensions) {
  const auto universe = co_await setUpUniverse();
  if (!isDynamicDimensionTest) {
    co_return;
  }

  // Initial assignment: container0: {object0}, container1: {object1},
  // container2: {object2}, container3 : {object3}, container4: {object4,
  // object5} group0 = {object0, object1, object2}, group1 = {object3, object4,
  // object5}
  // dynamic dimension: object contribution {object0 : 2, object1: 1, object2:
  // 2} to containers {0,1,2} and {object3 : 1, object4: 2, object2: 1} to
  // containers {3,4} and zero otherwise

  Context context;
  const TopToBottomEvaluator evaluator(context);
  auto initialAssignment =
      Assignment(universe->getContainers().getInitialAssignment());
  rootExpr_->fullApply(evaluator, initialAssignment);
  const folly::F14FastMap<
      GroupScopeItemTransformUtil::TransformFunctionType,
      folly::F14FastMap<std::string, double>>
      expectedValueInitialAssignment = {
          {GroupScopeItemTransformUtil::TransformFunctionType::STEP,
           {{"group0ScopeItemUtil_", 1},
            {"group1ScopeItemUtil_", 1},
            {"group0ScopeItemUtilWeighted_", 2},
            {"group0ScopeItemUtilNormalized_", 1}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY,
           {{"group0ScopeItemUtil_", 5},
            {"group1ScopeItemUtil_", 3},
            {"group0ScopeItemUtilWeighted_", 10},
            {"group0ScopeItemUtilNormalized_", 2.5}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::SQUARE,
           {{"group0ScopeItemUtil_", 25},
            {"group1ScopeItemUtil_", 9},
            {"group0ScopeItemUtilWeighted_", 50},
            {"group0ScopeItemUtilNormalized_", 6.25}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K,
           // group0 util = {scopeItem0: 5 (not divisible by 3), scopeItem1: 0
           // (divisible by 2)} penalty = 1 + 0 = 1
           // group1 util = {scopeItem0: 0 (divisible by 3), scopeItem1: 3
           // (not divisible by 2)} penalty = 0 + 1 = 0
           {{"group0ScopeItemUtil_", 1},
            {"group1ScopeItemUtil_", 1},
            {"group0ScopeItemUtilWeighted_", 2},
            {"group0ScopeItemUtilNormalized_", 1}}}};

  checkExpectedValuesWithDynamicDimensions(expectedValueInitialAssignment);

  // Test partialApply
  // Apply moves: (object1, container1 -> container3)
  // (object2, container2 -> container3)
  // After applying this change, we have: container0: {object0},
  // container3 : {object1, object2, object3}, container4: {object4}
  // The utilization of object1 outside of scopeItem0 is 0
  // The utilization of object2 in scopeItem1 is 1
  const ChangeSet changeSet1(
      {Change(object(1), container(1), -1),
       Change(object(1), container(3), 1),
       Change(object(2), container(2), -1),
       Change(object(2), container(3), 1)});
  context.clear();
  context.changes() = changeSet1;
  packer::tests::getOrchestrator(*rootExpr_).apply(context, initialAssignment);

  const folly::F14FastMap<
      GroupScopeItemTransformUtil::TransformFunctionType,
      folly::F14FastMap<std::string, double>>
      expectedValueAfterChange = {
          {GroupScopeItemTransformUtil::TransformFunctionType::STEP,
           {{"group0ScopeItemUtil_", 2},
            {"group1ScopeItemUtil_", 1},
            {"group0ScopeItemUtilWeighted_", 3},
            {"group0ScopeItemUtilNormalized_", 2}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY,
           {{"group0ScopeItemUtil_", 3},
            {"group1ScopeItemUtil_", 3},
            {"group0ScopeItemUtilWeighted_", 5},
            {"group0ScopeItemUtilNormalized_", 1.5}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::SQUARE,
           {{"group0ScopeItemUtil_", 5},
            {"group1ScopeItemUtil_", 9},
            {"group0ScopeItemUtilWeighted_", 9},
            {"group0ScopeItemUtilNormalized_", 1.25}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K,
           // group0 util = {scopeItem0: 1 (not divisible by 3), scopeItem1: 1
           // (not divisible by 2)} penalty = 1 + 1 = 2
           // group1 util = {scopeItem0: 0 (divisible by 3), scopeItem1: 3
           // (not divisible by 2)} penalty = 0 + 1 = 1
           {{"group0ScopeItemUtil_", 2},
            {"group1ScopeItemUtil_", 1},
            {"group0ScopeItemUtilWeighted_", 3},
            {"group0ScopeItemUtilNormalized_", 2}}}};
  checkExpectedValuesWithDynamicDimensions(expectedValueAfterChange);
}

CO_TEST_P(GroupScopeItemTransformUtilTest, testEvaluateWithDynamicDimensions) {
  const auto universe = co_await setUpUniverse();
  if (!isDynamicDimensionTest) {
    co_return;
  }

  // Initial assignment: container0: {object0}, container1: {object1},
  // container2: {object2}, container3 : {object3}, container4: {object4,
  // object5} group0 = {object0, object1, object2}, group1 = {object3, object4,
  // object5}
  // dynamic dimension: object contribution {object0 : 2, object1: 1, object2:
  // 2} to containers {0,1,2} and {object3 : 1, object4: 2, object2: 1} to
  // containers {3,4} and zero otherwise

  Context context;
  const TopToBottomEvaluator evaluator(context);
  auto initialAssignment =
      Assignment(universe->getContainers().getInitialAssignment());
  rootExpr_->fullApply(evaluator, initialAssignment);
  const folly::F14FastMap<
      GroupScopeItemTransformUtil::TransformFunctionType,
      folly::F14FastMap<std::string, double>>
      expectedValueInitialAssignment = {
          {GroupScopeItemTransformUtil::TransformFunctionType::STEP,
           {{"group0ScopeItemUtil_", 1},
            {"group1ScopeItemUtil_", 1},
            {"group0ScopeItemUtilWeighted_", 2},
            {"group0ScopeItemUtilNormalized_", 1}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY,
           {{"group0ScopeItemUtil_", 5},
            {"group1ScopeItemUtil_", 3},
            {"group0ScopeItemUtilWeighted_", 10},
            {"group0ScopeItemUtilNormalized_", 2.5}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::SQUARE,
           {{"group0ScopeItemUtil_", 25},
            {"group1ScopeItemUtil_", 9},
            {"group0ScopeItemUtilWeighted_", 50},
            {"group0ScopeItemUtilNormalized_", 6.25}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K,
           // group0 util = {scopeItem0: 5 (not divisible by 3), scopeItem1: 0
           // (divisible by 2)} penalty = 1 + 0 = 1
           // group1 util = {scopeItem0: 0 (divisible by 3), scopeItem1: 3
           // (not divisible by 2)} penalty = 0 + 1 = 1
           {{"group0ScopeItemUtil_", 1},
            {"group1ScopeItemUtil_", 1},
            {"group0ScopeItemUtilWeighted_", 2},
            {"group0ScopeItemUtilNormalized_", 1}}}};

  checkExpectedValuesWithDynamicDimensions(expectedValueInitialAssignment);

  // Test evaluate
  // Apply move: (object2, container2 -> container3)
  // After applying this change, we have: container0: {object0}, container1:
  // {object1}, container3 : {object2, object3}, container4: {object4}
  // The utilization of object2 in scopeItem1 is 1
  ChangeSet changeSet1(
      {Change(object(2), container(2), -1),
       Change(object(2), container(3), 1)});

  const folly::F14FastMap<
      GroupScopeItemTransformUtil::TransformFunctionType,
      folly::F14FastMap<std::string, double>>
      expectedValueAfterChange = {
          {GroupScopeItemTransformUtil::TransformFunctionType::STEP,
           {{"group0ScopeItemUtil_", 2},
            {"group1ScopeItemUtil_", 1},
            {"group0ScopeItemUtilWeighted_", 3},
            {"group0ScopeItemUtilNormalized_", 2}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY,
           {{"group0ScopeItemUtil_", 4},
            {"group1ScopeItemUtil_", 3},
            {"group0ScopeItemUtilWeighted_", 7},
            {"group0ScopeItemUtilNormalized_", 2}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::SQUARE,
           {{"group0ScopeItemUtil_", 10},
            {"group1ScopeItemUtil_", 9},
            {"group0ScopeItemUtilWeighted_", 19},
            {"group0ScopeItemUtilNormalized_", 2.5}}},
          {GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K,
           // group0 util = {scopeItem0: 3 (divisible by 3), scopeItem1: 1
           // (not divisible by 2)} penalty = 0 + 1 = 1
           // group1 util = {scopeItem0: 0 (divisible by 3), scopeItem1: 3
           // (not divisible by 2)} penalty = 0 + 1 = 1
           // normalization constant = 1 for STEP_MOD_K, so normalization has no
           // effect
           {{"group0ScopeItemUtil_", 1},
            {"group1ScopeItemUtil_", 1},
            {"group0ScopeItemUtilWeighted_", 1},
            {"group0ScopeItemUtilNormalized_", 1}}}};
  checkExpectedValuesWithDynamicDimensions(
      expectedValueAfterChange, changeSet1);
}

CO_TEST_P(GroupScopeItemTransformUtilTest, testStaticEquivalenceSets) {
  const auto universe = co_await setUpUniverse();
  if (isDynamicDimensionTest ||
      transformFunctionTypeParam !=
          GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY) {
    co_return;
  }

  EquivalenceSets equivalenceSets(*universe);
  group0ScopeItemUtil_->updateEquivalenceSets(equivalenceSets);
  // 2 equivalence sets
  // object(0) and object(2) in one set as they have the same dimension value
  // object(1) will be in a different equivalence set
  EXPECT_EQ(equivalenceSets.size(), 2);
  auto equivSetId = equivalenceSets.at(object(0));
  EXPECT_EQ(equivSetId, equivalenceSets.at(object(2)));
  EXPECT_NE(equivSetId, equivalenceSets.at(object(1)));
}

CO_TEST_P(GroupScopeItemTransformUtilTest, testDynamicEquivalenceSets) {
  const auto universe = co_await setUpUniverse();
  if (!isDynamicDimensionTest ||
      transformFunctionTypeParam !=
          GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY) {
    co_return;
  }

  EquivalenceSets equivalenceSets(*universe);
  group0ScopeItemUtil_->updateEquivalenceSets(equivalenceSets);
  // 3 equivalence sets
  // object(0) and object(2) have the same dimension value in scopeItem0, but
  // not in scopeItem1 so they are in different equivalence sets
  EXPECT_EQ(equivalenceSets.size(), 3);
  auto equivSetIdObject0 = equivalenceSets.at(object(0));
  auto equivSetIdObject1 = equivalenceSets.at(object(1));
  auto equivSetIdObject2 = equivalenceSets.at(object(2));
  EXPECT_NE(equivSetIdObject0, equivSetIdObject1);
  EXPECT_NE(equivSetIdObject0, equivSetIdObject2);
  EXPECT_NE(equivSetIdObject1, equivSetIdObject2);
}

CO_TEST_P(GroupScopeItemTransformUtilTest, TestGetProperties) {
  const auto universe = co_await setUpUniverse();
  const auto properties = *group0ScopeItemUtil_->getProperties().properties();
  EXPECT_EQ(5, properties.size());

  // Test partition property
  CO_ASSERT_TRUE(properties.contains("partition"));
  EXPECT_EQ("partition", properties.at("partition").valueString()->value());

  // Test group property
  CO_ASSERT_TRUE(properties.contains("group"));
  EXPECT_EQ("group0", properties.at("group").valueString()->value());

  // Test scope property
  CO_ASSERT_TRUE(properties.contains("scope"));
  EXPECT_EQ("scope", properties.at("scope").valueString()->value());

  // Test allowed scope items property (comma-separated names)
  CO_ASSERT_TRUE(properties.contains("allowed scope items"));
  const auto& allowedScopeItemsStr =
      *properties.at("allowed scope items").valueString()->value();
  EXPECT_FALSE(allowedScopeItemsStr.empty());
  EXPECT_TRUE(
      allowedScopeItemsStr.find("all scope items in scope 'scope'") !=
      std::string::npos);

  // Test transform type property
  EXPECT_TRUE(properties.contains("transform_type"));
  const auto& transformTypeStr =
      *properties.at("transform_type").valueString()->value();
  // The actual transform type depends on the test parameter
  switch (transformFunctionTypeParam) {
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP:
      EXPECT_EQ("step(x)", transformTypeStr);
      break;
    case GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY:
      EXPECT_EQ("f(x) = x", transformTypeStr);
      break;
    case GroupScopeItemTransformUtil::TransformFunctionType::SQUARE:
      EXPECT_EQ("f(x) = x^2", transformTypeStr);
      break;
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K:
      EXPECT_EQ("step(x mod k)", transformTypeStr);
      break;
  }
}

CO_TEST_P(
    GroupScopeItemTransformUtilTest,
    testBoundsWithEmptyBoundConstraintsDefaultWeights) {
  const auto universe = co_await setUpUniverse();
  // Test with default weights (weight=1.0 for all scope items)
  Context context;
  auto bounds0 = group0ScopeItemUtil_->lowerAndUpperBounds(context);
  auto bounds1 = group1ScopeItemUtil_->lowerAndUpperBounds(context);

  // Lower bounds should always be 0.0 when bound constraints are empty
  EXPECT_EQ(bounds0.lower_bound, 0.0);
  EXPECT_EQ(bounds1.lower_bound, 0.0);

  // Static and dynamic dimensions: group0 has 3 relevant objects with total
  // value=5 group1 has 2 relevant objects with total value=3 2 scope items
  // available
  switch (transformFunctionTypeParam) {
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP:
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K: {
      // Max: min(relevantObjects, numScopeItems) * defaultWeight
      EXPECT_EQ(bounds0.upper_bound, 2.0); // min(3,2) * 1.0
      EXPECT_EQ(bounds1.upper_bound, 2.0); // min(2,2) * 1.0
      break;
    }
    case GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY: {
      // Max: all objects in one scope item with default weight
      EXPECT_EQ(bounds0.upper_bound, 5.0); // totalValue=5 * weight(=1)
      EXPECT_EQ(bounds1.upper_bound, 3.0); // totalValue=3 * weight(=1)
      break;
    }
    case GroupScopeItemTransformUtil::TransformFunctionType::SQUARE: {
      // Max: (totalValue)^2 * defaultWeight
      EXPECT_EQ(bounds0.upper_bound, 25.0); // 5^2 * 1
      EXPECT_EQ(bounds1.upper_bound, 9.0); // 3^2 * 1
      break;
    }
  }
}

CO_TEST_P(
    GroupScopeItemTransformUtilTest,
    testBoundsWithEmptyBoundConstraintsCustomWeights) {
  const auto universe = co_await setUpUniverse();
  // Test with custom weights (scopeItem0=2, scopeItem1=1)
  Context context;
  auto bounds0Weighted =
      group0ScopeItemUtilWeighted_->lowerAndUpperBounds(context);

  // Lower bound should always be 0.0 when bound constraints are empty
  EXPECT_EQ(bounds0Weighted.lower_bound, 0.0);

  // Static and dynamic dimensions: group0 has 3 relevant objects with total
  // value=5 scopeItem0 weight=2, scopeItem1 weight=1
  switch (transformFunctionTypeParam) {
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP:
    case GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K: {
      // Max: heaviest weights for occupied scope items
      // Can occupy min(3,2)=2 scope items, so scopeItem0 (weight=2) +
      // scopeItem1 (weight=1)
      EXPECT_EQ(bounds0Weighted.upper_bound, 3.0); // 2 + 1 = 3
      break;
    }
    case GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY: {
      // Max: all objects in heaviest scope item (scopeItem0 with weight=2)
      EXPECT_EQ(
          bounds0Weighted.upper_bound,
          10.0); // totalValue=5 * heaviestWeight=2
      break;
    }
    case GroupScopeItemTransformUtil::TransformFunctionType::SQUARE: {
      // Max: (totalValue)^2 * heaviest weight
      EXPECT_EQ(bounds0Weighted.upper_bound, 50.0); // 5^2 * heaviestWeight=2
      break;
    }
  }
}

CO_TEST_P(
    GroupScopeItemTransformUtilTest,
    testBoundsWithEmptyBoundConstraintsMoreScopeItemsThanObjects) {
  // group2 has only 1 object
  const auto universe = co_await setUpUniverse();
  const auto scopeId = ExpressionTestsBase::scopeId("scope");
  const auto dimensionId = ExpressionTestsBase::dimensionId("dimension");
  Context context;
  auto group2ScopeItemUtil = makeGroupUtilExpr(
      {.groupId = group(2),
       .dimensionId = dimensionId,
       .allowedScopeItems = universe->getScope(scopeId).getScopeItemIds()},
      universe);
  auto bounds1 = group2ScopeItemUtil->lowerAndUpperBounds(context);

  EXPECT_EQ(bounds1.lower_bound, 0.0);

  if (!isDynamicDimensionTest) {
    // For group2: 1 relevant object, 2 scope items available
    // This tests the case where relevantObjects < numScopeItems
    switch (transformFunctionTypeParam) {
      case GroupScopeItemTransformUtil::TransformFunctionType::STEP:
      case GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K: {
        // Max: min(1 object, 2 scope items) = 1 * weight(=1.0) = 1.0
        EXPECT_EQ(bounds1.upper_bound, 1.0);
        break;
      }
      case GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY: {
        // Max: all objects in one scope item = 1.0 (default value) * weight
        // (1.0) = 1.0
        EXPECT_EQ(bounds1.upper_bound, 1.0);
        break;
      }
      case GroupScopeItemTransformUtil::TransformFunctionType::SQUARE: {
        // Max: (1.0)^2 * weight(=1.0) = 1.0
        EXPECT_EQ(bounds1.upper_bound, 1.0);
        break;
      }
    }
  } else {
    // Dynamic dimensions
    switch (transformFunctionTypeParam) {
      case GroupScopeItemTransformUtil::TransformFunctionType::STEP:
      case GroupScopeItemTransformUtil::TransformFunctionType::STEP_MOD_K: {
        // default value is zero for dynamic dimensions and object(5) in
        // group(2) has default value
        EXPECT_EQ(bounds1.upper_bound, 0.0);
        break;
      }
      case GroupScopeItemTransformUtil::TransformFunctionType::IDENTITY: {
        EXPECT_EQ(bounds1.upper_bound, 0.0);
        break;
      }
      case GroupScopeItemTransformUtil::TransformFunctionType::SQUARE: {
        EXPECT_EQ(bounds1.upper_bound, 0.0);
        break;
      }
    }
  }
}

// Regression test: empty allowedScopeItems previously caused a segfault
// in getMaxPossibleExpressionValue() by dereferencing .begin() on an empty
// collection. The expression should construct safely and evaluate to 0.
CO_TEST_P(
    GroupScopeItemTransformUtilTest,
    testConstructionWithEmptyAllowedScopeItems) {
  const auto universe = co_await setUpUniverse();
  const auto dimensionId = ExpressionTestsBase::dimensionId("dimension");

  auto expr = makeGroupUtilExpr(
      {.groupId = group(0),
       .dimensionId = dimensionId,
       .allowedScopeItems = {}},
      universe);

  Context context;
  const TopToBottomEvaluator evaluator(context);
  auto assignment =
      Assignment(universe->getContainers().getInitialAssignment());
  expr->fullApply(evaluator, assignment);
  EXPECT_EQ(expr->value, 0.0);
}

} // namespace facebook::rebalancer::packer::tests
