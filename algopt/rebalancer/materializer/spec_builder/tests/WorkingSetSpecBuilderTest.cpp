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
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/materializer/spec_builder/WorkingSetSpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace facebook::rebalancer::materializer::tests {

class WorkingSetSpecBuilderTest : public SpecBuilderTestBase<> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1"}},
        {"host1", {}},
    });

    const entities::Map<entities::ObjectId, double> cpuValues = {
        {objectId("task0"), 10}, {objectId("task1"), 100}};
    co_await addObjectDimension("cpu", cpuValues, 0);

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

TEST_F(WorkingSetSpecBuilderTest, Description) {
  interface::WorkingSetSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";

  const WorkingSetSpecBuilder specBuilder(buildUniverse(), spec);
  EXPECT_EQ(
      "Minimize avg working set size of hosts weighted by cpu",
      specBuilder.description());
}

TEST_F(WorkingSetSpecBuilderTest, SpecInfo) {
  interface::WorkingSetSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";

  const WorkingSetSpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "",
      .scope = "host",
      .dimension = "cpu",
      .definition = "AVG",
      .size = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}

TEST_F(WorkingSetSpecBuilderTest, Constraint) {
  const interface::WorkingSetSpec spec;
  WorkingSetSpecBuilder specBuilder(buildUniverse(), spec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      folly::coro::blockingWait(specBuilder.constraints(expressionBuilder())),
      "WorkingSetSpec not supported as a constraint");
}

CO_TEST_F(WorkingSetSpecBuilderTest, Goal) {
  interface::WorkingSetSpec spec;

  spec.scope() = "host";
  spec.dimension() = "cpu";

  {
    interface::WorkingUnit unit;
    unit.weight() = 1;
    unit.endpoints() = {"task0", "task1"};
    spec.workingUnits()->push_back(unit);
  }

  {
    interface::WorkingUnit unit;
    unit.weight() = 2;
    unit.endpoints() = {"task0"};
    spec.workingUnits()->push_back(unit);
  }

  {
    interface::WorkingUnit unit;
    unit.weight() = 4;
    unit.endpoints() = {"task1"};
    spec.workingUnits()->push_back(unit);
  }

  const WorkingSetSpecBuilder specBuilder(buildUniverse(), spec);

  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // disable lp expression evaluation since the spec used non-linear terms
  packer::tests::LpAssertOptions assertOptions = {
      .exceptionForLpExpr =
          "At least one of the operands must be a binary variable"};

  EXPECT_NEAR(770, evaluate(goal, deltaFromInitial({}), assertOptions), 1e-8);
  EXPECT_NEAR(
      530,
      evaluate(goal, deltaFromInitial({{"task0", "host1"}}), assertOptions),
      1e-8);
  EXPECT_NEAR(
      530,
      evaluate(goal, deltaFromInitial({{"task1", "host1"}}), assertOptions),
      1e-8);
  EXPECT_NEAR(
      770,
      evaluate(
          goal,
          deltaFromInitial({{"task0", "host1"}, {"task1", "host1"}}),
          assertOptions),
      1e-8);
}
} // namespace facebook::rebalancer::materializer::tests
