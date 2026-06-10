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

#include "algopt/rebalancer/interface/tests/utils.h"

#include <gtest/gtest.h>

namespace facebook::rebalancer::interface::tests {

class LocalSearchStageSummaryTest : public ::testing::TestWithParam<int> {
 protected:
  void SetUp() override {
    solver = initializeTestProblemSolver({.executorThreadCount = GetParam()});
  }

  void setUpProblem() {
    solver->setObjectName("task");
    solver->setContainerName("host");
    solver->setAssignment(
        std::vector<std::pair<std::string, std::vector<std::string>>>{
            {"h0", {"t0", "t1"}},
            {"h1", {}},
            {"h2", {"t5", "t6"}},
        });

    // h2 is non-accepting
    interface::NonAcceptingSpec nonAcceptingSpec;
    nonAcceptingSpec.scope() = "host";
    nonAcceptingSpec.items() = {"h2"};
    solver->addConstraint(std::move(nonAcceptingSpec));

    // avoid moving t5 and t6
    interface::AvoidMovingSpec avoidMovingSpec;
    avoidMovingSpec.objects() = {"t5", "t6"};
    solver->addConstraint(std::move(avoidMovingSpec));

    // Stage 0: balance
    interface::BalanceSpec balanceSpec;
    balanceSpec.name() = "balance_task_count";
    balanceSpec.scope() = "host";
    balanceSpec.dimension() = "task_count";
    balanceSpec.filter()->itemsBlacklist() = {"h2"};
    solver->addGoal(balanceSpec);

    solver->addGoalBoundary();

    // Stage 1: affinities
    interface::AssignmentAffinitiesSpec assignmentAffinitiesSpec;
    assignmentAffinitiesSpec.name() = "t0_t1_affinities";
    assignmentAffinitiesSpec.scope() = "host";
    assignmentAffinitiesSpec.affinities() = {
        interface::makeAssignmentAffinity("t0", "h1", 1.0),
        interface::makeAssignmentAffinity("t1", "h1", 2.0),
    };
    solver->addGoal(assignmentAffinitiesSpec);
  }

  void addStageSolver() {
    interface::LocalSearchStageSolverSpec localSearchStageSolverSpec;

    interface::LocalSearchStageSpec localSearchStageSpec1;
    localSearchStageSpec1.begin() = 0;
    localSearchStageSpec1.end() = 1;
    localSearchStageSpec1.solverSpec()->moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

    interface::LocalSearchStageSpec localSearchStageSpec2;
    localSearchStageSpec2.begin() = 1;
    localSearchStageSpec2.end() = 2;
    localSearchStageSpec2.solverSpec()->moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));

    localSearchStageSolverSpec.stageSpecs() = {
        localSearchStageSpec1, localSearchStageSpec2};
    localSearchStageSolverSpec.exploreMovesFromContainersNotInObjective() =
        true;

    // Configure local search stages solver
    solver->addSolver(localSearchStageSolverSpec);
  }

  static interface::LocalSearchStageSpec makeStageSpec(
      int begin,
      int end,
      std::optional<double> solveTime = std::nullopt,
      std::optional<int> moveLimit = std::nullopt) {
    interface::LocalSearchStageSpec stageSpec;
    stageSpec.begin() = begin;
    stageSpec.end() = end;
    stageSpec.solverSpec()->moveTypeList() = {
        ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec())};
    stageSpec.solverSpec()->solveTime().from_optional(solveTime);
    stageSpec.solverSpec()->stopAfterMoves().from_optional(moveLimit);
    return stageSpec;
  }

  std::unique_ptr<ProblemSolver> solver;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    LocalSearchStageSummaryTest,
    testThreadCounts());

