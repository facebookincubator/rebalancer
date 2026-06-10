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
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <folly/container/irange.h>
#include <gtest/gtest.h>

#include <map>
#include <random>
#include <string>
#include <tuple>

namespace facebook::rebalancer::interface::tests {

using facebook::algopt::isSolverUnavailable;
using facebook::algopt::solverName;
using facebook::algopt::testSolverPackages;

class ToFreeTest : public ::testing::TestWithParam<
                       std::tuple<SolverAlgoType, int, OptimalSolverPackage>> {
 protected:
  void SetUp() override {
    const auto [algoType, threads, solver] = GetParam();
    if (algoType == SolverAlgoType::OPTIMAL) {
      if (isSolverUnavailable(solver)) {
        GTEST_SKIP() << solverName(solver) << " solver not available";
      }
    }
    solver_ = initializeTestProblemSolver({.executorThreadCount = threads});
    solver_->setObjectName("task");
    solver_->setContainerName("host");
  }

  static SolverAlgoType getSolverAlgoType() {
    const auto [algoType, threads, solver] = GetParam();
    return algoType;
  }

  void addSolver(
      std::optional<interface::LocalSearchSolverSpec> customLocalSearchSolver =
          std::nullopt) {
    switch (getSolverAlgoType()) {
      case SolverAlgoType::LOCALSEARCH: {
        if (customLocalSearchSolver.has_value()) {
          solver_->addSolver(*customLocalSearchSolver);
        } else {
          const interface::LocalSearchSolverSpec localSearchSolverSpec =
              makeDefaultLocalSearchSolver();
          solver_->addSolver(localSearchSolverSpec);
        }
        break;
      }
      case SolverAlgoType::OPTIMAL: {
        interface::OptimalSolverSpec optimalSolverSpec;
        const auto [algoType, threads, solverPkg] = GetParam();
        optimalSolverSpec.solverPackage() = solverPkg;
        solver_->addSolver(optimalSolverSpec);
        break;
      }
    }
  }

  std::unique_ptr<ProblemSolver> solver_;
};

INSTANTIATE_TEST_CASE_P(
    LocalSearch,
    ToFreeTest,
    ::testing::Combine(
        ::testing::Values(SolverAlgoType::LOCALSEARCH),
        testThreadCounts(),
        ::testing::Values(OptimalSolverPackage::GUROBI)));

INSTANTIATE_TEST_CASE_P(
    Optimal,
    ToFreeTest,
    ::testing::Combine(
        ::testing::Values(SolverAlgoType::OPTIMAL),
        testThreadCounts(),
        testSolverPackages()));

TEST_P(ToFreeTest, FullyFreed) {
  // In this example there are 2 hosts and 4 tasks. A constraint forces host0
  // to become empty. task3 which is initially in host1 has an affinity for
  // host0, however moving task3 to host0 would require violating the constraint
  // so Rebalancer doesn't do it.

  // host0 initially has 3 tasks in it, and host1 only 1.
  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
      });

  // host0 must be empty.
  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "xyz";
  toFreeSpec.containers() = {"host0"};
  solver_->addConstraint(toFreeSpec);

  // task3 prefers host0.
  AssignmentAffinitiesSpec assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = {
      makeAssignmentAffinity("task3", "host0", 1.0)};

  solver_->addGoal(assignmentAffinitiesSpec);

  addSolver();

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  // We expect all tasks to be in host1. The affinity of task3 for host0 is
  // ignored because that assignment would violate the ToFree constraint.
  EXPECT_EQ("host1", assignment["task0"]);
  EXPECT_EQ("host1", assignment["task1"]);
  EXPECT_EQ("host1", assignment["task2"]);
  EXPECT_EQ("host1", assignment["task3"]);

  // The solution summary refers to the constraint using the same name we've
  // given it before.
  EXPECT_EQ("xyz", *solution.initialConstraint()->constraints()->at(0).name());
}

