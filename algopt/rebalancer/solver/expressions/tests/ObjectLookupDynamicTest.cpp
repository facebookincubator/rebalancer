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

#include "algopt/rebalancer/algopt_common/TestUtils.h"
#include "algopt/rebalancer/entities/tests/Utils.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionTestsBase.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {

class ObjectLookupDynamicTest : public ExpressionTestsBase {};

CO_TEST_F(ObjectLookupDynamicTest, Basic) {
  // Setup: Create initial assignment to register objects and containers
  setInitialAssignment(
      folly::F14FastMap<std::string, std::vector<std::string>>{
          {"container0", {"object0", "object1"}}});

  // Create a dynamic dimension with a scope
  co_await addScope("scope", {{"scopeItem0", {"container0"}}});

  const auto dynamicDimScopeId = scopeId("scope");
  const auto container0ScopeItem = scopeItemId(dynamicDimScopeId, "scopeItem0");

  const auto numObjects = getNumObjects();
  auto dynamicDimensionData = std::make_unique<const entities::ObjectDimension>(
      dynamicDimScopeId,
      entities::Map<
          entities::ScopeItemId,
          std::shared_ptr<const entities::ObjectIdToDoubleMap>>{
          {container0ScopeItem,
           entities::tests::makeSharedObjectIdToDoubleMap(
               {{object(0), 10}, {object(1), 5}},
               /*defaultValue=*/0.0,
               numObjects)}},
      0,
      numObjects);
  co_await addObjectDimension(
      "dynamicDim",
      entities::ObjectDimensionData{std::move(dynamicDimensionData)});

  const auto universe = buildUniverse();
  const auto dynamicDimId = dimensionId("dynamicDim");
  const auto& dynamicDimension =
      universe->getObjects().getDimension(dynamicDimId).at(0);

  // Create ObjectLookup child for scopeItem0's container
  auto objVec = makeObjectVector({{object(0), 10}, {object(1), 5}}, universe);
  auto containers = std::make_shared<PackerSet<entities::ContainerId>>(
      PackerSet<entities::ContainerId>{container(0)});
  auto sumOfLookups = const_expr(0, universe);
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  sumOfLookups +=
      object_lookup(objVec, containers, universe, initialAssignment);

  // Create ObjectLookupDynamic for scopeItem0
  auto lookupDynamic =
      object_lookup_dynamic(sumOfLookups, dynamicDimension, universe);

  EXPECT_EQ("ObjectLookupDynamic", lookupDynamic->getType());

  // Test evaluation: both objects in container(0)
  const Assignment assignment({{container(0), {object(0), object(1)}}});
  EXPECT_EQ(15, apply(lookupDynamic, assignment));
}

CO_TEST_F(ObjectLookupDynamicTest, ThrowsOnNonDynamicDimension) {
  setInitialAssignment(
      folly::F14FastMap<std::string, std::vector<std::string>>{
          {"container0", {"object0"}}});

  co_await addScope("scope", {{"scopeItem", {"container0"}}});

  // Create a static dimension (not dynamic)
  auto staticDimensionData = std::make_unique<const entities::ObjectDimension>(
      entities::tests::makeObjectIdToDoubleMap(
          {{object(0), 10}}, /*defaultValue=*/5.0, getNumObjects()));
  co_await addObjectDimension(
      "staticDim",
      entities::ObjectDimensionData{std::move(staticDimensionData)});

  const auto universe = buildUniverse();
  const auto staticDimId = dimensionId("staticDim");
  const auto& staticDimension =
      universe->getObjects().getDimension(staticDimId).at(0);

  auto objVec = makeObjectVector({{object(0), 10}}, universe);
  auto containers = std::make_shared<PackerSet<entities::ContainerId>>(
      PackerSet<entities::ContainerId>{container(0)});
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  auto lookup = object_lookup(objVec, containers, universe, initialAssignment);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      object_lookup_dynamic({lookup}, staticDimension, universe),
      "ObjectLookupDynamic can only be used with dynamic dimensions");
}