TEST_P(LocalSearchStageSummaryTest, Basic) {
  setUpProblem();
  addStageSolver();

  // Enable move stats tracking
  solver->enableMoveStats();

  // Solve
  auto solution = solver->solve();

  // Expect stats
  EXPECT_EQ(1, solution.solverSummaries()->size());

  auto& solverSummary = solution.solverSummaries()->at(0);

  // since we are using stage solver, solverSummary should have 2 moveTypes
  EXPECT_EQ(2, solverSummary.moveStats()->moveTypes()->size());
  auto& stagesSummaries = *solverSummary.stagesSummaries();
  EXPECT_EQ(2, stagesSummaries.size());

  auto& stage0 = *stagesSummaries.at(0).finalEvaluationSummary();
  EXPECT_EQ(0, *stage0.globalStats()->totalCount());
  EXPECT_EQ(1, stage0.globalStats()->nonAcceptingContainersCount().value());
  EXPECT_EQ(1, stage0.globalStats()->fixedContainersCount().value());
  EXPECT_EQ(2, stage0.globalStats()->fixedObjectsCount().value());

  auto& stage1 = *stagesSummaries.at(1).finalEvaluationSummary();
  // just check that the field has some value greater than 0
  EXPECT_GE(*stage1.globalStats()->totalCount(), 0);
  EXPECT_EQ(2, stage1.sourceContainerStats()->size());
  EXPECT_EQ(
      1, stage1.sourceContainerStats()->at("h0").brokenConstraints()->size());
  EXPECT_EQ(
      1,
      stage1.sourceContainerStats()->at("h0").brokenConstraints()->at(
          "doNotWorsenGoal: Objective at position 0 cannot worsen in Stage-1: objectives:[1-2)"));

  // objectStats is empty since containers are not tracked in the moveStatsSpec
  EXPECT_EQ(stage1.objectStats()->size(), 0);
}

TEST_P(LocalSearchStageSummaryTest, TrackObjectsTest) {
  setUpProblem();
  addStageSolver();

  // Enable object tracking
  MoveStatsSpec moveStatsSpec;
  moveStatsSpec.trackObjects() = true;
  solver->enableMoveStats(std::move(moveStatsSpec));

  // Solve
  auto solution = solver->solve();

  // Expect stats
  EXPECT_EQ(1, solution.solverSummaries()->size());
  auto& solverSummary = solution.solverSummaries()->at(0);

  auto& stagesSummaries = *solverSummary.stagesSummaries();
  EXPECT_EQ(2, stagesSummaries.size());

  // empty stage0 finalEvaluationSummary since there are no evaluations after
  // the only move in that stage
  auto& stage0 = *stagesSummaries.at(0).finalEvaluationSummary();
  EXPECT_EQ(0, *stage0.globalStats()->totalCount());

  // just check that the field has some value greater than 0
  auto& stage1 = *stagesSummaries.at(1).finalEvaluationSummary();
  EXPECT_LE(1, *stage1.globalStats()->totalCount());

  // sourceContainerStats is empty since containers are not tracked in the
  // moveStatsSpec
  EXPECT_EQ(0, stage1.sourceContainerStats()->size());

  // we expect 2 object stats in the finalEvaluationSummary for stage1
  EXPECT_EQ(2, stage1.objectStats()->size());

  EXPECT_EQ(1, stage1.objectStats()->at("t0").brokenConstraints()->size());
  EXPECT_EQ(
      1,
      stage1.objectStats()->at("t0").brokenConstraints()->at(
          "doNotWorsenGoal: Objective at position 0 cannot worsen in Stage-1: objectives:[1-2)"));

  EXPECT_EQ(1, stage1.objectStats()->at("t1").brokenConstraints()->size());
  EXPECT_EQ(
      1,
      stage1.objectStats()->at("t1").brokenConstraints()->at(
          "doNotWorsenGoal: Objective at position 0 cannot worsen in Stage-1: objectives:[1-2)"));
  EXPECT_FALSE(
      stage1.objectStats()->at("t1").nonAcceptingContainersCount().has_value());
  EXPECT_FALSE(
      stage1.objectStats()->at("t1").fixedContainersCount().has_value());
  EXPECT_FALSE(stage1.objectStats()->at("t1").fixedObjectsCount().has_value());
}

