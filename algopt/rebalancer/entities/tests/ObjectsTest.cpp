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
#include "algopt/rebalancer/entities/Objects.h"
#include "algopt/rebalancer/entities/tests/Utils.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::entities::tests {
TEST(ObjectsTest, Constructor) {
  const auto object1Id = ObjectId(0);
  const auto object2Id = ObjectId(1);
  const auto dimension1Id = DimensionId(100);

  const std::vector<ObjectId> objectIds{object1Id, object2Id};

  auto dim1 = std::make_shared<ObjectDimension>(ObjectIdToDoubleMap(
      Map<ObjectId, double>{{object1Id, 5.0}, {object2Id, 10.0}},
      /*defaultValue=*/1.0,
      /*totalSize=*/3));

  Map<DimensionId, std::shared_ptr<const ObjectDimension>> dimensions;
  dimensions.emplace(dimension1Id, std::move(dim1));

  const Objects objects(objectIds.size(), std::move(dimensions));

  const std::vector<ObjectId> actualIds(
      objects.getObjectIds().begin(), objects.getObjectIds().end());
  EXPECT_EQ(objectIds, actualIds);
  EXPECT_EQ(1, objects.getDimensionCount());
  EXPECT_EQ(
      std::set<DimensionId>({dimension1Id}),
      toSet<DimensionId>(objects.getDimensionIds()));
  EXPECT_EQ(1, objects.getDimension(dimension1Id).size());
  EXPECT_EQ(5.0, objects.getDimension(dimension1Id).at(0).getValue(object1Id));
  EXPECT_EQ(10.0, objects.getDimension(dimension1Id).at(0).getValue(object2Id));
}

TEST(ObjectsTest, ToThrift) {
  const auto dimensionId = DimensionId(3);

  auto dim = std::make_shared<ObjectDimension>(ObjectIdToDoubleMap(
      /*totalSize=*/1,
      /*defaultValue=*/20.0,
      /*expectedNonDefaultSize=*/0));

  Map<DimensionId, std::shared_ptr<const ObjectDimension>> dimensions;
  dimensions.emplace(dimensionId, std::move(dim));

  const Objects objects(/*numObjects=*/1, std::move(dimensions));

  auto thrift = objects.toThrift();

  EXPECT_EQ(1, *thrift.numObjects());
  EXPECT_EQ(1, thrift.dimensions()->size());
  EXPECT_EQ(1, thrift.dimensions()->at(3).scalarDimensions()->size());
}

TEST(ObjectsTest, FromThrift) {
  thrift::ObjectStaticDimension objectStaticDimensionThrift;
  objectStaticDimensionThrift.defaultValue() = 20;

  thrift::ObjectScalarDimension objectScalarDimensionThrift;
  objectScalarDimensionThrift.objectStaticDimension() =
      objectStaticDimensionThrift;

  thrift::ObjectDimension objectDimensionThrift;
  objectDimensionThrift.scalarDimensions() = {objectScalarDimensionThrift};
  objectDimensionThrift.isDynamic() = false;

  thrift::Objects objectsThrift;
  objectsThrift.numObjects() = 1;
  objectsThrift.dimensions() = {{3, objectDimensionThrift}};

  const Objects objects(objectsThrift);

  const std::vector<ObjectId> expectedIds{ObjectId(0)};
  const std::vector<ObjectId> actualIds(
      objects.getObjectIds().begin(), objects.getObjectIds().end());
  EXPECT_EQ(expectedIds, actualIds);
  EXPECT_EQ(1, objects.getDimensionCount());
  EXPECT_EQ(
      std::set<DimensionId>({DimensionId(3)}),
      toSet<DimensionId>(objects.getDimensionIds()));
  EXPECT_EQ(1, objects.getDimension(DimensionId(3)).size());
  EXPECT_EQ(20, objects.getDimension(DimensionId(3)).at(0).getDefaultValue());
  EXPECT_FALSE(objects.hasNegativeDimensions());
}

