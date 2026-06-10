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

package "meta.com/algopt/rebalancer/entities"

namespace cpp2 facebook.rebalancer.entities.thrift
namespace py3 rebalancer.entities.thrift

enum VertexType {
  OBJECT = 0,
  CONTAINER = 1,
  SCOPE_ITEM = 2,
  GROUP = 3,
}

struct GraphVertex {
  1: VertexType type;
  2: list<double> embedding;
  3: i64 entityId;
}

struct GraphEdge {
  1: i64 sourceIndex;
  2: i64 targetIndex;
}

struct GraphState {
  1: list<GraphVertex> vertices;
  2: list<GraphEdge> edges;
  3: i32 embeddingDimension;
  4: list<string> dimensionNames;
}