TEST_P(ToFreeTest, PartiallyFreed) {
  // In this example there are 2 hosts and 4 tasks. All tasks are initially in
  // host0. A constraint forces host0 to become empty. A capacity constraint
  // prevents either host from having more than 3 tasks. Only 3 tasks are moved
  // from host0 to host1. Moving the 4th task would violate the capacity
  // constraint on host1, which is not broken initially. The capacity constraint
  // is divided into one component per scope item, and each is separately
  // considered initially broken or not. Rebalancer would rather not fix
  // completely an already broken constraint, than to break another constraint
  // that wasn't broken before.

  // There are 2 hosts and 4 tasks. All tasks are in host0 initially.
  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2", "task3"}},
          {"host1", {}},
      });

  // host0 must be empty.
  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "xyz";
  toFreeSpec.containers() = {"host0"};
  solver_->addConstraint(toFreeSpec);

  // A host may not contain more than 3 tasks.
  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "task_count";

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 3;
  capacitySpec.limit() = limit;

  solver_->addConstraint(capacitySpec);

  addSolver();

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  std::map<std::string, int> tasksInHost;
  for (auto& [_, host] : assignment) {
    ++tasksInHost[host];
  }

  // We expect 3 tasks to move from host0 to host1, and 1 task to stay in host0.
  // This solution fixes the initially broken ToFree constraint without breaking
  // other constraints in the process. Moving all tasks from host0 to host1
  // would completely fix the ToFree constraint, but it would be at the cost of
  // breaking the capacity constraint on host1, which is initially not broken.
  EXPECT_EQ(1, tasksInHost["host0"]);
  EXPECT_EQ(3, tasksInHost["host1"]);
}

TEST_P(ToFreeTest, SwapNotAllowed) {
  // In this example there are 2 hosts with 1 task each. Both hosts are required
  // to become empty, but since there are no other hosts to take the tasks in,
  // there's no way to fix the initially broken constraint. An
  // AssignmentAffinities objective makes each task prefer the other host, and
  // swapping the tasks would improve this objective. However, the ToFree
  // constraint imposes that no new objects may move into a container to free,
  // making the swap invalid.

  // There are 2 hosts, and 1 task inside each host.
  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0"}},
          {"host1", {"task1"}},
      });

  // Both hosts must be empty.
  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "xyz";
  toFreeSpec.containers() = {"host0", "host1"};

  solver_->addConstraint(toFreeSpec);

  // task0 prefers host1 and task1 prefers host0.
  AssignmentAffinitiesSpec assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = {
      makeAssignmentAffinity("task0", "host1", 1.0),
      makeAssignmentAffinity("task1", "host0", 1.0),
  };

  solver_->addGoal(assignmentAffinitiesSpec);

  addSolver();

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  // No moves expected. Even though tasks would prefer to be swapped from the
  // perspective of the affinities objective, swapping them would violate the
  // property that no new objects may move into a container to be freed.
  EXPECT_EQ("host0", assignment["task0"]);
  EXPECT_EQ("host1", assignment["task1"]);

  // expect moves summary to be empty since no moves were made
  EXPECT_EQ(0, solution.movesSummary()->size());
}

