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

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/tests/Utils.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::entities::tests {

namespace {
template <typename T, typename U, typename = void>
struct is_equality_comparable : std::false_type {};

template <typename T, typename U>
struct is_equality_comparable<
    T,
    U,
    std::void_t<decltype(std::declval<const T&>() == std::declval<const U&>())>>
    : std::true_type {};

// Verifies all six operators produce correct results for a given ID type.
template <typename IdType>
void verifySameTypeOperations() {
  const auto a = IdType(1);
  const auto b = IdType(2);
  const auto a2 = IdType(1);

  EXPECT_TRUE(a == a2);
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
  EXPECT_FALSE(a != a2);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
  EXPECT_TRUE(b > a);
  EXPECT_FALSE(a > b);
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(a <= a2);
  EXPECT_TRUE(b >= a);
  EXPECT_TRUE(a >= a2);
}

} // namespace

TEST(IdentifiersTest, Constructor) {
  IdStoreConfig config;
  config.objectNames = {"object1"};
  config.containerNames = {"container1"};
  config.scopeNames = {"scope1"};
  config.scopeItemNames = {"scopeItem1"};
  config.dimensionNames = {"dimension1"};
  config.partitionNames = {"partition1"};
  config.groupNames = {"group1"};
  config.goalNames = {"goal1"};
  config.constraintNames = {"constraint1"};
  config.routingConfigNames = {"routingConfig1"};
  config.objectNameToId = {{"object1", ObjectId(0)}};
  config.containerNameToId = {{"container1", ContainerId(0)}};
  config.scopeNameToId = {{"scope1", ScopeId(0)}};
  config.scopeIdToScopeItemNameToId = {
      {ScopeId(0),
       std::make_shared<Map<std::string, ScopeItemId>>(
           Map<std::string, ScopeItemId>{{"scopeItem1", ScopeItemId(0)}})}};
  config.dimensionNameToId = {{"dimension1", DimensionId(0)}};
  config.partitionNameToId = {{"partition1", PartitionId(0)}};
  config.partitionIdToGroupNameToId = {
      {PartitionId(0),
       std::make_shared<const Map<std::string, GroupId>>(
           Map<std::string, GroupId>{{"group1", GroupId(0)}})}};
  config.goalNameToId = {{"goal1", GoalId(0)}};
  config.constraintNameToId = {{"constraint1", ConstraintId(0)}};
  config.routingConfigNameToId = {{"routingConfig1", RoutingConfigId(0)}};

  const IdStore idStore(std::move(config));

  EXPECT_EQ(ObjectId(0), idStore.getObjectId("object1"));
  EXPECT_EQ(ContainerId(0), idStore.getContainerId("container1"));
  EXPECT_EQ(ScopeId(0), idStore.getScopeId("scope1"));
  EXPECT_EQ(ScopeItemId(0), idStore.getScopeItemId(ScopeId(0), "scopeItem1"));
  EXPECT_EQ(DimensionId(0), idStore.getDimensionId("dimension1"));
  EXPECT_EQ(PartitionId(0), idStore.getPartitionId("partition1"));
  EXPECT_EQ(GroupId(0), idStore.getGroupId(PartitionId(0), "group1"));
  EXPECT_EQ(GoalId(0), idStore.getGoalId("goal1"));
  EXPECT_EQ(ConstraintId(0), idStore.getConstraintId("constraint1"));
  EXPECT_EQ(RoutingConfigId(0), idStore.getRoutingConfigId("routingConfig1"));

  EXPECT_EQ("object1", idStore.getEntityName(ObjectId(0)));
  EXPECT_EQ("container1", idStore.getEntityName(ContainerId(0)));
  EXPECT_EQ("scope1", idStore.getEntityName(ScopeId(0)));
  EXPECT_EQ("scopeItem1", idStore.getEntityName(ScopeItemId(0)));
  EXPECT_EQ("dimension1", idStore.getEntityName(DimensionId(0)));
  EXPECT_EQ("partition1", idStore.getEntityName(PartitionId(0)));
  EXPECT_EQ("group1", idStore.getEntityName(GroupId(0)));
  EXPECT_EQ("goal1", idStore.getEntityName(GoalId(0)));
  EXPECT_EQ("constraint1", idStore.getEntityName(ConstraintId(0)));
  EXPECT_EQ("routingConfig1", idStore.getEntityName(RoutingConfigId(0)));
}

TEST(IdentifiersTest, MultipleEntities) {
  IdStoreConfig config;
  config.objectNames = {"object1", "object2"};
  config.containerNames = {"container1", "container2"};
  config.scopeNames = {"scope1", "scope2"};
  config.scopeItemNames = {"scopeItem1"};
  config.partitionNames = {"partition1"};
  config.groupNames = {"group1"};
  config.dimensionNames = {"dimension1"};
  config.goalNames = {"goal1"};
  config.constraintNames = {"constraint1"};
  config.routingConfigNames = {"routingConfig1"};
  config.objectNameToId = {{"object1", ObjectId(0)}, {"object2", ObjectId(1)}};
  config.containerNameToId = {
      {"container1", ContainerId(0)}, {"container2", ContainerId(1)}};
  config.scopeNameToId = {{"scope1", ScopeId(0)}, {"scope2", ScopeId(1)}};
  config.scopeIdToScopeItemNameToId = {
      {ScopeId(0),
       std::make_shared<Map<std::string, ScopeItemId>>(
           Map<std::string, ScopeItemId>{{"scopeItem1", ScopeItemId(0)}})}};
  config.partitionNameToId = {{"partition1", PartitionId(0)}};
  config.partitionIdToGroupNameToId = {
      {PartitionId(0),
       std::make_shared<const Map<std::string, GroupId>>(
           Map<std::string, GroupId>{{"group1", GroupId(0)}})}};
  config.dimensionNameToId = {{"dimension1", DimensionId(0)}};
  config.goalNameToId = {{"goal1", GoalId(0)}};
  config.constraintNameToId = {{"constraint1", ConstraintId(0)}};
  config.routingConfigNameToId = {{"routingConfig1", RoutingConfigId(0)}};

  const IdStore idStore(std::move(config));

  const auto object1 = ObjectId(0);
  const auto object2 = ObjectId(1);
  const auto container1 = ContainerId(0);
  const auto container2 = ContainerId(1);
  const auto scope1 = ScopeId(0);
  const auto scope2 = ScopeId(1);
  const auto scopeItem1 = ScopeItemId(0);
  const auto partition1 = PartitionId(0);
  const auto group1 = GroupId(0);
  const auto dimension1 = DimensionId(0);
  const auto goal1 = GoalId(0);
  const auto constraint1 = ConstraintId(0);
  const auto routingConfig1Id = RoutingConfigId(0);

  EXPECT_EQ(object1, idStore.getObjectId("object1"));
  EXPECT_EQ(object2, idStore.getObjectId("object2"));
  EXPECT_EQ(container1, idStore.getContainerId("container1"));
  EXPECT_EQ(container2, idStore.getContainerId("container2"));
  EXPECT_EQ(scope1, idStore.getScopeId("scope1"));
  EXPECT_EQ(scope2, idStore.getScopeId("scope2"));
  EXPECT_EQ(scopeItem1, idStore.getScopeItemId(scope1, "scopeItem1"));
  EXPECT_EQ(partition1, idStore.getPartitionId("partition1"));
  EXPECT_EQ(group1, idStore.getGroupId(partition1, "group1"));
  EXPECT_EQ(dimension1, idStore.getDimensionId("dimension1"));
  EXPECT_EQ(goal1, idStore.getGoalId("goal1"));
  EXPECT_EQ(constraint1, idStore.getConstraintId("constraint1"));
  EXPECT_EQ(routingConfig1Id, idStore.getRoutingConfigId("routingConfig1"));

  EXPECT_EQ("object1", idStore.getEntityName(object1));
  EXPECT_EQ("object2", idStore.getEntityName(object2));
  EXPECT_EQ("container1", idStore.getEntityName(container1));
  EXPECT_EQ("container2", idStore.getEntityName(container2));
  EXPECT_EQ("scope1", idStore.getEntityName(scope1));
  EXPECT_EQ("scope2", idStore.getEntityName(scope2));
  EXPECT_EQ("scopeItem1", idStore.getEntityName(scopeItem1));
  EXPECT_EQ("partition1", idStore.getEntityName(partition1));
  EXPECT_EQ("group1", idStore.getEntityName(group1));
  EXPECT_EQ("dimension1", idStore.getEntityName(dimension1));
  EXPECT_EQ("goal1", idStore.getEntityName(goal1));
  EXPECT_EQ("constraint1", idStore.getEntityName(constraint1));
  EXPECT_EQ("routingConfig1", idStore.getEntityName(routingConfig1Id));
}

TEST(IdentifiersTest, DuplicateNamesDifferentEntity) {
  // Same name can be used for different entity types
  IdStoreConfig config;
  config.objectNames = {"name1"};
  config.containerNames = {"name1"};
  config.objectNameToId = {{"name1", ObjectId(0)}};
  config.containerNameToId = {{"name1", ContainerId(0)}};

  const IdStore idStore(std::move(config));

  EXPECT_EQ("name1", idStore.getEntityName(ObjectId(0)));
  EXPECT_EQ("name1", idStore.getEntityName(ContainerId(0)));
}

TEST(IdentifiersTest, ToThrift) {
  IdStoreConfig config;
  config.objectNames = {"object1"};
  config.containerNames = {"container1"};
  config.scopeNames = {"scope1"};
  config.scopeItemNames = {"scopeItem1"};
  config.partitionNames = {"partition1"};
  config.groupNames = {"group1"};
  config.dimensionNames = {"dimension1"};
  config.goalNames = {"goal1"};
  config.constraintNames = {"constraint1"};
  config.routingConfigNames = {"routingConfig1"};
  config.objectNameToId = {{"object1", ObjectId(0)}};
  config.containerNameToId = {{"container1", ContainerId(0)}};
  config.scopeNameToId = {{"scope1", ScopeId(0)}};
  config.scopeIdToScopeItemNameToId = {
      {ScopeId(0),
       std::make_shared<Map<std::string, ScopeItemId>>(
           Map<std::string, ScopeItemId>{{"scopeItem1", ScopeItemId(0)}})}};
  config.partitionNameToId = {{"partition1", PartitionId(0)}};
  config.partitionIdToGroupNameToId = {
      {PartitionId(0),
       std::make_shared<const Map<std::string, GroupId>>(
           Map<std::string, GroupId>{{"group1", GroupId(0)}})}};
  config.dimensionNameToId = {{"dimension1", DimensionId(0)}};
  config.goalNameToId = {{"goal1", GoalId(0)}};
  config.constraintNameToId = {{"constraint1", ConstraintId(0)}};
  config.routingConfigNameToId = {{"routingConfig1", RoutingConfigId(0)}};

  const IdStore idStore(std::move(config));

  auto thrift = idStore.toThrift();

  EXPECT_EQ(std::vector<std::string>({"object1"}), *thrift.objectNames());
  EXPECT_EQ(std::vector<std::string>({"container1"}), *thrift.containerNames());
  EXPECT_EQ(std::vector<std::string>({"scope1"}), *thrift.scopeNames());
  EXPECT_EQ(std::vector<std::string>({"scopeItem1"}), *thrift.scopeItemNames());
  EXPECT_EQ(std::vector<std::string>({"partition1"}), *thrift.partitionNames());
  EXPECT_EQ(std::vector<std::string>({"group1"}), *thrift.groupNames());
  EXPECT_EQ(std::vector<std::string>({"dimension1"}), *thrift.dimensionNames());
  EXPECT_EQ(std::vector<std::string>({"goal1"}), *thrift.goalNames());
  EXPECT_EQ(
      std::vector<std::string>({"constraint1"}), *thrift.constraintNames());
  EXPECT_EQ(
      std::vector<std::string>({"routingConfig1"}),
      *thrift.routingConfigNames());
  EXPECT_EQ(std::vector<int>({0}), *thrift.objectIds());
  EXPECT_EQ(std::vector<int>({0}), *thrift.containerIds());
  const std::map<int, std::vector<int>> expectedScopeItemIds = {{0, {0}}};
  EXPECT_EQ(expectedScopeItemIds, toStdMap(*thrift.scopeItemIds()));
  const std::map<int, std::vector<int>> expectedGroupIds = {{0, {0}}};
  EXPECT_EQ(expectedGroupIds, toStdMap(*thrift.groupIds()));
  EXPECT_EQ(std::vector<int>({0}), *thrift.dimensionIds());
  EXPECT_EQ(std::vector<int>({0}), *thrift.goalIds());
  EXPECT_EQ(std::vector<int>({0}), *thrift.constraintIds());
  EXPECT_EQ(std::vector<int>({0}), *thrift.routingConfigIds());
}

TEST(IdentifiersTest, FromThrift) {
  thrift::IdStore thrift;
  thrift.objectNames() = {"object1"};
  thrift.containerNames() = {"container1"};
  thrift.scopeNames() = {"scope1"};
  thrift.scopeItemNames() = {"scopeItem1"};
  thrift.partitionNames() = {"partition1"};
  thrift.groupNames() = {"group1"};
  thrift.dimensionNames() = {"dimension1"};
  thrift.goalNames() = {"goal1"};
  thrift.constraintNames() = {"constraint1"};
  thrift.routingConfigNames() = {"routingConfig1"};
  thrift.objectIds() = {0};
  thrift.containerIds() = {0};
  thrift.scopeItemIds() = {{0, {0}}};
  thrift.groupIds() = {{0, {0}}};
  thrift.dimensionIds() = {0};
  thrift.goalIds() = {0};
  thrift.constraintIds() = {0};
  thrift.routingConfigIds() = {0};
  const IdStore idStore(thrift);

  EXPECT_EQ(ObjectId(0), idStore.getObjectId("object1"));
  EXPECT_EQ(ContainerId(0), idStore.getContainerId("container1"));
  EXPECT_EQ(ScopeId(0), idStore.getScopeId("scope1"));
  EXPECT_EQ(ScopeItemId(0), idStore.getScopeItemId(ScopeId(0), "scopeItem1"));
  EXPECT_EQ(PartitionId(0), idStore.getPartitionId("partition1"));
  EXPECT_EQ(GroupId(0), idStore.getGroupId(PartitionId(0), "group1"));
  EXPECT_EQ(DimensionId(0), idStore.getDimensionId("dimension1"));
  EXPECT_EQ(GoalId(0), idStore.getGoalId("goal1"));
  EXPECT_EQ(ConstraintId(0), idStore.getConstraintId("constraint1"));
  EXPECT_EQ(RoutingConfigId(0), idStore.getRoutingConfigId("routingConfig1"));

  EXPECT_EQ("object1", idStore.getEntityName(ObjectId(0)));
  EXPECT_EQ("container1", idStore.getEntityName(ContainerId(0)));
  EXPECT_EQ("scope1", idStore.getEntityName(ScopeId(0)));
  EXPECT_EQ("scopeItem1", idStore.getEntityName(ScopeItemId(0)));
  EXPECT_EQ("partition1", idStore.getEntityName(PartitionId(0)));
  EXPECT_EQ("group1", idStore.getEntityName(GroupId(0)));
  EXPECT_EQ("dimension1", idStore.getEntityName(DimensionId(0)));
  EXPECT_EQ("goal1", idStore.getEntityName(GoalId(0)));
  EXPECT_EQ("constraint1", idStore.getEntityName(ConstraintId(0)));
  EXPECT_EQ("routingConfig1", idStore.getEntityName(RoutingConfigId(0)));
}

// Verify same-type operator semantics (==, !=, <, >, <=, >=) for every type.
TEST(IdentifiersTest, SameTypeOperations) {
  verifySameTypeOperations<ObjectId>();
  verifySameTypeOperations<ContainerId>();
  verifySameTypeOperations<ScopeId>();
  verifySameTypeOperations<ScopeItemId>();
  verifySameTypeOperations<PartitionId>();
  verifySameTypeOperations<GroupId>();
  verifySameTypeOperations<DimensionId>();
  verifySameTypeOperations<GoalId>();
  verifySameTypeOperations<ConstraintId>();
  verifySameTypeOperations<RoutingConfigId>();
  verifySameTypeOperations<EquivalenceSetId>();
}

// Cross-type comparisons must not compile; same-type must.
// Enforced at compile time via static_assert so any regression is a build
// error.
TEST(IdentifiersTest, CrossTypeComparisonsMustNotCompile) {
  // Same-type comparisons must compile.
  static_assert(is_equality_comparable<ObjectId, ObjectId>::value);
  static_assert(is_equality_comparable<ContainerId, ContainerId>::value);
  static_assert(is_equality_comparable<ScopeId, ScopeId>::value);

  // Cross-type comparisons must NOT compile.
  static_assert(!is_equality_comparable<ObjectId, ContainerId>::value);
  static_assert(!is_equality_comparable<ContainerId, ScopeId>::value);
  static_assert(!is_equality_comparable<ScopeId, ScopeItemId>::value);
  static_assert(!is_equality_comparable<ScopeItemId, PartitionId>::value);
  static_assert(!is_equality_comparable<PartitionId, GroupId>::value);
  static_assert(!is_equality_comparable<GroupId, DimensionId>::value);
  static_assert(!is_equality_comparable<DimensionId, GoalId>::value);
  static_assert(!is_equality_comparable<GoalId, ConstraintId>::value);
  static_assert(!is_equality_comparable<ConstraintId, RoutingConfigId>::value);
  static_assert(
      !is_equality_comparable<RoutingConfigId, EquivalenceSetId>::value);
  static_assert(!is_equality_comparable<EquivalenceSetId, ObjectId>::value);
}

} // namespace facebook::rebalancer::entities::tests
