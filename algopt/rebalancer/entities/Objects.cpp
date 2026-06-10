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

#include "algopt/rebalancer/entities/Objects.h"

#include "algopt/rebalancer/entities/GraphEmbeddingUtils.h"
#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Partitions.h"

#include <folly/container/MapUtil.h>
#include <folly/Conv.h>

namespace facebook::rebalancer::entities {

namespace {
std::shared_ptr<const ObjectDimension> makeDefaultDimension(
    std::size_t totalObjects) {
  return std::make_shared<const ObjectDimension>(ObjectIdToDoubleMap(
      totalObjects,
      /*defaultValue=*/0.0,
      /*expectedNonDefaultSize=*/0));
}
} // namespace

Objects::Objects(
    EntityIdType numObjects,
    Map<DimensionId, std::shared_ptr<const ObjectDimension>> dimensions)
    : numObjects_(numObjects),
      dimensions_(std::move(dimensions)),
      defaultDimension_(makeDefaultDimension(numObjects_)) {}

Objects::Objects(const thrift::Objects& objects, const Partitions* partitions)
    : numObjects_(static_cast<EntityIdType>(*objects.numObjects())),
      defaultDimension_(makeDefaultDimension(numObjects_)) {
  dimensions_.reserve(objects.dimensions()->size());
  for (const auto& [dimensionId, dimensionThrift] : *objects.dimensions()) {
    dimensions_.emplace(
        DimensionId(dimensionId),
        std::make_shared<ObjectDimension>(
            dimensionThrift, numObjects_, partitions));
  }
}

const ObjectDimension& Objects::getDimension(DimensionId dimensionId) const {
  return *folly::get_ref_default(dimensions_, dimensionId, defaultDimension_);
}

thrift::Objects Objects::toThrift() const {
  thrift::Objects objects;
  // TODO: remove objectIds field
  objects.objectIds() = asIntsVec(getObjectIds());

  objects.numObjects() = folly::to<int32_t>(numObjects_);
  auto& thriftDimensions = *objects.dimensions();
  thriftDimensions.reserve(dimensions_.size());
  for (const auto& [dimensionId, dimension] : dimensions_) {
    thriftDimensions.emplace(dimensionId.asInt(), dimension->toThrift());
  }
  return objects;
}

bool Objects::hasNegativeDimensions() const {
  return std::any_of(
      dimensions_.begin(),
      dimensions_.end(),
      [](const auto& dimensionIdDimensionPair) {
        return dimensionIdDimensionPair.second->hasNegativeValues();
      });
}

size_t Objects::getDimensionCount() const {
  return dimensions_.size();
}

std::vector<thrift::GraphVertex> Objects::toEmbedding(
    const std::vector<DimensionId>& dimIds) const {
  // Precompute per-dimension descriptors so we do map lookups, only(),
  // isDynamic(), and isRoutingConfigBased() once per dimension (O(D))
  // rather than once per (object, dimension) pair (O(N*D)).
  struct DimDescriptor {
    const ObjectScalarDimension* scalarDim;
    bool useSentinel;
  };
  std::vector<DimDescriptor> dimDescriptors;
  dimDescriptors.reserve(dimIds.size());
  for (const auto dimId : dimIds) {
    const auto& scalarDim = getDimension(dimId).only();
    dimDescriptors.push_back(
        {&scalarDim,
         scalarDim.isDynamic() || scalarDim.isRoutingConfigBased()});
  }

  std::vector<thrift::GraphVertex> vertices;
  vertices.reserve(numObjects_);

  for (const auto objectId : getObjectIds()) {
    thrift::GraphVertex vertex;
    vertex.type() = thrift::VertexType::OBJECT;
    vertex.entityId() = objectId.asInt();

    auto& embedding = *vertex.embedding();
    embedding.reserve(kNumVertexTypes + dimIds.size());
    initOneHotEmbedding(embedding, thrift::VertexType::OBJECT);

    for (const auto& desc : dimDescriptors) {
      embedding.push_back(
          desc.useSentinel ? 0.0 : desc.scalarDim->getValue(objectId));
    }

    vertices.push_back(std::move(vertex));
  }
  return vertices;
}

} // namespace facebook::rebalancer::entities