TEST_P(ToFreeTest, ToFreeConstraintWithCustomDimension) {
  // In this example there are 2 hosts and 4 tasks. A constraint forces host0
  // to become empty w.r.t. dimension 'onlyTask0Matters'. task3 which is
  // initially in host1 has an affinity for host0, and it can move into host0
  // since only task0 has non-zero value w.r.t. the dimension
  // 'onlyTask0Matters'.
  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
      });

  solver_->addObjectDimension(
      "onlyTask0Matters",
      std::unordered_map<std::string, double>{{"task0", 1.0}},
      0.0);

  {
    // host0 must be empty w.r.t. dimension 'onlyTask0Matters'.
    ToFreeSpec toFreeSpec;
    toFreeSpec.name() = "free host0 w.r.t. onlyTask0Matters";
    toFreeSpec.containers() = {"host0"};
    toFreeSpec.dimension() = "onlyTask0Matters";
    solver_->addConstraint(toFreeSpec);
  }

  {
    // task3 prefers host0.
    AssignmentAffinitiesSpec assignmentAffinitiesSpec;
    assignmentAffinitiesSpec.scope() = "host";
    assignmentAffinitiesSpec.affinities() = {
        makeAssignmentAffinity("task3", "host0", 1.0)};

    solver_->addGoal(assignmentAffinitiesSpec);
  }

  addSolver();

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  // task0 must have left host0 (ToFree constraint on onlyTask0Matters).
  EXPECT_NE("host0", assignment["task0"]);
  // task3 should be in host0 due to the affinity goal.
  EXPECT_EQ("host0", assignment["task3"]);
  // task1 and task2 have no movement reason, but the solver may freely
  // move them since there is no movement-minimization goal.
}

TEST_P(ToFreeTest, TestToFreeAsGoal) {
  // In this example there are 2 hosts and 4 tasks; the first host has 3 tasks.
  // The goal is to free "host0" and there is a capacity constraint.
  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
      });

  { // "host0" must be empty"
    ToFreeSpec toFreeSpec;
    toFreeSpec.name() = "drain host0";
    toFreeSpec.containers() = {"host0"};

    solver_->addGoal(toFreeSpec);
  }

  {
    // tie-breaker goal: prefer task3 on host0.
    AssignmentAffinitiesSpec assignmentAffinitiesSpec;
    assignmentAffinitiesSpec.scope() = "host";
    assignmentAffinitiesSpec.affinities() = {
        makeAssignmentAffinity("task3", "host0", 1.0)};

    solver_->addGoal(assignmentAffinitiesSpec, /*weight=*/1, /*tuplePos=*/1);
  }

  {
    CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "memory";

    // "task3" has "memory" value of 0.6; all the other have value 0.5
    solver_->addObjectDimension(
        "memory", std::unordered_map<std::string, double>{{"task3", 0.6}}, 0.5);

    solver_->addContainerDimension(
        "memory",
        std::map<std::string, double>{
            {"host0", 100},
            {"host1", 1.55},
        });

    solver_->addConstraint(capacitySpec);
  }

  addSolver();

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  std::map<std::string, int> tasksInHost;
  for (auto& [obj, host] : assignment) {
    ++tasksInHost[host];
  }

  // Note that when used as a goal, ToFree only cares about minimizing the
  // number of objects in "host0" (the one that needs to be freed). Therefore,
  // the optimal solution is to move all the three tasks initially in "host0" to
  // "host1" and move "task3" that is initially in "host1" to "host0".
  EXPECT_EQ(1, tasksInHost["host0"]);
  EXPECT_EQ(3, tasksInHost["host1"]);
  EXPECT_EQ("host0", assignment.at("task3"));
}

