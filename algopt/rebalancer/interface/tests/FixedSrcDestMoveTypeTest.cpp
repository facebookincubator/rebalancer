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

#include <folly/container/irange.h>
#include <gtest/gtest.h>

using namespace facebook::rebalancer::interface;

class FixedSrcDestMoveTypeTest : public testing::TestWithParam<int> {
 protected:
  void SetUp() override;

  LocalSearchStageSpec makeStageSpec(
      std::string name,
      int goalIdx,
      std::vector<MoveTypeSpec> preferredMoves);

  void setupLocalSearchWithStages(
      std::map<std::string, std::vector<std::string>> initialContainerToObjects,
      std::vector<LocalSearchStageSpec> stages);

  std::unique_ptr<ProblemSolver> solver_;
  std::map<std::string, std::string> serverType_;
  std::string specialContainer_;
};

INSTANTIATE_TEST_CASE_P(
    NumThreads,
    FixedSrcDestMoveTypeTest,
    testThreadCounts());

void FixedSrcDestMoveTypeTest::SetUp() {
  solver_ = initializeTestProblemSolver({.executorThreadCount = GetParam()});
  solver_->setObjectName("server");
  solver_->setContainerName("reservation");

  ManifoldBackupParams params;
  params.uploadPolicy() = ManifoldUploadPolicy::NEVER;
  solver_->setManifoldBackupParams(params);

  // "unassigned" will be the special unassigned container.
  specialContainer_ = "unassigned";
}

LocalSearchStageSpec FixedSrcDestMoveTypeTest::makeStageSpec(
    std::string name,
    int goalIdx,
    std::vector<MoveTypeSpec> preferredMoves) {
  // each stage focuses on exactly one goal
  LocalSearchStageSpec stage;
  stage.name() = name;
  stage.begin() = goalIdx;
  stage.end() = goalIdx + 1;
  stage.solverSpec()->exploreMovesFromContainersNotInObjective() = false;
  stage.solverSpec()->specialContainer() = specialContainer_;
  stage.solverSpec()->moveTypeList() = preferredMoves;
  return stage;
}

void FixedSrcDestMoveTypeTest::setupLocalSearchWithStages(
    std::map<std::string, std::vector<std::string>> initialContainerToObjects,
    std::vector<LocalSearchStageSpec> stages) {
  solver_->setAssignment(initialContainerToObjects);

  serverType_ = {
      {"server0", "T1"},
      {"server1", "T1"},
      {"server2", "T1"},
      {"server3", "T5"},
      {"server4", "T5"},
      {"server5", "T5"},
  };
  solver_->addPartition("serverType", serverType_);

  CapacitySpec capacitySpec;
  capacitySpec.scope() = "reservation";
  capacitySpec.dimension() = "server_count";
  capacitySpec.filter()->itemsWhitelist() = {"reservation1", "reservation2"};
  capacitySpec.limit()->globalLimit() = 2;

  // Goal 0: Capacity LB
  capacitySpec.name() = "capacity LB";
  capacitySpec.bound() = CapacitySpecBound::MIN;
  solver_->addGoal(capacitySpec);
  solver_->addGoalBoundary();

  // Goal 1: Capacity UB
  capacitySpec.name() = "capacity UB";
  capacitySpec.bound() = CapacitySpecBound::MAX;
  solver_->addGoal(capacitySpec);
  solver_->addGoalBoundary();

  // Goal 2: Spread of "unassigned" container
  GroupCountSpec groupCountSpec;
  groupCountSpec.name() = "unassigned spread";
  groupCountSpec.scope() = "reservation";
  groupCountSpec.dimension() = "server_count";
  groupCountSpec.partitionName() = "serverType";
  groupCountSpec.filter()->itemsWhitelist() = {specialContainer_};
  groupCountSpec.bound() = GroupCountSpecBound::MIN;
  groupCountSpec.limit()->type() = LimitType::ABSOLUTE;
  groupCountSpec.limit()->globalLimit() = 1;
  solver_->addGoal(groupCountSpec);

  LocalSearchStageSolverSpec spec;
  spec.stageSpecs() = std::move(stages);
  solver_->addSolver(spec);
}

