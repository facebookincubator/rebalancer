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
#include "algopt/rebalancer/materializer/spec_builder/GroupCapacitySpecBuilder.h"
#include "algopt/rebalancer/materializer/utils/tests/SpecBuilderTestBase.h"
#include "algopt/rebalancer/solver/expressions/Expression.h"
#include "algopt/rebalancer/solver/expressions/Operators.h"
#include <algopt/rebalancer/interface/tests/utils.h>
#include <algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h>

#include <folly/coro/GtestHelpers.h>
#include <gtest/gtest.h>

namespace interface = facebook::rebalancer::interface;

namespace facebook::rebalancer::materializer::tests {

class GroupCapacitySpecBuilderTest
    : public SpecBuilderTestBase<interface::GroupCapacitySpecUtilType> {
 protected:
  folly::coro::Task<void> setUpCoro() {
    setUpUniverse({
        {"host0", {"task0", "task1", "task2"}},
        {"host1", {"task3", "task4", "task5"}},
    });

    co_await addPartition(
        "job",
        {{"job0", {"task0", "task1"}},
         {"job1", {"task2", "task3"}},
         {"job2", {"task4", "task5"}}});

    co_return;
  }

  void SetUp() override {
    folly::coro::blockingWait(setUpCoro());
  }
};

INSTANTIATE_TEST_CASE_P(
    LinearUtil,
    GroupCapacitySpecBuilderTest,
    ::testing::Values(interface::GroupCapacitySpecUtilType::LINEAR));

INSTANTIATE_TEST_CASE_P(
    StepUtil,
    GroupCapacitySpecBuilderTest,
    ::testing::Values(interface::GroupCapacitySpecUtilType::STEP));

CO_TEST_P(GroupCapacitySpecBuilderTest, goal) {
  interface::GroupCapacitySpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.contributionPartition() = "job";
  spec.contribution()->globalLimit() = 10;
  spec.limit()->groupLimits() = {{"job0", 5}, {"job1", 9}, {"job2", 20}};
  spec.definition() = interface::GroupCapacitySpecDefinition::AFTER;
  spec.utilType() = GetParam();

  const GroupCapacitySpecBuilder specBuilder(buildUniverse(), spec);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  switch (GetParam()) {
    case interface::GroupCapacitySpecUtilType::LINEAR: {
      // goalValue = max(0, 10*2 -5) + max(0, 10*2 - 9) + max(0, 10*2 - 20) = 15
      // + 11 = 26
      EXPECT_NEAR(26, evaluate(goal, deltaFromInitial({})), 1e-8);
      break;
    }
    case interface::GroupCapacitySpecUtilType::STEP: {
      // goalValue = max(0, 10*1 -5) + max(0, 10*1 + 10*1 - 9) + max(0, 10*1 -
      // 20) = 5 + 11 + 0 = 16
      EXPECT_NEAR(16, evaluate(goal, deltaFromInitial({})), 1e-8);
      break;
    }
    case interface::GroupCapacitySpecUtilType::STEP_MOD_K: {
      XLOG(INFO)
          << "TODO: Requires additional work / bug fixes in MIP model generation";
      break;
    }
  }
}

CO_TEST_P(GroupCapacitySpecBuilderTest, constraintMaxAfter) {
  interface::GroupCapacitySpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.contributionPartition() = "job";
  spec.contribution()->globalLimit() = 0;
  // cost of job0 on host1 = 0, job2 on host0 = 0
  spec.contribution()->scopeItemToGroupLimits() = {
      {"host0", {{"job1", 2}, {"job0", 1}}},
      {"host1", {{"job1", 2}, {"job2", 1}}}};
  spec.limit()->groupLimits() = {{"job0", 0}, {"job1", 4}, {"job2", 0}};
  spec.bound() = interface::GroupCapacitySpecBound::MAX;
  spec.definition() = interface::GroupCapacitySpecDefinition::AFTER;
  spec.utilType() = GetParam();

  const auto universe = buildUniverse();
  const GroupCapacitySpecBuilder specBuilder(universe, spec);
  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  // one constraint per group
  EXPECT_EQ(3, constraints.size());
  auto sumOfConstraints = const_expr(0, universe);
  for (const auto& expr : constraints) {
    sumOfConstraints += expr.constraintExpr;
  }

  switch (GetParam()) {
    case interface::GroupCapacitySpecUtilType::LINEAR: {
      // job0 : 2 (util host0) + 0 (util host1)  - 0 (limit)
      // job1 : 2 (util host0) + 2 (util host1)  - 4 (limit)
      // job2 : 0 (util host0) + 2 (util host1)  - 0 (limit)
      EXPECT_EQ(4, evaluate(sumOfConstraints, deltaFromInitial({})));

      // swapping hosts for jobs job0 and job2 gets rid of the excess
      // utilization
      EXPECT_EQ(
          0,
          evaluate(
              sumOfConstraints,
              deltaFromInitial(
                  {{"task0", "host1"},
                   {"task1", "host1"},
                   {"task4", "host0"},
                   {"task5", "host0"}})));

      break;
    }
    case interface::GroupCapacitySpecUtilType::STEP: {
      // job0 : 1*1 (util host0) + 0*0 (util host1)  - 0 (limit) = 1
      // job1 : 2*1 (util host0) + 2*1 (util host1)  - 4 (limit) = 0
      // job2 : 0*0 (util host0) + 1*1 (util host1)  - 0 (limit) = 1
      EXPECT_EQ(2, evaluate(sumOfConstraints, deltaFromInitial({})));

      // swapping hosts for jobs job0 and job2
      // job0 : 1*0 (util host0) + 0*1 (util host1)  - 0 (limit) = 0
      // job1 : 2*1 (util host0) + 2*1 (util host1)  - 4 (limit) = 0
      // job2 : 0*1 (util host0) + 1*0 (util host1)  - 0 (limit) = 0
      EXPECT_EQ(
          0,
          evaluate(
              sumOfConstraints,
              deltaFromInitial(
                  {{"task0", "host1"},
                   {"task1", "host1"},
                   {"task4", "host0"},
                   {"task5", "host0"}})));
      break;
    }
    case interface::GroupCapacitySpecUtilType::STEP_MOD_K: {
      XLOG(INFO)
          << "TODO: Requires additional work / bug fixes in MIP model generation";
    }
  }
}

CO_TEST_P(GroupCapacitySpecBuilderTest, constraintMinDuring) {
  interface::GroupCapacitySpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.contributionPartition() = "job";
  spec.contribution()->globalLimit() = 0.5;
  // cost of job0 on host0 = 1, job2 on host1 = 1
  spec.contribution()->scopeItemToGroupLimits() = {
      {"host0", {{"job1", 2}, {"job2", 1}}},
      {"host1", {{"job1", 2}, {"job0", 1}}}};
  spec.limit()->groupLimits() = {{"job0", 2}, {"job1", 4}, {"job2", 2}};
  spec.bound() = interface::GroupCapacitySpecBound::MIN;
  spec.definition() = interface::GroupCapacitySpecDefinition::DURING;
  spec.utilType() = GetParam();

  const auto universe = buildUniverse();
  const GroupCapacitySpecBuilder specBuilder(universe, spec);
  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  // one constraint per group
  EXPECT_EQ(3, constraints.size());
  auto sumOfConstraints = const_expr(0, universe);
  for (const auto& expr : constraints) {
    sumOfConstraints += expr.constraintExpr;
  }

  switch (GetParam()) {
    case interface::GroupCapacitySpecUtilType::LINEAR: {
      // job0 : 2 (limit) - 1 (util host0) - 0 (util host1)
      // job1 : 4 (limit) - 2 (util host0) - 2 (util host1)
      // job2 : 2 (limit) - 0 (util host0) - 1 (util host1)
      // Initially job0 = {task0, task1} are on host0, so their util is
      // 0.5 * 2 = 1 similarly job2 = {task4, task5} are on host1, so
      // their util is 0.5
      // * 2 = 1 these two groups are below the MIN capacity required
      EXPECT_EQ(2, evaluate(sumOfConstraints, deltaFromInitial({})));

      // swapping hosts for jobs job0 and job2 results in their
      // utilization to be 2 with during, their initial utilizations
      // will be included as well total  (2 - 1 - 2) + (4 - 2 - 2) + (2
      // - 2 - 1) = -2
      EXPECT_EQ(
          -2,
          evaluate(
              sumOfConstraints,
              deltaFromInitial(
                  {{"task0", "host1"},
                   {"task1", "host1"},
                   {"task4", "host0"},
                   {"task5", "host0"}})));
      break;
    }
    case interface::GroupCapacitySpecUtilType::STEP: {
      // job0 : 2 (limit) - 0.5 * 1 (util host0) - 1*0 (util host1)
      // = 1.5 job1 : 4 (limit) - 2 * 1 (util host0) - 2 * 1 (util
      // host1) = 0.0 job2 : 2 (limit) - 2* 0 (util host0) - 0.5 * 1
      // (util host1) = 1.5
      EXPECT_EQ(3.0, evaluate(sumOfConstraints, deltaFromInitial({})));

      // swapping hosts for jobs job0 and job2 results in their
      // utilization to be 2 with during, their initial utilizations
      // will be included as well total  (2 - 0 - 1*1) + (4 - 2*1 - 2*1)
      // + (2 - 2*1 - 0) = 1 + 0 + 0 =
      // 2
      EXPECT_EQ(
          1.0,
          evaluate(
              sumOfConstraints,
              deltaFromInitial(
                  {{"task0", "host1"},
                   {"task1", "host1"},
                   {"task4", "host0"},
                   {"task5", "host0"}})));
      break;
    }

    case interface::GroupCapacitySpecUtilType::STEP_MOD_K: {
      XLOG(INFO)
          << "TODO: Requires additional work / bug fixes in MIP model generation";
      break;
    }
  }
}

CO_TEST_P(GroupCapacitySpecBuilderTest, constraintExactDuringAndAfter) {
  interface::GroupCapacitySpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.contributionPartition() = "job";
  spec.contribution()->globalLimit() = 0.5;
  // cost of job0 on host0 = 1, job2 on host1 = 1
  spec.contribution()->scopeItemToGroupLimits() = {
      {"host0", {{"job1", 2}, {"job2", 1}}},
      {"host1", {{"job1", 2}, {"job0", 1}}}};
  spec.limit()->groupLimits() = {{"job0", 2}, {"job1", 4}, {"job2", 2}};
  spec.bound() = interface::GroupCapacitySpecBound::EXACT;
  spec.definition() = interface::GroupCapacitySpecDefinition::DURING_AND_AFTER;
  spec.utilType() = GetParam();

  const auto universe = buildUniverse();
  const GroupCapacitySpecBuilder specBuilder(universe, spec);
  auto constraints = co_await specBuilder.constraints(expressionBuilder());
  // one constraint per group * 2 (EXACT = MAX + MIN) * 2 (DURING + AFTER)
  EXPECT_EQ(12, constraints.size());
  auto sumOfConstraints = const_expr(0, universe);
  for (const auto& expr : constraints) {
    sumOfConstraints += expr.constraintExpr;
  }

  switch (GetParam()) {
    case interface::GroupCapacitySpecUtilType::LINEAR: {
      // MAX and MIN expressions should cancel each other ensuring that
      // they are symmetric
      EXPECT_EQ(0, evaluate(sumOfConstraints, deltaFromInitial({})));
      break;
    }
    case interface::GroupCapacitySpecUtilType::STEP: {
      EXPECT_EQ(0, evaluate(sumOfConstraints, deltaFromInitial({})));
      break;
    }
    case interface::GroupCapacitySpecUtilType::STEP_MOD_K: {
      XLOG(INFO)
          << "TODO: Requires additional work / bug fixes in MIP model generation";
      break;
    }
  }
}

CO_TEST_P(GroupCapacitySpecBuilderTest, differentPartitions) {
  co_await addPartition(
      "owner",
      {{"owner0", {"task0"}},
       {"owner1", {"task1"}},
       {"owner2", {"task2", "task3"}}});

  interface::GroupCapacitySpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.contributionPartition() = "owner";
  spec.contribution()->globalLimit() = 10;
  spec.contribution()->scopeItemToGroupLimits() = {
      {"host0", {{"owner2", 3}}}, {"host1", {{"owner1", 5}}}};
  spec.limit()->globalLimit() = 5;
  spec.bound() = interface::GroupCapacitySpecBound::MAX;
  spec.definition() = interface::GroupCapacitySpecDefinition::AFTER;
  spec.utilType() = GetParam();

  const GroupCapacitySpecBuilder specBuilder(buildUniverse(), spec);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  switch (GetParam()) {
    case interface::GroupCapacitySpecUtilType::LINEAR: {
      // job0: 10*1 (host0 and owner0) + 10*1  (host0 and owner1) + 10*0
      // (host1 and owner0) + 5*0(host1 and owner1) - 5 (limit) = 15
      // job1 : 3*1 (host0 and owner2) + 10*1 (host1 and owner2)  - 5
      // (limit) = 8 job2: does not map to any groups in 'owner' partition
      EXPECT_NEAR(23, evaluate(goal, deltaFromInitial({})), 1e-8);
      EXPECT_NEAR(
          23, evaluate(goal, deltaFromInitial({{"task0", "host1"}})), 1e-8);
      EXPECT_NEAR(
          18, evaluate(goal, deltaFromInitial({{"task1", "host1"}})), 1e-8);
      EXPECT_NEAR(
          30, evaluate(goal, deltaFromInitial({{"task2", "host1"}})), 1e-8);
      EXPECT_NEAR(
          16, evaluate(goal, deltaFromInitial({{"task3", "host0"}})), 1e-8);
      EXPECT_NEAR(
          23, evaluate(goal, deltaFromInitial({{"task4", "host0"}})), 1e-8);
      EXPECT_NEAR(
          23, evaluate(goal, deltaFromInitial({{"task5", "host0"}})), 1e-8);
      EXPECT_NEAR(
          11,
          evaluate(
              goal,
              deltaFromInitial({
                  {"task1", "host1"},
                  {"task3", "host0"},
              })),
          1e-8);
      break;
    }
    case interface::GroupCapacitySpecUtilType::STEP: {
      // job0: 10*1 (host0 and owner0) + 10*1 (host0 and owner1) + 10*0
      // (host1 and owner0) + 5*0(host1 and owner1) - 5 (limit) = 15
      // job1 : 3*1 (host0 and owner2) + 10 * 1 (host1 and owner2)  - 5
      // (limit) = 8 job2: does not map to any groups in 'owner' partition
      EXPECT_NEAR(23, evaluate(goal, deltaFromInitial({})), 1e-8);
      // job0: 10*0 (host0 and owner0) + 10*1 (host0 and owner1) + 10*1
      // (host1 and owner0) + 5*0(host1 and owner1) - 5 (limit) = 15
      // job1 : 3*1 (host0 and owner2) + 10 * 1 (host1 and owner2)  - 5
      // (limit) = 8
      EXPECT_NEAR(
          23, evaluate(goal, deltaFromInitial({{"task0", "host1"}})), 1e-8);
      // job0: 10*1 (host0 and owner0) + 10*0 (host0 and owner1) + 10*0
      // (host1 and owner0) + 5*1(host1 and owner1) - 5 (limit) = 15
      // job1 : 3*1 (host0 and owner2) + 10 * 1 (host1 and owner2)  - 5
      // (limit) = 8
      EXPECT_NEAR(
          18, evaluate(goal, deltaFromInitial({{"task1", "host1"}})), 1e-8);
      // job0: 10*1 (host0 and owner0) + 10*1 (host0 and owner1) + 10*0
      // (host1 and owner0) + 5*0(host1 and owner1) - 5 (limit) = 15
      // job1 : 3*0 (host0 and owner2) + 10 * 1 (host1 and owner2)  - 5
      // (limit) = 5
      EXPECT_NEAR(
          20, evaluate(goal, deltaFromInitial({{"task2", "host1"}})), 1e-8);
      // job0: 10*1 (host0 and owner0) + 10*1 (host0 and owner1) + 10*0
      // (host1 and owner0) + 5*0(host1 and owner1) - 5 (limit) = 15
      // job1 : 3*1 (host0 and owner2) + 10 * 0 (host1 and owner2)  - 5
      // (limit) = 0 (note that this is max(3-5, 0))
      EXPECT_NEAR(
          15, evaluate(goal, deltaFromInitial({{"task3", "host0"}})), 1e-8);
      // job0: 10*1 (host0 and owner0) + 10*1 (host0 and owner1) + 10*0
      // (host1 and owner0) + 5*0(host1 and owner1) - 5 (limit) = 15
      // job1 : 3*1 (host0 and owner2) + 10 * 1 (host1 and owner2)  - 5
      // (limit) = 8
      EXPECT_NEAR(
          23, evaluate(goal, deltaFromInitial({{"task4", "host0"}})), 1e-8);
      // job0: 10*1 (host0 and owner0) + 10*1 (host0 and owner1) + 10*0
      // (host1 and owner0) + 5*0(host1 and owner1) - 5 (limit) = 15
      // job1 : 3*1 (host0 and owner2) + 10 * 1 (host1 and owner2)  - 5
      // (limit) = 8
      EXPECT_NEAR(
          23, evaluate(goal, deltaFromInitial({{"task5", "host0"}})), 1e-8);
      // job0: 10*1 (host0 and owner0) + 10*0 (host0 and owner1) + 10*0
      // (host1 and owner0) + 5*1(host1 and owner1) - 5 (limit) = 10
      // job1 : 3*1 (host0 and owner2) + 10 * 0 (host1 and owner2)  - 5
      // (limit) = 0 (note that this max(3-5, 0))
      EXPECT_NEAR(
          10,
          evaluate(
              goal,
              deltaFromInitial({
                  {"task1", "host1"},
                  {"task3", "host0"},
              })),
          1e-8);
      break;
    }
    case interface::GroupCapacitySpecUtilType::STEP_MOD_K: {
      XLOG(INFO)
          << "TODO: Requires additional work / bug fixes in MIP model generation";
      break;
    }
  }
}

CO_TEST_P(GroupCapacitySpecBuilderTest, invalidContributionPartition) {
  // create a new contribution partition (jobMod4) that is "coarser" (less
  // granular) than specified partition (job)
  co_await addPartition(
      "jobMod4",
      {{"jobMod4_0", {"task0", "task1", "task2", "task3"}},
       {"jobMod4_1", {"task4", "task5"}}});

  interface::GroupCapacitySpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.contributionPartition() = "jobMod4";
  spec.utilType() = GetParam();

  REBALANCER_EXPECT_RUNTIME_ERROR(
      const GroupCapacitySpecBuilder specBuilder(buildUniverse(), spec),
      "Contribution group jobMod4_0 has objects in multiple groups of partition job");
}

CO_TEST_P(GroupCapacitySpecBuilderTest, TwoContributionGroupsInMainGroup) {
  co_await addPartition(
      "contribution",
      {{"contribution0", {"task0"}}, {"contribution1", {"task1"}}});

  interface::GroupCapacitySpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.contributionPartition() = "contribution";
  spec.contribution()->groupLimits() = {
      {"contribution0", 4}, {"contribution1", 8}};
  spec.limit()->globalLimit() = 5;
  spec.filter()->itemsWhitelist() = {"host0"};
  spec.definition() = interface::GroupCapacitySpecDefinition::AFTER;
  spec.utilType() = GetParam();

  const GroupCapacitySpecBuilder specBuilder(buildUniverse(), spec);
  auto goal = co_await specBuilder.goalCoro(expressionBuilder());

  // job0: 4*1 (contribution0, host0) + 8*1 (contribution0, host0) - 5 = 7
  EXPECT_NEAR(7, evaluate(goal, deltaFromInitial({})), 1e-8);
  // job0: 4*0 (contribution0, host0) + 8*1 (contribution0, host0) - 5 = 3
  // (note that only host0 is part of the filter)
  EXPECT_NEAR(3, evaluate(goal, deltaFromInitial({{"task0", "host1"}})), 1e-8);
  // job0: 4*0 (contribution0, host0) + 8*0 (contribution0, host0) - 5 = 0
  // (note that only host0 is part of the filter and this max (0-5, 0))
  EXPECT_NEAR(0, evaluate(goal, deltaFromInitial({{"task1", "host1"}})), 1e-8);
}

TEST_P(GroupCapacitySpecBuilderTest, SpecInfo) {
  interface::GroupCapacitySpec spec;
  spec.scope() = "host";
  spec.partitionName() = "job";
  spec.limit()->groupLimits() = {{"job0", 2}, {"job1", 4}, {"job2", 2}};
  spec.bound() = interface::GroupCapacitySpecBound::EXACT;
  spec.definition() = interface::GroupCapacitySpecDefinition::DURING_AND_AFTER;

  const GroupCapacitySpecBuilder specBuilder(buildUniverse(), spec);

  auto expectedSpecInfo = SpecParameters{
      .name = "",
      .scope = "host",
      .partition = "job",
      .definition = "DURING_AND_AFTER",
      .boundType = "EXACT",
      .limitType = "ABSOLUTE",
      .filterAllowListSize = 0,
      .filterBlockListSize = 0};
  EXPECT_EQ(expectedSpecInfo, specBuilder.getSpecInfo());
}
} // namespace facebook::rebalancer::materializer::tests
