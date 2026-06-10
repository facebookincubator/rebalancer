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
#include "algopt/rebalancer/entities/GraphEmbeddingUtils.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Scope.h"
#include "algopt/rebalancer/entities/tests/Utils.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace facebook::rebalancer::entities::tests {

TEST(ScopeTest, Constructor) {
  const auto scopeItem1Id = ScopeItemId(1);
  const auto scopeItem2Id = ScopeItemId(2);
  const auto container1Id = ContainerId(10);
  const auto container2Id = ContainerId(20);
  const auto dimension1Id = DimensionId(100);
  const auto dimension2Id = DimensionId(101);

  Map<ScopeItemId, Set<ContainerId>> scopeItems{
      {scopeItem1Id, {container1Id}}, {scopeItem2Id, {container2Id}}};

  auto dim1 = std::make_shared<ScopeDimension>(
      Map<ScopeItemId, double>{{scopeItem1Id, 5.0}}, /*defaultValue=*/1.0);
  auto dim2 = std::make_shared<ScopeDimension>(
      Map<ScopeItemId, double>{{scopeItem2Id, 10.0}}, /*defaultValue=*/2.0);

  Map<DimensionId, std::shared_ptr<const ScopeDimension>> dimensions;
  dimensions.emplace(dimension1Id, std::move(dim1));
  dimensions.emplace(dimension2Id, std::move(dim2));

  const Scope scope(
      std::make_shared<const Map<ScopeItemId, Set<ContainerId>>>(
          std::move(scopeItems)),
      std::move(dimensions));

  // Verify scope items
  EXPECT_EQ(2, scope.getScopeItemIds().size());
  EXPECT_EQ(
      Set<ContainerId>({container1Id}), scope.getContainerIds(scopeItem1Id));
  EXPECT_EQ(
      Set<ContainerId>({container2Id}), scope.getContainerIds(scopeItem2Id));

  // Verify container to scope item mapping
  EXPECT_EQ(scopeItem1Id, scope.getScopeItemId(container1Id));
  EXPECT_EQ(scopeItem2Id, scope.getScopeItemId(container2Id));

  // Verify dimensions
  EXPECT_EQ(2, toVec<DimensionId>(scope.getDimensionIds()).size());
  EXPECT_EQ(5.0, scope.getDimension(dimension1Id).getValue(scopeItem1Id));
  EXPECT_EQ(1.0, scope.getDimension(dimension1Id).getValue(scopeItem2Id));
  EXPECT_EQ(2.0, scope.getDimension(dimension2Id).getValue(scopeItem1Id));
  EXPECT_EQ(10.0, scope.getDimension(dimension2Id).getValue(scopeItem2Id));
}

TEST(ScopeTest, ToThrift) {
  Map<DimensionId, std::shared_ptr<const ScopeDimension>> dimensions;
  dimensions.emplace(
      DimensionId(2),
      std::make_shared<ScopeDimension>(Map<ScopeItemId, double>(), 20));

  const Scope scope(
      std::make_shared<const Map<ScopeItemId, Set<ContainerId>>>(
          Map<ScopeItemId, Set<ContainerId>>{
              {ScopeItemId(0), {ContainerId(1)}}}),
      std::move(dimensions));
  EXPECT_EQ(ScopeItemId(0), scope.getScopeItemId(ContainerId(1)));
  EXPECT_EQ(std::nullopt, scope.getScopeItemId(ContainerId(2)));

  auto thrift = scope.toThrift();

  const std::map<int, std::vector<int>> expectedScopeItems({{0, {1}}});
  EXPECT_EQ(expectedScopeItems, toStdMap(*thrift.scopeItems()));

  EXPECT_EQ(1, thrift.dimensions()->size());
  EXPECT_EQ(20, *thrift.dimensions()->at(2).defaultValue());
}