TEST(ObjectsTest, ToEmbedding) {
  const auto object0 = ObjectId(0);
  const auto object1 = ObjectId(1);
  const auto dim0 = DimensionId(0);
  const auto dim1 = DimensionId(1);

  auto ramDim = std::make_shared<ObjectDimension>(ObjectIdToDoubleMap(
      Map<ObjectId, double>{{object0, 4.0}, {object1, 8.0}},
      /*defaultValue=*/0.0,
      /*totalSize=*/2));
  auto cpuDim = std::make_shared<ObjectDimension>(ObjectIdToDoubleMap(
      Map<ObjectId, double>{{object0, 0.5}, {object1, 1.0}},
      /*defaultValue=*/0.0,
      /*totalSize=*/2));

  Map<DimensionId, std::shared_ptr<const ObjectDimension>> dimensions;
  dimensions.emplace(dim0, std::move(ramDim));
  dimensions.emplace(dim1, std::move(cpuDim));

  const Objects objects(/*numObjects=*/2, std::move(dimensions));
  const std::vector<DimensionId> dimIds{dim0, dim1};

  const auto vertices = objects.toEmbedding(dimIds);

  // One-hot prefix for OBJECT is [1, 0, 0, 0] followed by per-dimension values.
  std::vector<double> objectOneHot(kNumVertexTypes, 0.0);
  objectOneHot[static_cast<std::size_t>(thrift::VertexType::OBJECT)] = 1.0;

  thrift::GraphVertex expectedVertex0;
  expectedVertex0.type() = thrift::VertexType::OBJECT;
  expectedVertex0.entityId() = 0;
  expectedVertex0.embedding() = objectOneHot;
  expectedVertex0.embedding()->insert(
      expectedVertex0.embedding()->end(), {4.0, 0.5});

  thrift::GraphVertex expectedVertex1;
  expectedVertex1.type() = thrift::VertexType::OBJECT;
  expectedVertex1.entityId() = 1;
  expectedVertex1.embedding() = objectOneHot;
  expectedVertex1.embedding()->insert(
      expectedVertex1.embedding()->end(), {8.0, 1.0});

  const std::vector<thrift::GraphVertex> expected{
      expectedVertex0, expectedVertex1};
  EXPECT_EQ(expected, vertices);
}

TEST(ObjectsTest, ToEmbeddingDynamicDimensionEmitsSentinel) {
  const auto object0 = ObjectId(0);
  const auto object1 = ObjectId(1);
  const auto dimensionId = DimensionId(0);
  const auto scopeId = ScopeId(10);
  const auto scopeItemId = ScopeItemId(11);

  Map<ScopeItemId, std::shared_ptr<const ObjectIdToDoubleMap>> values;
  values.emplace(
      scopeItemId,
      std::make_shared<const ObjectIdToDoubleMap>(
          Map<ObjectId, double>{{object0, 7.0}, {object1, 9.0}},
          /*defaultValue=*/42.0,
          /*totalSize=*/2));

  auto dynamicDim = std::make_shared<ObjectDimension>(
      scopeId,
      std::move(values),
      /*defaultValue=*/42.0,
      /*totalObjects=*/2);

  Map<DimensionId, std::shared_ptr<const ObjectDimension>> dimensions;
  dimensions.emplace(dimensionId, std::move(dynamicDim));

  const Objects objects(/*numObjects=*/2, std::move(dimensions));

  const auto vertices = objects.toEmbedding({dimensionId});

  std::vector<double> objectOneHot(kNumVertexTypes, 0.0);
  objectOneHot[static_cast<std::size_t>(thrift::VertexType::OBJECT)] = 1.0;

  thrift::GraphVertex expectedVertex0;
  expectedVertex0.type() = thrift::VertexType::OBJECT;
  expectedVertex0.entityId() = 0;
  expectedVertex0.embedding() = objectOneHot;
  expectedVertex0.embedding()->push_back(0.0);

  thrift::GraphVertex expectedVertex1;
  expectedVertex1.type() = thrift::VertexType::OBJECT;
  expectedVertex1.entityId() = 1;
  expectedVertex1.embedding() = objectOneHot;
  expectedVertex1.embedding()->push_back(0.0);

  const std::vector<thrift::GraphVertex> expected{
      expectedVertex0, expectedVertex1};
  EXPECT_EQ(expected, vertices);
}

