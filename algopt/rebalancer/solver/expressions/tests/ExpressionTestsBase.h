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

#include "algopt/rebalancer/entities/tests/UniverseBuilderTestUtils.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::packer::tests {
namespace entities = facebook::rebalancer::entities;

constexpr double kEps = 1e-5;

template <typename T = entities::ObjectId>
std::shared_ptr<const entities::Map<T, double>> makeSharedPtrEntityToValueMap(
    entities::Map<T, double>&& map) {
  return std::make_shared<const entities::Map<T, double>>(std::move(map));
}

class ExpressionTestsBase : public ::testing::Test,
                            public entities::tests::UniverseBuilderTestUtils {
 public:
  using ObjectToNewContainer =
      std::vector<std::pair<entities::ObjectId, entities::ContainerId>>;

  ExpressionTestsBase(
      const std::string& objectName = "object",
      const std::string& containerName = "container")
      : UniverseBuilderTestUtils(objectName, containerName) {}

  double apply(
      const ExprPtr& expr,
      const Assignment& assignment,
      const std::optional<LpAssertOptions>& = std::nullopt) const;

  double evaluate(
      const ExprPtr& expr,
      const ObjectToNewContainer& changes,
      const Assignment& currAssignment,
      const std::optional<LpAssertOptions>& = std::nullopt) const;

  static double evaluate(Expression& expression, const ChangeSet& changes);

  double applyChanges(
      const ExprPtr& expr,
      const ObjectToNewContainer& moves,
      const Assignment& currAssignment,
      const std::optional<LpAssertOptions>& = std::nullopt) const;

  static ChangeSet getChangeSet(
      const ObjectToNewContainer& moves,
      const Assignment& currAssignment);

  static Assignment getModifiedAssignment(
      const Assignment& assignment,
      const ObjectToNewContainer& changes);
};

} // namespace facebook::rebalancer::packer::tests