TEST(ScopeTest, FromThrift) {
  thrift::ScopeDimension scopeDimensionThrift;
  scopeDimensionThrift.defaultValue() = 20;

  thrift::Scope scopeThrift;
  scopeThrift.scopeItems() = {{0, {1}}};
  scopeThrift.dimensions() = {{2, scopeDimensionThrift}};

  const Scope scope(scopeThrift);

  EXPECT_EQ(
      std::vector<ScopeItemId>({ScopeItemId(0)}),
      toVec<entities::ScopeItemId>(scope.getScopeItemIds()));
  EXPECT_EQ(
      Set<ContainerId>({ContainerId(1)}),
      scope.getContainerIds(ScopeItemId(0)));
  EXPECT_EQ(
      std::vector<DimensionId>({DimensionId(2)}),
      toVec<entities::DimensionId>(scope.getDimensionIds()));
  EXPECT_EQ(20, scope.getDimension(DimensionId(2)).getDefaultValue());
  EXPECT_EQ(1, scope.getDimension(DimensionId(3)).getDefaultValue());
  EXPECT_EQ(ScopeItemId(0), scope.getScopeItemId(ContainerId(1)));
  EXPECT_EQ(std::nullopt, scope.getScopeItemId(ContainerId(2)));
}

TEST(ScopeTest, ToEmbedding) {
  const auto scopeItem0 = ScopeItemId(0);
  const auto scopeItem1 = ScopeItemId(1);
  const auto dim0 = DimensionId(0);
  const auto dim1 = DimensionId(1);

  auto ramDim = std::make_shared<ScopeDimension>(
      Map<ScopeItemId, double>{{scopeItem0, 16.0}, {scopeItem1, 32.0}}, 1.0);
  auto cpuDim = std::make_shared<ScopeDimension>(
      Map<ScopeItemId, double>{{scopeItem0, 4.0}, {scopeItem1, 8.0}}, 1.0);

  Map<DimensionId, std::shared_ptr<const ScopeDimension>> dimensions;
  dimensions.emplace(dim0, std::move(ramDim));
  dimensions.emplace(dim1, std::move(cpuDim));

  const Scope scope(
      std::make_shared<const Map<ScopeItemId, Set<ContainerId>>>(
          Map<ScopeItemId, Set<ContainerId>>{
              {scopeItem0, {ContainerId(10)}},
              {scopeItem1, {ContainerId(20)}}}),
      std::move(dimensions));

  const std::vector<DimensionId> dimIds{dim0, dim1};
  const auto vertices = scope.toEmbedding(dimIds);

  std::vector<double> scopeItemOneHot(kNumVertexTypes, 0.0);
  scopeItemOneHot[static_cast<std::size_t>(thrift::VertexType::SCOPE_ITEM)] =
      1.0;

  thrift::GraphVertex expectedVertex0;
  expectedVertex0.type() = thrift::VertexType::SCOPE_ITEM;
  expectedVertex0.entityId() = 0;
  expectedVertex0.embedding() = scopeItemOneHot;
  expectedVertex0.embedding()->insert(
      expectedVertex0.embedding()->end(), {16.0, 4.0});

  thrift::GraphVertex expectedVertex1;
  expectedVertex1.type() = thrift::VertexType::SCOPE_ITEM;
  expectedVertex1.entityId() = 1;
  expectedVertex1.embedding() = scopeItemOneHot;
  expectedVertex1.embedding()->insert(
      expectedVertex1.embedding()->end(), {32.0, 8.0});

  const std::vector<thrift::GraphVertex> expected{
      expectedVertex0, expectedVertex1};
  // scopeItemIds_ is drawn from a hash map so toEmbedding's output order is
  // non-deterministic. Sort by entityId in the test rather than in production.
  auto sortedVertices = vertices;
  std::sort(
      sortedVertices.begin(),
      sortedVertices.end(),
      [](const auto& a, const auto& b) {
        return *a.entityId() < *b.entityId();
      });
  EXPECT_EQ(expected, sortedVertices);
}

TEST(ScopeTest, ContainerPartOfMultipleScopeItems) {
  {
    REBALANCER_EXPECT_RUNTIME_ERROR(
        const Scope scope(
            std::make_shared<const Map<ScopeItemId, Set<ContainerId>>>(
                Map<ScopeItemId, Set<ContainerId>>{
                    {ScopeItemId(0), {ContainerId(1), ContainerId(2)}},
                    {ScopeItemId(10), {ContainerId(1)}}}),
            Map<DimensionId, std::shared_ptr<const ScopeDimension>>{}),
        "containerId 1 appears as part of more than one scope item");
  }

  {
    thrift::Scope scopeThrift;
    scopeThrift.scopeItems() = {{0, {1, 2}}, {10, {2}}};
    REBALANCER_EXPECT_RUNTIME_ERROR(
        const Scope scope(scopeThrift),
        "containerId 2 appears as part of more than one scope item");
  }
}

} // namespace facebook::rebalancer::entities::tests