TEST_P(LocalSearchStageSummaryTest, TrackObjectsWithObjectWhiteListTest) {
  // same example as TrackObjectsTest, but where we are only tracking one of the
  // objects
  setUpProblem();
  addStageSolver();

  // Enable object tracking
  MoveStatsSpec moveStatsSpec;
  moveStatsSpec.trackObjects() = true;
  moveStatsSpec.trackObjectsWhitelist() = {"t1"};
  solver->enableMoveStats(std::move(moveStatsSpec));

  // Solve
  auto solution = solver->solve();

  // Expect stats
  EXPECT_EQ(1, solution.solverSummaries()->size());
  auto& solverSummary = solution.solverSummaries()->at(0);

  auto& stagesSummaries = *solverSummary.stagesSummaries();
  EXPECT_EQ(2, stagesSummaries.size());

  // empty stage0 finalEvaluationSummary since there are no evaluations after
  // the only move in that stage
  auto& stage0 = *stagesSummaries.at(0).finalEvaluationSummary();
  EXPECT_EQ(0, *stage0.globalStats()->totalCount());

  // just check that the field has some value greater than 0
  auto& stage1 = *stagesSummaries.at(1).finalEvaluationSummary();
  EXPECT_LE(1, *stage1.globalStats()->totalCount());

  // sourceContainerStats is empty since containers are not tracked in the
  // moveStatsSpec
  EXPECT_EQ(0, stage1.sourceContainerStats()->size());

  // we expect 1 object stats in the finalEvaluationSummary for stage1 since we
  // are only tracking "t1"
  EXPECT_EQ(1, stage1.objectStats()->size());

  EXPECT_EQ(1, stage1.objectStats()->at("t1").brokenConstraints()->size());
  EXPECT_EQ(
      1,
      stage1.objectStats()->at("t1").brokenConstraints()->at(
          "doNotWorsenGoal: Objective at position 0 cannot worsen in Stage-1: objectives:[1-2)"));

  // expect 1 moves summary since only one move is made
  EXPECT_EQ(1, solution.movesSummary()->size());

  auto onlyMove = solution.movesSummary()->at(0);

  // moveStatsSpec.showAllChangedObjectivesInMovesSummary() is set to
  // false by default; so we expect only the first chanegd objective (tuple 0
  // objective) to be present in the moves summary
  EXPECT_EQ(1, onlyMove.objectives()->size());

  auto& obj1Change = onlyMove.objectives()->at("balance_task_count");
  EXPECT_NEAR(0, *obj1Change.newValue(), 1e-8);
  EXPECT_NEAR(0.5, *obj1Change.change(), 1e-8);
  EXPECT_EQ(0, *obj1Change.tuplePos());
}

TEST_P(
    LocalSearchStageSummaryTest,
    showAllChangedObjectivesInMovesSummaryTest) {
  setUpProblem();
  addStageSolver();

  // Enable object tracking
  MoveStatsSpec moveStatsSpec;
  moveStatsSpec.trackObjects() = true;
  moveStatsSpec.trackObjectsWhitelist() = {"t1"};
  moveStatsSpec.showAllChangedObjectivesInMovesSummary() = true;
  solver->enableMoveStats(std::move(moveStatsSpec));

  // Solve
  auto solution = solver->solve();

  // expect 1 moves summary since only one move is made
  EXPECT_EQ(1, solution.movesSummary()->size());

  auto onlyMove = solution.movesSummary()->at(0);

  // moveStatsSpec.showAllChangedObjectivesInMovesSummary() is set to
  // true; so we expect all changed objectives to be present in the moves
  // summary
  EXPECT_EQ(2, onlyMove.objectives()->size());

  auto& obj1Change = onlyMove.objectives()->at("balance_task_count");
  EXPECT_NEAR(0, *obj1Change.newValue(), 1e-8);
  EXPECT_NEAR(0.5, *obj1Change.change(), 1e-8);
  EXPECT_EQ(0, *obj1Change.tuplePos());

  auto& obj2Change = onlyMove.objectives()->at("t0_t1_affinities");
  EXPECT_NEAR(-2, *obj2Change.newValue(), 1e-8);
  EXPECT_NEAR(2, *obj2Change.change(), 1e-8);
  EXPECT_EQ(1, *obj2Change.tuplePos());
}

TEST_P(LocalSearchStageSummaryTest, ZeroSolveTimeInAStage) {
  setUpProblem();

  interface::LocalSearchStageSolverSpec localSearchStageSolverSpec;
  localSearchStageSolverSpec.stageSpecs() = {
      makeStageSpec(0, 1, 0, 100), makeStageSpec(1, 2, 0, 100)};

  solver->addSolver(localSearchStageSolverSpec);

  auto solution = solver->solve();

  EXPECT_EQ(1, solution.solverSummaries()->size());
  auto& solverSummary = solution.solverSummaries()->at(0);

  auto& stagesSummaries = *solverSummary.stagesSummaries();
  EXPECT_EQ(2, stagesSummaries.size());
  EXPECT_EQ(
      interface::EndReason::HIT_TIME_LIMIT, stagesSummaries.at(0).endReason());
  EXPECT_EQ(
      interface::EndReason::HIT_TIME_LIMIT, stagesSummaries.at(1).endReason());

  auto& stage0 = *stagesSummaries.at(0).finalEvaluationSummary();
  EXPECT_EQ(0, *stage0.globalStats()->totalCount());
  EXPECT_EQ(1, stage0.globalStats()->nonAcceptingContainersCount().value());
  EXPECT_EQ(1, stage0.globalStats()->fixedContainersCount().value());
  EXPECT_EQ(2, stage0.globalStats()->fixedObjectsCount().value());

  auto& stage1 = *stagesSummaries.at(1).finalEvaluationSummary();
  EXPECT_EQ(0, *stage1.globalStats()->totalCount());
  EXPECT_EQ(1, stage1.globalStats()->nonAcceptingContainersCount().value());
  EXPECT_EQ(1, stage1.globalStats()->fixedContainersCount().value());
  EXPECT_EQ(2, stage1.globalStats()->fixedObjectsCount().value());
}