TEST_P(ToFreeTest, ToFreeGoalWithCustomDimension) {
  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1", "task2"}},
          {"host1", {"task3"}},
      });

  solver_->addObjectDimension(
      "onlyTask0Matters",
      std::unordered_map<std::string, double>{{"task0", 1.0}},
      0.0);

  {
    // primary goal: "host0" must be empty" w.r.t. "onlyTask0Matters"
    ToFreeSpec toFreeSpec;
    toFreeSpec.name() = "drain host0 w.r.t. onlyTask0Matters";
    toFreeSpec.containers() = {"host0"};
    toFreeSpec.dimension() = "onlyTask0Matters";

    solver_->addGoal(toFreeSpec);
  }

  {
    // secondary goal: minimize moves
    MinimizeMovementSpec minimizeMovementSpec;
    minimizeMovementSpec.scope() = "host";
    minimizeMovementSpec.dimension() = "task_count";

    solver_->addGoal(minimizeMovementSpec, 1, 1 /*tuplePos*/);
  }

  {
    CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "memory";

    // "task3" has "memory" value of 2; all the other have value 0.5
    solver_->addObjectDimension(
        "memory", std::unordered_map<std::string, double>{{"task3", 2}}, 0.5);
    solver_->addContainerDimension(
        "memory",
        std::map<std::string, double>{{"host0", 100}, {"host1", 1.5}});
    solver_->addConstraint(capacitySpec);
  }

  addSolver();

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  // Given that there is a primary goal to free "host0" w.r.t.
  // "onlyTask0Matters" dimension and a secondary goal to minimize moves, the
  // optimal solution here is move "task0" to "host1" and move "task3" that is
  // initially in "host1" to "host0".
  EXPECT_EQ("host1", assignment["task0"]);
  EXPECT_EQ("host0", assignment["task1"]);
  EXPECT_EQ("host0", assignment["task2"]);
  EXPECT_EQ("host0", assignment["task3"]);
}

TEST_P(ToFreeTest, MultipleToFree) {
  // In this example there are 3 hosts and the first two hosts have 2 tasks
  // each; last host is empty. There is a ToFree constraint saying that
  // "host0" needs to be emptied and there is a ToFree goal which tries to drain
  // "host1" as much as possible. Additionally, there is a capacity constraint
  // which limits the number of tasks per host to 3. This in turn implies that
  // in the optimal solution, host0 will have zero containers and host1 will
  // have one container (since rebalancer will prioritize fixing the constraint
  // on "host0" over draining "host1".)

  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
          {"host2", {}},
      });

  { // "host0" must be empty"
    ToFreeSpec toFreeSpec;
    toFreeSpec.name() = "drain host0";
    toFreeSpec.containers() = {"host0"};

    solver_->addConstraint(toFreeSpec);
  }

  { // drain "host1" as much as possible"
    ToFreeSpec toFreeSpec;
    toFreeSpec.name() = "try to drain host1";
    toFreeSpec.containers() = {"host1"};

    solver_->addGoal(toFreeSpec);
  }

  {
    CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "task_count";

    Limit limit;
    limit.type() = LimitType::ABSOLUTE;
    limit.globalLimit() = 3.0;

    capacitySpec.limit() = limit;

    solver_->addConstraint(capacitySpec);
  }

  addSolver();

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  std::map<std::string, int> tasksInHost;
  for (auto& [_, host] : assignment) {
    ++tasksInHost[host];
  }

  // We expect host0 to have zero containers and host1 to have one container
  // (since rebalancer will prioritize fixing the constraint on "host0" over
  // draining "host1", while making the sure that the capacity constraint is
  // satisfied)
  EXPECT_EQ(0, tasksInHost["host0"]);
  EXPECT_EQ(1, tasksInHost["host1"]);
  EXPECT_EQ(3, tasksInHost["host2"]);
}

