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

#include "algopt/rebalancer/interface/serialization/Serializer.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/AssignmentProblem_types.h"

#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;

TEST(SerializerTest, SerializeAndDeserialize) {
  const AssignmentProblem problem;
  auto serialized = Serializer::serialize(problem);
  auto deserialized = Serializer::deserialize<AssignmentProblem>(serialized);
  ASSERT_EQ(problem, deserialized);
}
