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
#include "algopt/rebalancer/entities/builders/GoalsBuilder.h"
#include "algopt/rebalancer/entities/builders/tests/BuilderTestBase.h"

#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/coro/GtestHelpers.h>
#include <folly/portability/GTest.h>

namespace facebook::rebalancer::entities::tests {

class GoalsBuilderTest : public BuilderTestBase {};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    GoalsBuilderTest,
    ::testing::Values(1, 3, 20));

CO_TEST_P(GoalsBuilderTest, AddGoals) {
  GoalsBuilder goalsBuilder(idBuilder_);

  const auto goal1Id = goalsBuilder.makeGoalId("goal1");
  const auto goal2Id = goalsBuilder.makeGoalId("goal2");
  const auto goal3Id = goalsBuilder.makeGoalId("goal3");
  EXPECT_NE(goal1Id, goal2Id);
  EXPECT_NE(goal2Id, goal3Id);
  EXPECT_NE(goal1Id, goal3Id);

  co_await folly::coro::co_withExecutor(
      executor.get(),
      folly::coro::collectAll(
          goalsBuilder.add(goal1Id, GoalData{.goal = makeGoal(1.0, 0)}),
          goalsBuilder.add(goal2Id, GoalData{.goal = makeGoal(2.0, 0)}),
          goalsBuilder.add(goal3Id, GoalData{.goal = makeGoal(0.5, 1)})));

  const auto result = co_await goalsBuilder.build();

  EXPECT_EQ(3, result.goalNameToId.size());
  EXPECT_EQ(goal1Id, result.goalNameToId.at("goal1"));
  EXPECT_EQ(goal2Id, result.goalNameToId.at("goal2"));
  EXPECT_EQ(goal3Id, result.goalNameToId.at("goal3"));

  EXPECT_EQ(3, result.goalIdToGoal.size());
  EXPECT_TRUE(result.goalIdToGoal.contains(goal1Id));
  EXPECT_TRUE(result.goalIdToGoal.contains(goal2Id));
  EXPECT_TRUE(result.goalIdToGoal.contains(goal3Id));

  EXPECT_DOUBLE_EQ(1.0, result.goalIdToGoal.at(goal1Id)->getWeight());
  EXPECT_DOUBLE_EQ(2.0, result.goalIdToGoal.at(goal2Id)->getWeight());
  EXPECT_DOUBLE_EQ(0.5, result.goalIdToGoal.at(goal3Id)->getWeight());

  EXPECT_EQ(0, result.goalIdToGoal.at(goal1Id)->getTupleIndex());
  EXPECT_EQ(0, result.goalIdToGoal.at(goal2Id)->getTupleIndex());
  EXPECT_EQ(1, result.goalIdToGoal.at(goal3Id)->getTupleIndex());
}

CO_TEST_P(GoalsBuilderTest, ConcurrentAdd) {
  GoalsBuilder goalsBuilder(idBuilder_);

  constexpr int nGoals = 500;
  std::vector<GoalId> goalIds;
  goalIds.reserve(nGoals);
  for (const auto i : folly::irange(nGoals)) {
    goalIds.push_back(goalsBuilder.makeGoalId(fmt::format("goal_{}", i)));
  }

  std::vector<folly::coro::Task<void>> tasks;
  tasks.reserve(nGoals);
  for (const auto i : folly::irange(nGoals)) {
    tasks.push_back(
        folly::coro::co_invoke(
            [&goalsBuilder,
             goalId = goalIds[i],
             i]() -> folly::coro::Task<void> {
              executeRandomDelay();
              const double weight = static_cast<double>(i) / 100.0;
              const int tupleIndex = i % 3;
              co_await goalsBuilder.add(
                  goalId, GoalData{.goal = makeGoal(weight, tupleIndex)});
            }));
  }

  co_await folly::coro::co_withExecutor(
      executor.get(), folly::coro::collectAllRange(std::move(tasks)));

  const auto result = co_await goalsBuilder.build();

  EXPECT_EQ(nGoals, result.goalNameToId.size());
  EXPECT_EQ(nGoals, result.goalIdToGoal.size());

  for (const auto i : folly::irange(nGoals)) {
    const auto goalName = fmt::format("goal_{}", i);
    EXPECT_TRUE(result.goalNameToId.contains(goalName));
    const auto goalId = result.goalNameToId.at(goalName);
    EXPECT_TRUE(result.goalIdToGoal.contains(goalId));

    const double expectedWeight = static_cast<double>(i) / 100.0;
    const int expectedTupleIndex = i % 3;
    EXPECT_DOUBLE_EQ(
        expectedWeight, result.goalIdToGoal.at(goalId)->getWeight());
    EXPECT_EQ(
        expectedTupleIndex, result.goalIdToGoal.at(goalId)->getTupleIndex());
  }
}