TEST_P(ToFreeTest, ToFreeAndStageSolver) {
  // This is an example not so much about ToFreeSpec itself, but a subtle case
  // that can arise when using the stageSolver. In particular, this test shows
  // why when evaluating the arbiterGoal in MovesEvaluator, it is important to
  // evaluate all tuple position. See NOTE in MovesEvaluator::evaluate().
  constexpr int nTasks = 1000;
  std::vector<std::string> host0Tasks;
  std::map<std::string, double> taskToLoad;
  for (const auto i : folly::irange(nTasks)) {
    auto taskName = fmt::format("task-{}", i);
    host0Tasks.push_back(taskName);
    taskToLoad[taskName] = i + 10;
  }

  // using a random task because if arbiterGoals are not evaluated correctly,
  // this test will fail, but it depends on tie-breaking
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(0, nTasks - 1);
  std::string zeroWeightTask = fmt::format("task-{}", dist(gen));

  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", host0Tasks},
          {"host1", {}},
      });

  // only zeroWeightTask has load 0, everything else has load >= 1
  taskToLoad[zeroWeightTask] = 0.0;
  solver_->addObjectDimension("task_load", taskToLoad);

  { // "host0" must be empty"
    ToFreeSpec toFreeSpec;
    toFreeSpec.name() = "drain host0";
    toFreeSpec.containers() = {"host0"};

    solver_->addConstraint(toFreeSpec);
  }

  { // "host1" should have minimal number of elements"
    ToFreeSpec toFreeSpec;
    toFreeSpec.name() = "drain host1";
    toFreeSpec.containers() = {"host1"};

    solver_->addGoal(toFreeSpec, 1, 1 /*tuplePos*/);
  }

  {
    CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "task_load";
    capacitySpec.limit()->globalLimit() = 0.0;
    capacitySpec.filter()->itemsBlacklist() = {"host0"};

    solver_->addGoal(capacitySpec, 1, 2);
  }

  // Setup StageSolver
  {
    LocalSearchStageSolverSpec solverSpec;

    LocalSearchStageSpec stage0Spec;
    stage0Spec.begin() = 0;
    stage0Spec.end() = 1;
    stage0Spec.solverSpec()->exploreMovesFromContainersNotInObjective() = false;
    // allow only one move so that we want to ensure "task0" is the one
    // moved
    stage0Spec.solverSpec()->stopAfterMoves() = 1;
    stage0Spec.solverSpec()->moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
    solverSpec.stageSpecs()->push_back(stage0Spec);

    LocalSearchStageSpec stage1Spec;
    stage1Spec.begin() = 1;
    stage1Spec.end() = 3;
    stage1Spec.solverSpec()->exploreMovesFromContainersNotInObjective() = false;
    stage1Spec.solverSpec()->moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
    solverSpec.stageSpecs()->push_back(stage1Spec);

    solver_->addSolver(solverSpec);
  }

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  // expect zeroWeightTask to move into host1
  EXPECT_EQ(assignment[zeroWeightTask], "host1")
      << zeroWeightTask << " should be in host1, but is in host0";

  // expect other tasks to stay in their initial container
  for (auto& task : host0Tasks) {
    if (task != zeroWeightTask) {
      EXPECT_EQ(assignment[task], "host0");
    }
  }
}

