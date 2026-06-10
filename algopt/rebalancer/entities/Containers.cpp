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

#include "algopt/rebalancer/entities/Containers.h"

#include "algopt/rebalancer/entities/GraphEmbeddingUtils.h"

namespace facebook {
namespace rebalancer {
namespace entities {

Containers::Containers(
    Map<ContainerId, std::vector<ObjectId>> initialAssignment)
    : initialAssignment_(std::move(initialAssignment)) {}

Containers::Containers(const thrift::Containers& containers) {
  initialAssignment_.reserve(containers.initialAssignment()->size());
  for (const auto& [containerId, objects] : *containers.initialAssignment()) {
    std::vector<ObjectId> objectIds;
    objectIds.reserve(objects.size());
    for (auto objectId : objects) {
      objectIds.emplace_back(objectId);
    }
    initialAssignment_.emplace(ContainerId(containerId), std::move(objectIds));
  }
}

const std::vector<ObjectId>& Containers::getInitialObjectIds(
    ContainerId containerId) const {
  return initialAssignment_.at(containerId);
}

thrift::Containers Containers::toThrift() const {
  thrift::Containers containers;
  auto& thriftInitialAssignment = *containers.initialAssignment();
  thriftInitialAssignment.reserve(initialAssignment_.size());
  for (const auto& [containerId, objectIds] : initialAssignment_) {
    thriftInitialAssignment.emplace(containerId.asInt(), asIntsVec(objectIds));
  }
  return containers;
}

std::vector<thrift::GraphVertex> Containers::toEmbedding() const {
  std::vector<thrift::GraphVertex> vertices;
  vertices.reserve(initialAssignment_.size());

  for (const auto containerId : getContainerIds()) {
    thrift::GraphVertex vertex;
    vertex.type() = thrift::VertexType::CONTAINER;
    vertex.entityId() = containerId.asInt();

    auto& embedding = *vertex.embedding();
    initOneHotEmbedding(embedding, thrift::VertexType::CONTAINER);

    vertices.push_back(std::move(vertex));
  }
  return vertices;
}

} // namespace entities
} // namespace rebalancer
} // namespace facebook