TEST_P(FixedSrcDestMoveTypeTest, fixDeficitAndExcess) {
  SingleFixedSourceMoveTypeSpec singleFixedSourceMoveTypeSpec;
  singleFixedSourceMoveTypeSpec.specialContainer() = specialContainer_;
  const std::vector<LocalSearchStageSpec> stages = {
      makeStageSpec(
          "capacity LB",
          0,
          {ProblemSolver::makeMoveTypeSpec(singleFixedSourceMoveTypeSpec)}),
      makeStageSpec(
          "capacity UB",
          1,
          {ProblemSolver::makeMoveTypeSpec("SINGLE_FIXED_DEST")}),
  };
  // TODO: add a new stage to improve only spread goals with swaps

  setupLocalSearchWithStages(
      {{specialContainer_, {"server0", "server1"}},
       {"reservation1", {"server2", "server3", "server4"}},
       {"reservation2", {"server5"}}},
      stages);

  auto solution = solver_->solve();
  auto initialGoals = *solution.initialGlobalObjective()->goals();
  auto finalGoals = *solution.finalGlobalObjective()->goals();
  EXPECT_EQ(initialGoals.size(), finalGoals.size());
  for (const auto i : folly::irange(initialGoals.size())) {
    auto& obj = initialGoals.at(i);
    auto& objAfter = finalGoals.at(i);
    EXPECT_EQ(*obj.objs()->at(0).name(), *objAfter.objs()->at(0).name());
    // initially all goals have value 1
    EXPECT_EQ(1, *obj.objs()->at(0).value());
    // after solve, all capacity goals have value 0
    if (*obj.objs()->at(0).name() != "unassigned spread") {
      EXPECT_EQ(0, *objAfter.objs()->at(0).value());
    }
    XLOG(INFO) << fmt::format(
        "obj: {} value: {} → {}",
        *obj.objs()->at(0).name(),
        *obj.objs()->at(0).value(),
        *objAfter.objs()->at(0).value());
  }
}

TEST_P(FixedSrcDestMoveTypeTest, fixDeficitAfterFixingExcess) {
  SingleFixedSourceMoveTypeSpec singleFixedSourceMoveTypeSpec;
  singleFixedSourceMoveTypeSpec.specialContainer() = specialContainer_;
  const std::vector<LocalSearchStageSpec> stages = {
      makeStageSpec(
          "capacity UB",
          1,
          {ProblemSolver::makeMoveTypeSpec("SINGLE_FIXED_DEST")}),
      makeStageSpec(
          "capacity LB",
          0,
          {ProblemSolver::makeMoveTypeSpec(singleFixedSourceMoveTypeSpec)}),
  };
  // TODO: add a new stage to improve only spread goals with swaps

  setupLocalSearchWithStages(
      {{specialContainer_, {}},
       {"reservation1",
        {"server0", "server1", "server2", "server3", "server4"}},
       {"reservation2", {"server5"}}},
      stages);

  auto solution = solver_->solve();
  auto initialGoals = *solution.initialGlobalObjective()->goals();
  auto finalGoals = *solution.finalGlobalObjective()->goals();
  EXPECT_EQ(initialGoals.size(), finalGoals.size());
  for (const auto i : folly::irange(initialGoals.size())) {
    auto& obj = initialGoals.at(i);
    auto& objAfter = finalGoals.at(i);
    EXPECT_EQ(*obj.objs()->at(0).name(), *objAfter.objs()->at(0).name());
    // after solve, all capacity goals have value 0
    if (*obj.objs()->at(0).name() != "unassigned spread") {
      EXPECT_EQ(0, *objAfter.objs()->at(0).value());
    }
    XLOG(INFO) << fmt::format(
        "obj: {} value: {} → {}",
        *obj.objs()->at(0).name(),
        *obj.objs()->at(0).value(),
        *objAfter.objs()->at(0).value());
  }
}