TEST_P(LocalSearchStageSummaryTest, ZeroSolveTimeGlobal) {
  setUpProblem();

  interface::LocalSearchStageSolverSpec localSearchStageSolverSpec;
  localSearchStageSolverSpec.stageSpecs() = {
      makeStageSpec(0, 1), makeStageSpec(1, 2)};
  localSearchStageSolverSpec.solveTime() = 0;

  solver->addSolver(localSearchStageSolverSpec);

  auto solution = solver->solve();

  EXPECT_EQ(1, solution.solverSummaries()->size());
  auto& solverSummary = solution.solverSummaries()->at(0);
  EXPECT_EQ(interface::EndReason::HIT_TIME_LIMIT, solverSummary.endReason());
  auto& stagesSummaries = *solverSummary.stagesSummaries();
  EXPECT_EQ(2, stagesSummaries.size());
  EXPECT_EQ(
      interface::EndReason::HIT_TIME_LIMIT, stagesSummaries.at(0).endReason());
  EXPECT_EQ(
      interface::EndReason::HIT_TIME_LIMIT, stagesSummaries.at(1).endReason());

  auto& stage0 = *stagesSummaries.at(0).finalEvaluationSummary();
  EXPECT_EQ(0, *stage0.globalStats()->totalCount());
  EXPECT_EQ(1, stage0.globalStats()->nonAcceptingContainersCount().value());
  EXPECT_EQ(1, stage0.globalStats()->fixedContainersCount().value());
  EXPECT_EQ(2, stage0.globalStats()->fixedObjectsCount().value());

  auto& stage1 = *stagesSummaries.at(1).finalEvaluationSummary();
  EXPECT_EQ(0, *stage1.globalStats()->totalCount());
  EXPECT_EQ(1, stage1.globalStats()->nonAcceptingContainersCount().value());
  EXPECT_EQ(1, stage1.globalStats()->fixedContainersCount().value());
  EXPECT_EQ(2, stage1.globalStats()->fixedObjectsCount().value());
}

TEST_P(LocalSearchStageSummaryTest, ZeroMoveTimeInAStage) {
  setUpProblem();

  interface::LocalSearchStageSolverSpec localSearchStageSolverSpec;
  localSearchStageSolverSpec.stageSpecs() = {makeStageSpec(0, 1, 100, 0)};

  solver->addSolver(localSearchStageSolverSpec);

  auto solution = solver->solve();

  EXPECT_EQ(1, solution.solverSummaries()->size());
  auto& solverSummary = solution.solverSummaries()->at(0);
  auto& stagesSummaries = *solverSummary.stagesSummaries();
  EXPECT_EQ(1, stagesSummaries.size());
  EXPECT_EQ(
      interface::EndReason::HIT_MOVE_LIMIT, stagesSummaries.at(0).endReason());
}

TEST_P(LocalSearchStageSummaryTest, ZeroMoveTimeGlobal) {
  setUpProblem();

  interface::LocalSearchStageSolverSpec localSearchStageSolverSpec;
  localSearchStageSolverSpec.stageSpecs() = {
      makeStageSpec(0, 1), makeStageSpec(0, 1)};
  localSearchStageSolverSpec.stopAfterMoves() = 0;

  solver->addSolver(localSearchStageSolverSpec);

  auto solution = solver->solve();

  EXPECT_EQ(1, solution.solverSummaries()->size());
  auto& solverSummary = solution.solverSummaries()->at(0);
  EXPECT_EQ(interface::EndReason::HIT_MOVE_LIMIT, solverSummary.endReason());
  auto& stagesSummaries = *solverSummary.stagesSummaries();
  EXPECT_EQ(2, stagesSummaries.size());
  EXPECT_EQ(
      interface::EndReason::HIT_MOVE_LIMIT, stagesSummaries.at(0).endReason());
  EXPECT_EQ(
      interface::EndReason::HIT_MOVE_LIMIT, stagesSummaries.at(1).endReason());
}
} // namespace facebook::rebalancer::interface::tests
