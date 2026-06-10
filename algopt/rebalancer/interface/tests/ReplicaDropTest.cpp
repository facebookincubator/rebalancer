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

using namespace facebook::rebalancer::interface;

class ReplicaDropTest : public ::testing::TestWithParam<int> {};

INSTANTIATE_TEST_CASE_P(NumThreads, ReplicaDropTest, testThreadCounts());

TEST_P(ReplicaDropTest, Basic) {
  // This example replicates a scenario where different jobs need to be resized
  // to contain a specific number of tasks, which may be lower than the initial
  // number, and in that case Rebalancer must select the best tasks to drop
  // within each job.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});

  // Set entity names and initial assignment.
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"job0/task0", "job1/task1", "job2/task2"}},
          {"host1", {"job0/task1", "job2/task1", "job3/task0"}},
          {"host2", {"job0/task2", "job1/task0", "job2/task0"}},
          {"host3", {"job0/task3", "job3/task1"}},
          {"unassigned", {}},
      });

  // Partition tasks into jobs.
  solver->addPartition(
      "job",
      folly::F14FastMap<std::string, std::vector<std::string>>{
          {"job0", {"job0/task0", "job0/task1", "job0/task2", "job0/task3"}},
          {"job1", {"job1/task0", "job1/task1"}},
          {"job2", {"job2/task0", "job2/task1", "job2/task2"}},
          {"job3", {"job3/task0", "job3/task1"}},
      });

  // Create a scope of assignable hosts, excluding the special container
  // "unassigned".
  solver->addScope(
      "assigned",
      std::map<std::string, std::string>{
          {"host0", "assigned"},
          {"host1", "assigned"},
          {"host2", "assigned"},
          {"host3", "assigned"},
      });

  // Specify a different lag amount for each task in a job.
  solver->addObjectDimension(
      "lag",
      std::map<std::string, double>{
          {"job0/task0", 0.0},
          {"job0/task1", 0.01},
          {"job0/task2", 0.03},
          {"job0/task3", 0.02},
          {"job1/task0", 0.06},
          {"job1/task1", 0.0},
          {"job2/task0", 0.07},
          {"job2/task1", 0.09},
          {"job2/task2", 0.0},
          {"job3/task0", 0.0},
          {"job3/task1", 0.09},
      });

  {
    // Require a specific number of tasks per job to be assigned.
    GroupCountSpec spec;
    spec.scope() = "assigned";
    spec.dimension() = "task_count";
    spec.partitionName() = "job";
    spec.bound() = GroupCountSpecBound::EXACT;
    spec.limit()->groupLimits()->emplace("job0", 2);
    spec.limit()->groupLimits()->emplace("job1", 1);
    spec.limit()->groupLimits()->emplace("job2", 2);
    spec.limit()->groupLimits()->emplace("job3", 2);
    solver->addConstraint(spec);
  }

  {
    // Minimize the aggregate lag of all assigned tasks.
    CapacitySpec spec;
    spec.scope() = "assigned";
    spec.dimension() = "lag";
    spec.limit()->globalLimit() = 0;
    solver->addGoal(spec);
  }

  {
    // Use the replica drop move type.
    ReplicaDropMoveTypeSpec moveTypeSpec;
    // This partition represents replica groups. The REPLICA_DROP move type will
    // evaluate moving each replica in the same group and pick the best before
    // settling on a move.
    moveTypeSpec.replicaDropPartition() = "job";
    // The REPLICA_DROP move type will attempt to move objects to containers
    // outside of this scope.
    moveTypeSpec.replicaDropScope() = "assigned";
    LocalSearchSolverSpec spec;
    spec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(std::move(moveTypeSpec)));
    solver->addSolver(spec);
  }

  {
    // Make one of the replicas unmovable.
    AvoidMovingSpec spec;
    spec.objects() = {"job0/task0"};
    solver->addConstraint(spec);
  }

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  // Expect the most laggy tasks to be dropped for each job.
  EXPECT_EQ("host0", assignment.at("job0/task0"));
  EXPECT_EQ("host1", assignment.at("job0/task1"));
  EXPECT_EQ("unassigned", assignment.at("job0/task2"));
  EXPECT_EQ("unassigned", assignment.at("job0/task3"));
  EXPECT_EQ("unassigned", assignment.at("job1/task0"));
  EXPECT_EQ("host0", assignment.at("job1/task1"));
  EXPECT_EQ("host2", assignment.at("job2/task0"));
  EXPECT_EQ("unassigned", assignment.at("job2/task1"));
  EXPECT_EQ("host0", assignment.at("job2/task2"));
  EXPECT_EQ("host1", assignment.at("job3/task0"));
  EXPECT_EQ("host3", assignment.at("job3/task1"));
}

TEST_P(
    ReplicaDropTest,
    ExcludeFixedReplicasAndNonAcceptingContainersWhileEvaluating) {
  //  This test is just to make sure that while evaluating moves in
  //  ReplicaDropMoveType moves that involve fixed replicas and non-accepting
  //  containers are not evaluated.
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});

  // Set entity names and initial assignment.
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"job0/task0", "job0/task1", "job0/task2"}},
          {"source", {}},
          {"sink", {}},
      });

  solver->addPartition(
      "job",
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"job0", {"job0/task0", "job0/task1", "job0/task2"}}});
  solver->addScope(
      "assigned", std::map<std::string, std::string>{{"host0", "assigned"}});

  {
    // No task of job0 should be in "assigned" scope
    GroupCountSpec spec;
    spec.scope() = "assigned";
    spec.dimension() = "task_count";
    spec.partitionName() = "job";
    spec.bound() = GroupCountSpecBound::MAX;
    spec.limit()->globalLimit() = 0;
    solver->addConstraint(spec);
  }

  {
    // Make the source container as not accepting
    NonAcceptingSpec spec;
    spec.scope() = "host";
    spec.items() = {"source"};
    solver->addConstraint(spec);
  }

  {
    // Make one of the replicas unmovable.
    AvoidMovingSpec spec;
    spec.objects() = {"job0/task0"};
    solver->addConstraint(spec);
  }

  {
    // Use the replica drop move type.
    ReplicaDropMoveTypeSpec moveTypeSpec;
    // This partition represents replica groups. The REPLICA_DROP move type will
    // evaluate moving each replica in the same group and pick the best before
    // settling on a move.
    moveTypeSpec.replicaDropPartition() = "job";
    // The REPLICA_DROP move type will attempt to move objects to containers
    // outside of this scope.
    moveTypeSpec.replicaDropScope() = "assigned";
    LocalSearchSolverSpec spec;
    spec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(std::move(moveTypeSpec)));
    solver->addSolver(spec);
  }

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto& assignment = *solution.assignment();

  EXPECT_EQ("host0", assignment.at("job0/task0"));
  EXPECT_EQ("sink", assignment.at("job0/task1"));
  EXPECT_EQ("sink", assignment.at("job0/task2"));

  // we expect (2 + 1 = 3) evaluations in total; the first iteration of local
  // search will evaluate moves where each of job0/task1 and job0/task2 are
  // moved to sink (note that job0/task0 cannot move); the next iteration we
  // expect only one move evaluation as one of them is already in sink
  auto evalStats = solution.solverSummaries()->at(0).evalStats();
  if (evalStats.has_value()) {
    EXPECT_EQ(3, evalStats->numEvals());
  } else {
    throw std::runtime_error(
        "Expected solution.solverSummaries()->at(0).evalStats() to be populated");
  }
}