TEST(ObjectsTest, ToEmbeddingRoutingConfigBasedDimensionEmitsSentinel) {
  const auto dimensionId = DimensionId(0);

  // ObjectPartitionRoutingDimension::getValue(ObjectId) throws, so toEmbedding
  // must not call it on routing-config-based dimensions; instead it should emit
  // a sentinel 0.0.
  auto routingDim = std::make_shared<ObjectDimension>(
      /*groupIdToValue=*/Map<GroupId, double>{{GroupId(8), 10.0}},
      /*defaultValue=*/20.0,
      /*partitionId=*/PartitionId(7),
      /*routingConfigId=*/RoutingConfigId(4),
      /*groupIdToStaticValue=*/Map<GroupId, double>{{GroupId(9), 5.0}},
      /*defaultStaticValue=*/2.0);

  Map<DimensionId, std::shared_ptr<const ObjectDimension>> dimensions;
  dimensions.emplace(dimensionId, std::move(routingDim));

  const Objects objects(/*numObjects=*/2, std::move(dimensions));

  std::vector<thrift::GraphVertex> vertices;
  EXPECT_NO_THROW({ vertices = objects.toEmbedding({dimensionId}); });

  std::vector<double> objectOneHot(kNumVertexTypes, 0.0);
  objectOneHot[static_cast<std::size_t>(thrift::VertexType::OBJECT)] = 1.0;

  thrift::GraphVertex expectedVertex0;
  expectedVertex0.type() = thrift::VertexType::OBJECT;
  expectedVertex0.entityId() = 0;
  expectedVertex0.embedding() = objectOneHot;
  expectedVertex0.embedding()->push_back(0.0);

  thrift::GraphVertex expectedVertex1;
  expectedVertex1.type() = thrift::VertexType::OBJECT;
  expectedVertex1.entityId() = 1;
  expectedVertex1.embedding() = objectOneHot;
  expectedVertex1.embedding()->push_back(0.0);

  const std::vector<thrift::GraphVertex> expected{
      expectedVertex0, expectedVertex1};
  EXPECT_EQ(expected, vertices);
}

TEST(ObjectsTest, HasNegativeValues) {
  {
    const auto dimensionId = DimensionId(0);
    auto dim = std::make_shared<ObjectDimension>(ObjectIdToDoubleMap(
        /*totalSize=*/0,
        /*defaultValue=*/-1.0,
        /*expectedNonDefaultSize=*/0));

    Map<DimensionId, std::shared_ptr<const ObjectDimension>> dimensions;
    dimensions.emplace(dimensionId, std::move(dim));

    const Objects objects(/*numObjects=*/0, std::move(dimensions));
    EXPECT_TRUE(objects.hasNegativeDimensions());
  }

  {
    const auto object1 = ObjectId(0);
    const auto object2 = ObjectId(1);
    const auto dimension1 = DimensionId(100);
    const auto dimension2 = DimensionId(101);

    auto dim1 = std::make_shared<ObjectDimension>(makeObjectIdToDoubleMap(
        Map<ObjectId, double>{{object1, 10.0}, {object2, 0.01}},
        /*defaultValue=*/10.0,
        /*totalSize=*/2));
    auto dim2 = std::make_shared<ObjectDimension>(makeObjectIdToDoubleMap(
        Map<ObjectId, double>{{object1, 1.0}, {object2, -0.01}},
        /*defaultValue=*/20.0,
        /*totalSize=*/2));

    Map<DimensionId, std::shared_ptr<const ObjectDimension>> dimensions;
    dimensions.emplace(dimension1, std::move(dim1));
    dimensions.emplace(dimension2, std::move(dim2));

    const Objects objects(/*numObjects=*/2, std::move(dimensions));

    EXPECT_EQ(2, objects.getDimensionCount());
    EXPECT_TRUE(objects.hasNegativeDimensions());
  }
}

} // namespace facebook::rebalancer::entities::tests