CO_TEST_F(ObjectLookupDynamicTest, ContainerCoverage) {
  setInitialAssignment(
      folly::F14FastMap<std::string, std::vector<std::string>>{
          {"container0", {"object0"}},
          {"container1", {"object1"}},
          {"container2", {"object2"}}});

  co_await addScope("scope", {{"scopeItem", {"container0", "container1"}}});

  // create another scope for dimension
  co_await addScope(
      "dynamic_dim_scope",
      {{"scopeItem0", {"container0"}},
       {"scopeItem1", {"container1"}},
       {"scopeItem2", {"container2"}}});

  const auto dynamicDimScopeId = scopeId("dynamic_dim_scope");
  const auto container0ScopeItem = scopeItemId(dynamicDimScopeId, "scopeItem0");
  const auto container1ScopeItem = scopeItemId(dynamicDimScopeId, "scopeItem1");
  const auto container2ScopeItem = scopeItemId(dynamicDimScopeId, "scopeItem2");

  const auto numObjects = getNumObjects();
  auto dynamicDimensionData = std::make_unique<const entities::ObjectDimension>(
      dynamicDimScopeId,
      entities::Map<
          entities::ScopeItemId,
          std::shared_ptr<const entities::ObjectIdToDoubleMap>>{
          {container0ScopeItem,
           entities::tests::makeSharedObjectIdToDoubleMap(
               {{object(0), 10}}, /*defaultValue=*/0.0, numObjects)},
          {container1ScopeItem,
           entities::tests::makeSharedObjectIdToDoubleMap(
               {{object(1), 10}}, /*defaultValue=*/0.0, numObjects)},
          {container2ScopeItem,
           entities::tests::makeSharedObjectIdToDoubleMap(
               {{object(2), 10}}, /*defaultValue=*/0.0, numObjects)}},
      0,
      numObjects);
  co_await addObjectDimension(
      "dynamicDim",
      entities::ObjectDimensionData{std::move(dynamicDimensionData)});

  const auto universe = buildUniverse();
  const auto dynamicDimId = dimensionId("dynamicDim");
  const auto& dynamicDimension =
      universe->getObjects().getDimension(dynamicDimId).at(0);

  // lookup01 is not consistent with dynamic dimension's scope because
  // containers 0, 1 are not in the same scopeItem of dynamic dimension's scope
  auto objVec = makeObjectVector({{object(0), 10}}, universe);
  auto container01 = std::make_shared<PackerSet<entities::ContainerId>>(
      PackerSet<entities::ContainerId>{container(0), container(1)});
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  auto lookup01 = object_lookup(
      makeObjectVector({{object(0), 10}}, universe),
      container01,
      universe,
      initialAssignment);
  auto sumOfLookups01 = const_expr(0, universe);
  sumOfLookups01 += lookup01;
  REBALANCER_EXPECT_RUNTIME_ERROR(
      object_lookup_dynamic(sumOfLookups01, dynamicDimension, universe),
      "container set not consistent with dimension's scope dynamic_dim_scope");

  auto sumOfLookups = const_expr(0, universe);
  auto container1Ptr = std::make_shared<PackerSet<entities::ContainerId>>(
      PackerSet<entities::ContainerId>{container(1)});
  sumOfLookups += object_lookup(
      makeObjectVector({{object(1), 10}}, universe),
      container1Ptr,
      universe,
      initialAssignment);

  auto container2Ptr = std::make_shared<PackerSet<entities::ContainerId>>(
      PackerSet<entities::ContainerId>{container(2)});
  sumOfLookups += object_lookup(
      makeObjectVector({{object(2), 10}}, universe),
      container2Ptr,
      universe,
      initialAssignment);

  auto dynamicObjLookupExpr =
      object_lookup_dynamic(sumOfLookups, dynamicDimension, universe);

  EXPECT_EQ(2, dynamicObjLookupExpr->getDirectlyAffectedContainers().size());
  EXPECT_TRUE(dynamicObjLookupExpr->getDirectlyAffectedContainers()
                  .getSetPtr()
                  ->contains(container(1)));
  EXPECT_TRUE(dynamicObjLookupExpr->getDirectlyAffectedContainers()
                  .getSetPtr()
                  ->contains(container(2)));
}

