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

#pragma once

#include "algopt/rebalancer/entities/thrift/gen-cpp2/GraphState_types.h"

#include <fmt/core.h>

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace facebook::rebalancer::entities {

constexpr std::size_t kNumVertexTypes =
    apache::thrift::TEnumTraits<thrift::VertexType>::size;

inline std::size_t indexOfVertexType(thrift::VertexType type) {
  const auto& values = apache::thrift::TEnumTraits<thrift::VertexType>::values;
  const auto it = std::find(values.begin(), values.end(), type);
  if (it == values.end()) {
    throw std::invalid_argument(
        fmt::format("Unknown VertexType value: {}", static_cast<int>(type)));
  }
  return static_cast<std::size_t>(std::distance(values.begin(), it));
}

inline void initOneHotEmbedding(
    std::vector<double>& embedding,
    thrift::VertexType type) {
  embedding.assign(kNumVertexTypes, 0.0);
  embedding[indexOfVertexType(type)] = 1.0;
}

} // namespace facebook::rebalancer::entities
