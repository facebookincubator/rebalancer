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

#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/materializer/spec_builder/SRBufferCapacitySpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class SRBufferCapacitySpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse(
        {{"host0", {"task0", "task1"}},
         {"host1", {"task2", "task3", "task4", "task5"}}});

    co_await addPartition(
        "job",
        {{"job1", {"task0", "task3", "task2"}},
         {"job2", {"task1", "task4", "task5"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

static interface::SRBufferCapacitySpec createSRBufferSpec() {
  std::map<std::string, double> lowerBoundMatchingErrors = {};

  std::vector<interface::ScopeItemPair> pairs;
  auto pair = interface::ScopeItemPair();
  pair.scopeItem1() = "host0";
  pair.scopeItem2() = "host1";
  pairs.push_back(pair);

  interface::SRBufferCapacitySpec spec;
  spec.dimension() = "task_count";
  spec.filter() = interface::Filter();
  spec.lowerBoundMatchingErrors() = lowerBoundMatchingErrors;
  spec.matchingError() = 1.0;
  spec.name() = "name";
  spec.partitionName() = "job";
  spec.scope() = "host";
  spec.scopeItemPairs() = pairs;
  spec.useHeuristics() = false;

  return spec;
}

CO_TEST_F(SRBufferCapacitySpecBuilderTest, Constraint) {
  const SRBufferCapacitySpecBuilder specBuilder(
      buildUniverse(), createSRBufferSpec());

  EXPECT_EQ("SR Buffer capacity for host", specBuilder.description());

  auto& exprBuilder = expressionBuilder();
  ExprPtr result;
  for (auto useAsObjective : {true, false}) {
    if (useAsObjective) {
      result = co_await specBuilder.goalCoro(exprBuilder);
    } else {
      auto constraints = co_await specBuilder.constraints(exprBuilder);
      result = any_positive(constraints);
    }
  }

  // buffer requirement is 2, buffer allocated is 4 hence constraint is
  // satisfied
  EXPECT_NEAR(0, evaluate(result, deltaFromInitial({})), 1e-8);

  // buffer requirement is 3, buffer allocated is 2 but matchingError is
  // set to 1 hence constraint still satisfied
  EXPECT_NEAR(
      0,
      evaluate(
          result, deltaFromInitial({{"task2", "host0"}, {"task3", "host0"}})),
      1e-8);

  // buffer requirement is 3, buffer allocated is 1 hence constriant is
  // broken
  EXPECT_NEAR(
      1,
      evaluate(
          result,
          deltaFromInitial(
              {{"task2", "host0"}, {"task3", "host0"}, {"task4", "host0"}})),
      1e-8);

  auto expectedSpecInfo = SpecParameters{
      .name = "name",
      .scope = "host",
      .partition = "job",
      .dimension = "task_count",
      .filterAllowListSize = 0,
      .filterBlockListSize = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

} // namespace facebook::rebalancer::materializer::tests