CO_TEST_F(ObjectLookupDynamicTest, EvaluateAndPartialApply) {
  // Setup with multiple objects and containers
  setInitialAssignment(
      folly::F14FastMap<std::string, std::vector<std::string>>{
          {"container0", {"object0", "object1"}},
          {"container1", {"object2", "object3"}},
          {"container2", {"object4", "object5"}}});

  co_await addScope(
      "scope",
      {{"scopeItem0", {"container0", "container1"}},
       {"scopeItem1", {"container2"}}});

  const auto dynamicDimScopeId = scopeId("scope");
  const auto scopeItem0Id = scopeItemId(dynamicDimScopeId, "scopeItem0");
  const auto scopeItem1Id = scopeItemId(dynamicDimScopeId, "scopeItem1");

  const auto numObjects = getNumObjects();
  auto dynamicDimensionData = std::make_unique<const entities::ObjectDimension>(
      dynamicDimScopeId,
      entities::Map<
          entities::ScopeItemId,
          std::shared_ptr<const entities::ObjectIdToDoubleMap>>{
          {scopeItem0Id,
           entities::tests::makeSharedObjectIdToDoubleMap(
               {{object(0), 0},
                {object(1), 1},
                {object(2), 2},
                {object(3), 3},
                {object(4), 4},
                {object(5), 5}},
               /*defaultValue=*/0.0,
               numObjects)},
          {scopeItem1Id,
           entities::tests::makeSharedObjectIdToDoubleMap(
               {{object(0), 1},
                {object(1), 2},
                {object(2), 3},
                {object(3), 4},
                {object(4), 5},
                {object(5), 6}},
               /*defaultValue=*/0.0,
               numObjects)}},
      0,
      numObjects);
  co_await addObjectDimension(
      "dynamicDim",
      entities::ObjectDimensionData{std::move(dynamicDimensionData)});

  const auto universe = buildUniverse();
  const auto dynamicDimId = dimensionId("dynamicDim");
  const auto& dynamicDimension =
      universe->getObjects().getDimension(dynamicDimId).at(0);

  // Create ObjectLookup children for scopeItem0's containers
  auto objVecScopeItem0 = makeObjectVector(
      {{object(0), 0},
       {object(1), 1},
       {object(2), 2},
       {object(3), 3},
       {object(4), 4},
       {object(5), 5}},
      universe);
  auto objVecScopeItem1 = makeObjectVector(
      {{object(0), 1},
       {object(1), 2},
       {object(2), 3},
       {object(3), 4},
       {object(4), 5},
       {object(5), 6}},
      universe);

  auto scopeItem0Containers =
      std::make_shared<PackerSet<entities::ContainerId>>(
          PackerSet<entities::ContainerId>{container(0), container(1)});
  auto sumOfLookups = const_expr(0, universe);
  const Assignment initialAssignment(
      universe->getContainers().getInitialAssignment());
  sumOfLookups += object_lookup(
      objVecScopeItem0, scopeItem0Containers, universe, initialAssignment);
  auto scopeItem1Containers =
      std::make_shared<PackerSet<entities::ContainerId>>(
          PackerSet<entities::ContainerId>{container(2)});
  sumOfLookups += object_lookup(
      objVecScopeItem1, scopeItem1Containers, universe, initialAssignment);

  auto lookupDynamic =
      object_lookup_dynamic(sumOfLookups, dynamicDimension, universe);

  const Assignment assignment(
      {{container(0), {object(0), object(1)}},
       {container(1), {object(2), object(3)}},
       {container(2), {object(4), object(5)}}});

  // Initial apply: all objects in their initial containers
  // Total = 0+1+2+3 (scopeItem0)  5+6 (scopeItem1)= 17
  EXPECT_EQ(17, apply(lookupDynamic, assignment));

  // Test evaluate with no changes
  EXPECT_EQ(17, evaluate(lookupDynamic, {}, assignment));

  // Test evaluate with changes
  const auto swap = ObjectToNewContainer{
      {object(0), container(2)},
      {object(1), container(2)},
      {object(4), container(0)},
      {object(5), container(0)},
  };

  // After swap, expression value stays the same
  // Total = 4+5+2+3 (scopeItem0)  1+2 (scopeItem1)  = 17
  EXPECT_EQ(17, evaluate(lookupDynamic, swap, assignment));

  const auto emptyScopeItem1 = ObjectToNewContainer{
      {object(4), container(0)},
      {object(5), container(1)},
  };

  // After emptying scopeItem1, expression values changes by -2
  // Total = 0+1+2+3+4+5 (scopeItem0)  0 (scopeItem1)  = 15
  EXPECT_EQ(15, evaluate(lookupDynamic, emptyScopeItem1, assignment));

  // Apply these changes
  EXPECT_EQ(15, applyChanges(lookupDynamic, emptyScopeItem1, assignment));
  const auto newAssignment = getModifiedAssignment(assignment, emptyScopeItem1);

  // Test evaluate on the new state
  // After adding objects to scopeItem1, expression values changes by +4
  // Total = 4+5 (scopeItem0)  1+2+3+4 (scopeItem1)  = 19
  const auto addObjectsToScopeItem1 = ObjectToNewContainer{
      {object(0), container(2)},
      {object(1), container(2)},
      {object(2), container(2)},
      {object(3), container(2)},
  };
  EXPECT_EQ(19, evaluate(lookupDynamic, addObjectsToScopeItem1, newAssignment));
}

} // namespace facebook::rebalancer::packer::tests
