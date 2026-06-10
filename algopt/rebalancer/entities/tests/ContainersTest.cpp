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
#include "algopt/rebalancer/entities/Containers.h"
#include "algopt/rebalancer/entities/GraphEmbeddingUtils.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/tests/Utils.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace facebook::rebalancer::entities::tests {

TEST(ContainersTest, ToThrift) {
  const Containers containers({{ContainerId(0), {ObjectId(1), ObjectId(2)}}});

  auto thrift = containers.toThrift();

  const std::map<int, std::vector<int>> expectedContainerToParts({{0, {1, 2}}});
  EXPECT_EQ(expectedContainerToParts, toStdMap(*thrift.initialAssignment()));
}

TEST(ContainersTest, FromThrift) {
  thrift::Containers thrift;
  thrift.initialAssignment() = {{0, {1, 2}}};

  const Containers containers(thrift);

  const auto idsView = containers.getContainerIds();
  const auto idVec = std::vector<ContainerId>(idsView.begin(), idsView.end());

  EXPECT_EQ(std::vector<ContainerId>({ContainerId(0)}), idVec);
  EXPECT_EQ(
      std::set<ObjectId>({ObjectId(1), ObjectId(2)}),
      toSet<ObjectId>(containers.getInitialObjectIds(ContainerId(0))));
}

TEST(ContainersTest, ToEmbedding) {
  const Containers containers(
      {{ContainerId(0), {ObjectId(1)}}, {ContainerId(1), {ObjectId(2)}}});

  const auto vertices = containers.toEmbedding();
  auto sortedVertices = vertices;
  std::sort(
      sortedVertices.begin(),
      sortedVertices.end(),
      [](const thrift::GraphVertex& a, const thrift::GraphVertex& b) {
        return *a.entityId() < *b.entityId();
      });

  std::vector<double> containerOneHot(kNumVertexTypes, 0.0);
  containerOneHot[static_cast<std::size_t>(thrift::VertexType::CONTAINER)] =
      1.0;

  thrift::GraphVertex expectedVertex0;
  expectedVertex0.type() = thrift::VertexType::CONTAINER;
  expectedVertex0.entityId() = 0;
  expectedVertex0.embedding() = containerOneHot;

  thrift::GraphVertex expectedVertex1;
  expectedVertex1.type() = thrift::VertexType::CONTAINER;
  expectedVertex1.entityId() = 1;
  expectedVertex1.embedding() = containerOneHot;

  const std::vector<thrift::GraphVertex> expected{
      expectedVertex0, expectedVertex1};
  EXPECT_EQ(expected, sortedVertices);
}

} // namespace facebook::rebalancer::entities::tests