TEST_P(GoalsBuilderTest, DuplicateMakeGoalIdThrows) {
  GoalsBuilder goalsBuilder(idBuilder_);
  std::ignore = goalsBuilder.makeGoalId("goal1");
  REBALANCER_EXPECT_RUNTIME_ERROR(
      goalsBuilder.makeGoalId("goal1"),
      "Unexpected call to makeGoalId with a previously added name 'goal1'");
}

CO_TEST_P(GoalsBuilderTest, Summarize) {
  GoalsBuilder builder(idBuilder_);
  builder.makeGoalId("balance_goal");
  builder.makeGoalId("minimize_moves_goal");

  const auto result = co_await builder.summarize();

  const std::string expected =
      "Goals:\n"
      "  balance_goal\n"
      "  minimize_moves_goal\n";
  EXPECT_EQ(expected, result);
}

CO_TEST_P(GoalsBuilderTest, BuildAddGoalAndRebuild) {
  GoalsBuilder goalsBuilder(idBuilder_);

  const auto goal1Id = goalsBuilder.makeGoalId("goal1");
  co_await goalsBuilder.add(
      goal1Id, GoalData{.goal = makeGoal(/*weight=*/1.0, /*tupleIndex=*/0)});

  const auto result1 = co_await goalsBuilder.build();
  EXPECT_EQ(1, result1.goalNameToId.size());
  EXPECT_EQ(goal1Id, result1.goalNameToId.at("goal1"));
  EXPECT_EQ(1, result1.goalIdToGoal.size());
  EXPECT_DOUBLE_EQ(1.0, result1.goalIdToGoal.at(goal1Id)->getWeight());

  const auto goal2Id = goalsBuilder.makeGoalId("goal2");
  co_await goalsBuilder.add(
      goal2Id, GoalData{.goal = makeGoal(/*weight=*/2.0, /*tupleIndex=*/1)});

  const auto result2 = co_await goalsBuilder.build();

  // Verify that result1 was not mutated by the second build
  EXPECT_EQ(1, result1.goalNameToId.size());
  EXPECT_EQ(goal1Id, result1.goalNameToId.at("goal1"));
  EXPECT_EQ(1, result1.goalIdToGoal.size());
  EXPECT_DOUBLE_EQ(1.0, result1.goalIdToGoal.at(goal1Id)->getWeight());

  EXPECT_EQ(2, result2.goalNameToId.size());
  EXPECT_EQ(goal2Id, result2.goalNameToId.at("goal2"));
  EXPECT_EQ(2, result2.goalIdToGoal.size());
  EXPECT_DOUBLE_EQ(2.0, result2.goalIdToGoal.at(goal2Id)->getWeight());
  EXPECT_EQ(0, result2.goalIdToGoal.at(goal1Id)->getTupleIndex());
  EXPECT_EQ(1, result2.goalIdToGoal.at(goal2Id)->getTupleIndex());
}

} // namespace facebook::rebalancer::entities::tests