TEST_P(ToFreeTest, ToFreeWithMinimizeOccupiedContainersFormula) {
  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host1", {"task1", "task2", "task3", "task14", "task15"}}, // 5 tasks
          {"host2", {"task4", "task5", "task16"}}, // 3 tasks
          {"host3",
           {"task6", "task7", "task8", "task9", "task10", "task11", "task12"}},
          // 7 tasks,
          {"host4", {"task13", "task17"}}, // 2 tasks
          {"sinkHost", {"task18"}},
      });

  const folly::F14FastMap<std::string, double> taskToImportance =
      folly::F14FastMap<std::string, double>{
          {"task6", 0.0},
          {"task7", 0.0},
          {"task8", 0.0},
          {"task9", 0.0},
          {"task10", 0.0},
          {"task11", 0.0},
          {"task12", 7.0},
          {"task13", 0.5},
          {"task17", 3.0},
          {"task18", 20.0}};
  solver_->addObjectDimension("task_importance", taskToImportance, 1.0);

  {
    // make all hosts as empty as possible, while prioritizing making hosts
    // empty
    ToFreeSpec toFreeSpec;
    toFreeSpec.name() = "drain hosts1..4";
    toFreeSpec.containers() = {"host1", "host2", "host3", "host4"};
    toFreeSpec.dimension() = "task_importance";
    toFreeSpec.formula() = ToFreeSpecFormula::MINIMIZE_OCCUPIED_CONTAINERS;

    solver_->addGoal(toFreeSpec);
  }

  LocalSearchSolverSpec solverSpec;
  solverSpec.stopAfterMoves() = 11;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  addSolver(solverSpec);

  auto solution = solver_->solve();

  // when using local search verify the exact order in which moves were made
  if (getSolverAlgoType() == SolverAlgoType::LOCALSEARCH) {
    std::unordered_map<std::string, int> finalObjectCount;
    std::unordered_map<std::string, int> moveOutCountFrom;
    for (auto& [task, destinationHost] : *solution.assignment()) {
      auto& sourceHost = solution.initialAssignment()->at(task);
      if (sourceHost != destinationHost) {
        moveOutCountFrom[sourceHost]++;
        EXPECT_EQ("sinkHost", destinationHost);
      }

      finalObjectCount[destinationHost]++;
    }

    // expect that host1 is empty and 5 objects moved from host1
    EXPECT_EQ(5, moveOutCountFrom["host1"]);
    EXPECT_EQ(0, finalObjectCount["host1"]);

    // expect that host2 is empty and 3 objects moved from host2
    EXPECT_EQ(3, moveOutCountFrom["host2"]);
    EXPECT_EQ(0, finalObjectCount["host2"]);

    // expect that host4 is empty
    EXPECT_EQ(2, moveOutCountFrom["host4"]);
    EXPECT_EQ(0, finalObjectCount["host4"]);

    // expect that 1 object moved from host3
    EXPECT_EQ(1, moveOutCountFrom["host3"]);
    EXPECT_EQ(6, finalObjectCount["host3"]);

    // verify the order in which moves were made; we expect moves from host2 to
    // be explored first since its util is closest to zero for the dimension
    // 'task_importance', followed by host1, host4, and finally host3
    const std::vector<std::string> expectedContainerExplorationOrder = {
        "host2",
        "host2",
        "host2",
        "host4",
        "host4",
        "host1",
        "host1",
        "host1",
        "host1",
        "host1",
        "host3"};

    std::vector<std::string> actualContainerExplorationOrder;
    for (auto& moveSummary : *solution.movesSummary()) {
      for (auto& move : *moveSummary.moves()) {
        actualContainerExplorationOrder.push_back(*move.srcContainer());
      }
    }

    EXPECT_EQ(
        expectedContainerExplorationOrder, actualContainerExplorationOrder);

    // initial objective value = 1/minPositiveDimensionValue *
    // afterUtil(containerToFree) - 0.5/objectDimensionSum^2 * um_{c \in
    // containerToFree} (absoluteUtil(c))^2
    // = 1.1/0.5 * (18.5) - 1.0/38.5^2 * (5^2 + 3^2 + 7^2 + 3.5^2)
    // 40.7 - (95.25 / 1482.25)
    EXPECT_NEAR(
        40.7 - (95.25 / 1482.25),
        *solution.initialGlobalObjective()->goals()->at(0).value(),
        1e-8);

    // final objective value is expected to be zero since hosts1..4 have zero
    // utilizations (the remaining objects in host3 all have zero dimension
    // value)
    EXPECT_NEAR(
        0.0, *solution.finalGlobalObjective()->goals()->at(0).value(), 1e-8);
  } else if (getSolverAlgoType() == SolverAlgoType::OPTIMAL) {
    // when using OptimalSolver, just verify the initial and final objective
    // values

    // initial objective value = 4.0 since all 4 hosts that are part of ToFree
    // have non-zero utilizations
    EXPECT_NEAR(
        4.0, *solution.initialGlobalObjective()->goals()->at(0).value(), 1e-8);

    // final objective value is expected to be zero since it possible to fully
    // drain all the hosts
    EXPECT_NEAR(
        0.0, *solution.finalGlobalObjective()->goals()->at(0).value(), 1e-8);
  }
}

