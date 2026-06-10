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

#include "algopt/rebalancer/entities/Identifiers.h"
#include "algopt/rebalancer/entities/Map.h"
#include "algopt/rebalancer/entities/tests/UniverseBuilderTestUtils.h"
#include "algopt/rebalancer/materializer/spec_builder/SpecBuilder.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include "algopt/rebalancer/solver/expressions/tests/ExpressionUtils.h"
#include "algopt/rebalancer/solver/expressions/TopToBottomEvaluator.h"
#include "algopt/rebalancer/solver/moves/InvalidMoveFilter.h"
#include "algopt/rebalancer/solver/tests/ExprProblemCreation.h"
#include "algopt/rebalancer/solver/utils/Assignment.h"
#include "algopt/rebalancer/solver/utils/Context.h"

#include <fmt/core.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/coro/Task.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {
namespace entities = facebook::rebalancer::entities;

template <typename T = entities::ObjectId>
std::shared_ptr<const entities::Map<T, double>> makeSharedPtrEntityToValueMap(
    const entities::Map<T, double>& map) {
  return std::make_shared<const entities::Map<T, double>>(map);
}

template <typename T = bool>
class SpecBuilderTestBase : public ::testing::TestWithParam<T>,
                            public entities::tests::UniverseBuilderTestUtils {
 protected:
  using InvalidPair = std::pair<entities::ObjectId, entities::ContainerId>;

  SpecBuilderTestBase()
      : entities::tests::UniverseBuilderTestUtils("task", "host") {}

  std::set<InvalidPair> collectInvalidPairs(
      const InvalidMoveFilter& filter) const {
    std::set<InvalidPair> invalidPairs;
    for (const auto objId : universe_->getObjects().getObjectIds()) {
      for (const auto& cId : universe_->getContainers().getContainerIds()) {
        if (filter.isMarkedInvalid(objId, cId)) {
          invalidPairs.emplace(objId, cId);
        }
      }
    }
    return invalidPairs;
  }

  void setUpUniverse(
      const entities::Map<std::string, std::vector<std::string>>&
          initialAssignment,
      const std::string& objectName = "task",
      const std::string& containerName = "host") {
    universeBuilder_.setObjectTypeName(objectName);
    universeBuilder_.setContainerTypeName(containerName);
    setInitialAssignment(initialAssignment);
    initialAssignment_ = initialAssignment;
  }

  ExpressionBuilder& expressionBuilder() {
    if (!universe_) {
      buildUniverse();
    }

    if (expressionBuilder_) {
      return *expressionBuilder_;
    }

    expressionBuilder_ =
        std::make_shared<ExpressionBuilder>(universe_, deltaFromInitial({}));
    return *expressionBuilder_;
  }

  // Creates a full assignment given a delta from the initial assignment.
  entities::Map<entities::ContainerId, std::vector<entities::ObjectId>>
  deltaFromInitial(
      const entities::Map<std::string, std::string>& objectToContainerDelta)
      const {
    entities::Map<entities::ContainerId, std::vector<entities::ObjectId>>
        assignment;
    for (const auto& [containerName, objectNames] : initialAssignment_) {
      const auto contId = containerId(containerName);
      if (!assignment.contains(contId)) {
        assignment[contId] = {};
      }

      for (const auto& objectName : objectNames) {
        auto dstContainerName = folly::get_default(
            objectToContainerDelta, objectName, containerName);
        assignment[containerId(dstContainerName)].push_back(
            objectId(objectName));
      }
    }

    return assignment;
  }

  // Evaluates an expression for a given assignment.
  double evaluate(
      ExprPtr expr,
      const entities::Map<
          entities::ContainerId,
          std::vector<entities::ObjectId>>& containerToObjects,
      const std::optional<packer::tests::LpAssertOptions>& lpAssertOptions =
          std::nullopt) const {
    const Assignment assignment(containerToObjects);
    Context context;
    auto fullyApplyVal =
        expr->fullApply(TopToBottomEvaluator(context), assignment);

    if (!universe_) {
      throw std::runtime_error(
          "expected to call evaluate only after buildUniverse() is called");
    }
    packer::tests::verifyLpExpression(
        expr, containerToObjects, universe_, fullyApplyVal, lpAssertOptions);

    return fullyApplyVal;
  }

  // Evaluates the constraint expression by default; if evaluateConstraintExpr
  // is false, then it will evaluate the additionalPenaltyExpr
  double evaluate(
      materializer::ConstraintInfo constraintInfo,
      const entities::Map<
          entities::ContainerId,
          std::vector<entities::ObjectId>>& containerToObjects,
      bool evaluateConstraintExpr = true,
      std::optional<packer::tests::LpAssertOptions> lpAssertOptions =
          std::nullopt) const {
    auto& expr = evaluateConstraintExpr ? constraintInfo.constraintExpr
                                        : constraintInfo.additionalPenaltyExpr;

    assertConstraintViolationBounds({constraintInfo});

    return evaluate(expr, containerToObjects, lpAssertOptions);
  }

  void assertConstraintViolationBounds(
      const std::vector<materializer::ConstraintInfo>& constraintComponents)
      const {
    for (const auto& component : constraintComponents) {
      Context context;
      const auto violationExpr = SpecBuilder::getConstraintViolation(component);
      violationExpr->init_unconstrained_bounds(context);
      const auto [lb, _] = violationExpr->getUnconstrainedBounds();

      // if there is no additional penalty expression, then the lower bound
      // is expected to be at least 0; else it is expected to be at least
      // the lower bound of the additional penalty expression
      if (const auto& penalty = component.additionalPenaltyExpr) {
        penalty->init_unconstrained_bounds(context);
        const auto [expectedLb, _1] = penalty->getUnconstrainedBounds();
        EXPECT_GE(lb, expectedLb) << fmt::format(
            "expected lowerBound {} of constraint violation expression to be at least the lower bound of the additional penalty expression {}",
            lb,
            expectedLb);
      } else {
        EXPECT_GE(lb, 0)
            << "expected constraint violation expression lower bound to be >= 0";
      }
    }
  }

  ExprPtr any_positive(
      const std::vector<materializer::ConstraintInfo>& constraints) const {
    std::vector<ExprPtr> exprs;
    exprs.reserve(constraints.size());
    for (auto& constraintInfo : constraints) {
      exprs.push_back(constraintInfo.constraintExpr);
    }

    if (!universe_) {
      throw std::runtime_error(
          "expected to call any_positive only after buildUniverse() is called");
    }
    return facebook::rebalancer::any_positive(exprs, universe_);
  }

  [[nodiscard]] std::string digest(
      ExprPtr expression,
      const entities::Map<std::string, std::string>& objectToContainerDelta,
      int maxChildren = DEFAULT_DEBUG_LENGTH) const {
    if (!universe_) {
      throw std::runtime_error(
          "expected to call digest only after buildUniverse() is called");
    }

    auto problem = packer::tests::createTestProblem(
        universe_, {expression}, expression, {}, {}, false);
    problem->assignment = Assignment(deltaFromInitial(objectToContainerDelta));
    return expression->digest(*problem, false, maxChildren);
  }

 private:
  entities::Map<std::string, std::vector<std::string>> initialAssignment_;
  std::shared_ptr<ExpressionBuilder> expressionBuilder_{nullptr};
};

} // namespace facebook::rebalancer::materializer::tests