TEST_P(ToFreeTest, NonAcceptingContainersPreventEvaluations) {
  // This test checks that when ToFreeSpec is used as a constraint and all
  // containers are part of the spec, then they are marked as nonAccepting and
  // no evaluations happen (since there are no valid destinations for moves).

  if (getSolverAlgoType() == SolverAlgoType::OPTIMAL) {
    // test is not relevant for Optimal solver
    return;
  }

  // Set up 2 hosts with 2 tasks each
  solver_->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
      });

  // Mark both hosts as needing to be freed (all containers in the spec)
  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "drain_all_hosts";
  toFreeSpec.containers() = {"host0", "host1"};
  solver_->addConstraint(toFreeSpec);

  // Add an affinity goal that would normally encourage moves
  AssignmentAffinitiesSpec assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.scope() = "host";
  assignmentAffinitiesSpec.affinities() = {
      makeAssignmentAffinity("task0", "host1", 1.0),
      makeAssignmentAffinity("task1", "host1", 1.0),
      makeAssignmentAffinity("task2", "host0", 1.0),
      makeAssignmentAffinity("task3", "host0", 1.0),
  };
  solver_->addGoal(assignmentAffinitiesSpec);

  addSolver();

  // Get a solution from Rebalancer.
  auto solution = solver_->solve();
  auto assignment = *solution.assignment();

  // Verify no moves were made and also that no evaluations happened.
  EXPECT_EQ(0, solution.movesSummary()->size());
  const auto& evalStats =
      solution.solverSummaries()->at(0).evalStats().as_const();
  ASSERT_FALSE(evalStats.has_value());
}

TEST_P(ToFreeTest, SingleFastDestinationsToExploreCurrentScopeItem) {
  if (getSolverAlgoType() == SolverAlgoType::OPTIMAL) {
    // test is not relevant for Optimal solver
    return;
  }

  // setup 20 hosts, only 5 task at host 0
  folly::F14FastMap<std::string, std::vector<std::string>> initialAssignment;

  for (const auto i : folly::irange(0, 20)) {
    initialAssignment.emplace(
        fmt::format("host{}", i), std::vector<std::string>{});
  }
  for (const auto i : folly::irange(0, 5)) {
    initialAssignment["host0"].push_back(fmt::format("task{}", i));
  }
  solver_->setAssignment(initialAssignment);

  // each scope item has 2 hosts
  folly::F14FastMap<std::string, std::string> hostToScopeItem;
  for (const auto i : folly::irange(0, 10)) {
    hostToScopeItem.emplace(
        fmt::format("host{}", 2 * i), fmt::format("scopeItem{}", i));
    hostToScopeItem.emplace(
        fmt::format("host{}", 2 * i + 1), fmt::format("scopeItem{}", i));
  }

  solver_->addScope("myScope", hostToScopeItem);

  ToFreeSpec toFreeSpec;
  toFreeSpec.name() = "empty host0";
  toFreeSpec.containers() = {"host0"};
  solver_->addConstraint(toFreeSpec);

  DestinationsToExploreOptions destinationsToExploreOptions;
  MoveToScopeItemsSpec moveToScopeItemsSpec;
  ScopeItemList scopeItemList;
  scopeItemList.exploreCurrentScopeItem() = true;
  scopeItemList.scopeName() = "myScope";
  moveToScopeItemsSpec.defaultScopeItems() = scopeItemList;
  destinationsToExploreOptions.moveToScopeItems() = moveToScopeItemsSpec;

  SingleFastMoveTypeSpec singleFastMoveTypeSpec;
  singleFastMoveTypeSpec.destinationsToExplore() = destinationsToExploreOptions;

  LocalSearchSolverSpec solverSpec;
  solverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(singleFastMoveTypeSpec));
  solver_->addSolver(solverSpec);

  auto solution = solver_->solve();

  // verify that all tasks are moved to host1
  // without destinations to explore, the tasks can be move to anywhere but host
  // 0
  for (const auto& [task, host] : *solution.assignment()) {
    EXPECT_EQ(host, "host1");
  }
}

} // namespace facebook::rebalancer::interface::tests
