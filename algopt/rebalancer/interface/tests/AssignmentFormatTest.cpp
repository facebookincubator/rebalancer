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

#include <cstdint>
#include <map>
#include <string>
#include <tuple>

using namespace facebook::rebalancer::interface;

namespace {

using CompactAssignmentInput =
    std::map<std::string, std::map<std::string, int64_t>>;

ContainerAssignment makeContainerAssignment(
    const CompactAssignmentInput& compactAssignmentInput) {
  ContainerAssignment compactAssignment;
  auto& objectsPerContainer = *compactAssignment.objectsPerContainer();
  for (const auto& [container, objectCounts] : compactAssignmentInput) {
    auto& compactCounts = objectsPerContainer[container];
    for (const auto& [objectName, count] : objectCounts) {
      compactCounts[objectName] = count;
    }
  }
  return compactAssignment;
}

const auto& getCompactAssignment(const AssignmentSolution& solution) {
  return *solution.compactAssignment()->objectsPerContainer();
}

const auto& getCompactAssignmentInitial(const AssignmentSolution& solution) {
  return *solution.compactAssignmentInitial()->objectsPerContainer();
}

std::unique_ptr<ProblemSolver> makeCompactAssignmentSolver(
    const CompactAssignmentInput& compactAssignmentInput) {
  auto solver = initializeTestProblemSolver();
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(compactAssignmentInput);
  return solver;
}

std::unique_ptr<ProblemSolver> makeCompactAssignmentSolver(
    const ContainerAssignment& compactAssignment) {
  auto solver = initializeTestProblemSolver();
  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(compactAssignment);
  return solver;
}

void addCpuBalanceGoalAndSolver(ProblemSolver& solver) {
  BalanceSpec balanceSpec;
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  balanceSpec.formula() = BalanceSpecFormula::LEGACY;
  balanceSpec.fixAverageToInitial() = true;
  solver.addGoal(balanceSpec);

  auto localSearchSpec = makeDefaultLocalSearchSolver();
  localSearchSpec.solveTime() = 10;
  solver.addSolver(localSearchSpec);
}

template <typename ContainerAssignmentLike>
int64_t getObjectCount(
    const ContainerAssignmentLike& compactAssignment,
    const std::string& container,
    const std::string& object) {
  const auto containerIt = compactAssignment.find(container);
  if (containerIt == compactAssignment.end()) {
    return 0;
  }

  const auto objectIt = containerIt->second.find(object);
  return objectIt == containerIt->second.end() ? 0 : objectIt->second;
}

} // namespace

class AssignmentFormatTest
    : public ::testing::TestWithParam<std::tuple<int, bool>> {
 protected:
  static int getThreadCount() {
    return std::get<0>(GetParam());
  }
  static bool getMoveGroups() {
    return std::get<1>(GetParam());
  }
};

INSTANTIATE_TEST_CASE_P(
    NumThreadsAndMoveGroups,
    AssignmentFormatTest,
    ::testing::Combine(testThreadCounts(), ::testing::Values(false, true)));

TEST_P(AssignmentFormatTest, Basic) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = getThreadCount()});

  auto moveGroups = getMoveGroups();

  solver->setObjectName("task");
  solver->setContainerName("host");
  solver->setAssignment(
      std::vector<std::pair<std::string, std::vector<std::string>>>{
          {"host0", {"task0", "task1"}},
          {"host1", {"task2", "task3"}},
      });
  solver->addPartition(
      "job",
      std::map<std::string, std::vector<std::string>>{
          {"job0", {"task0", "task1"}},
          {"job1", {"task2", "task3"}},
      });

  if (moveGroups) {
    MoveGroupSpec spec;
    spec.partitionName() = "job";
    solver->addConstraint(spec);
  }

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution = solver->solve();

  const std::map<std::string, std::string> initialAssignment = {
      {"task0", "host0"},
      {"task1", "host0"},
      {"task2", "host1"},
      {"task3", "host1"},
  };
  EXPECT_EQ(initialAssignment, toOrderedMap(*solution.initialAssignment()));

  const std::map<std::string, std::vector<std::string>> finalAssignment = {
      {"host0", {"task0", "task1"}},
      {"host1", {"task2", "task3"}},
  };
  EXPECT_EQ(initialAssignment, toOrderedMap(*solution.assignment()));
}

TEST(AssignmentFormatContainerAssignmentTest, Basic) {
  auto solver = makeCompactAssignmentSolver(
      CompactAssignmentInput{
          {"host0", {{"taskA", 2}}},
          {"host1", {}},
      });
  solver->addObjectDimension(
      "cpu", std::map<std::string, double>{{"taskA", 10}});
  solver->addContainerDimension(
      "cpu", std::map<std::string, double>{{"host0", 100}, {"host1", 100}});
  addCpuBalanceGoalAndSolver(*solver);

  const auto solution = solver->solve();
  const auto& initialCompactAssignment = getCompactAssignmentInitial(solution);
  EXPECT_EQ(initialCompactAssignment.at("host0").at("taskA"), 2);

  const auto& compactAssignment = getCompactAssignment(solution);
  EXPECT_EQ(getObjectCount(compactAssignment, "host0", "taskA"), 1);
  EXPECT_EQ(getObjectCount(compactAssignment, "host1", "taskA"), 1);
  EXPECT_TRUE(solution.assignment()->empty());
  EXPECT_TRUE(solution.initialAssignment()->empty());
}

TEST(
    AssignmentFormatContainerAssignmentTest,
    ExplicitContainerAssignmentOverload) {
  auto solver = makeCompactAssignmentSolver(makeContainerAssignment(
      CompactAssignmentInput{
          {"host0", {{"taskA", 2}}},
          {"host1", {}},
      }));
  solver->addObjectDimension(
      "cpu", std::map<std::string, double>{{"taskA", 10}});
  solver->addContainerDimension(
      "cpu", std::map<std::string, double>{{"host0", 100}, {"host1", 100}});
  addCpuBalanceGoalAndSolver(*solver);

  const auto solution = solver->solve();
  const auto& compactAssignment = getCompactAssignment(solution);
  EXPECT_EQ(getObjectCount(compactAssignment, "host0", "taskA"), 1);
  EXPECT_EQ(getObjectCount(compactAssignment, "host1", "taskA"), 1);
}

TEST(
    AssignmentFormatContainerAssignmentTest,
    AvoidMovingSpecUsesOriginalObjectNames) {
  auto solver = makeCompactAssignmentSolver(
      CompactAssignmentInput{
          {"host0", {{"taskA", 2}}},
          {"host1", {}},
      });
  solver->addObjectDimension(
      "cpu", std::map<std::string, double>{{"taskA", 10}});
  solver->addContainerDimension(
      "cpu", std::map<std::string, double>{{"host0", 100}, {"host1", 100}});

  AvoidMovingSpec avoidMovingSpec;
  avoidMovingSpec.name() = "avoid-taskA";
  avoidMovingSpec.objects() = {"taskA"};
  solver->addConstraint(avoidMovingSpec);
  addCpuBalanceGoalAndSolver(*solver);

  const auto solution = solver->solve();
  const auto& compactAssignment = getCompactAssignment(solution);
  EXPECT_EQ(getObjectCount(compactAssignment, "host0", "taskA"), 2);
  EXPECT_EQ(getObjectCount(compactAssignment, "host1", "taskA"), 0);
}

TEST(
    AssignmentFormatContainerAssignmentTest,
    CompactAssignmentAcceptsGroupsAndGoalsUsingOriginalObjectNames) {
  auto solver = makeCompactAssignmentSolver(
      CompactAssignmentInput{
          {"host0", {{"taskA", 2}, {"taskB", 1}}},
          {"host1", {}},
      });
  solver->addPartition(
      "job",
      std::map<std::string, std::vector<std::string>>{
          {"job0", {"taskA", "taskB"}},
      });
  MoveGroupSpec moveGroupSpec;
  moveGroupSpec.partitionName() = "job";
  solver->addConstraint(moveGroupSpec);

  AssignmentAffinitiesSpec affinitySpec;
  affinitySpec.name() = "prefer-host1";
  affinitySpec.scope() = "host";
  affinitySpec.affinities() = {makeAssignmentAffinity("taskA", "host1", 1.0)};
  solver->addGoal(affinitySpec);
  solver->addSolver(makeDefaultLocalSearchSolver());

  const auto solution = solver->solve();
  const auto& initialCompactAssignment = getCompactAssignmentInitial(solution);
  const auto& compactAssignment = getCompactAssignment(solution);
  EXPECT_EQ(initialCompactAssignment, compactAssignment);
}

TEST(
    AssignmentFormatContainerAssignmentTest,
    RestrictedObjectSpecsHandleCompactAssignment) {
  {
    auto solver = makeCompactAssignmentSolver(
        CompactAssignmentInput{
            {"host0", {{"taskA", 2}, {"taskB", 1}}},
            {"host1", {}},
        });

    ExclusiveObjectsSpec spec;
    spec.name() = "exclusive";
    spec.scope() = "host";
    ObjectPair pair;
    pair.object1() = "taskA";
    pair.object2() = "taskB";
    spec.pairs() = {pair};

    EXPECT_THROW(solver->addConstraint(spec), std::invalid_argument);
  }

  {
    auto solver = makeCompactAssignmentSolver(
        CompactAssignmentInput{
            {"host0", {{"taskA", 1}, {"taskB", 1}}},
            {"host1", {}},
        });

    ExclusiveObjectsSpec spec;
    spec.name() = "exclusive";
    spec.scope() = "host";
    ObjectPair pair;
    pair.object1() = "taskA";
    pair.object2() = "taskB";
    spec.pairs() = {pair};

    EXPECT_NO_THROW(solver->addConstraint(spec));
  }

  {
    auto solver = makeCompactAssignmentSolver(
        CompactAssignmentInput{
            {"host0", {{"taskA", 2}, {"taskB", 1}}},
            {"host1", {}},
        });
    solver->addObjectDimension(
        "cpu", std::map<std::string, double>{{"taskA", 10}, {"taskB", 20}});

    WorkingSetSpec spec;
    spec.name() = "working-set";
    spec.scope() = "host";
    spec.dimension() = "cpu";

    WorkingUnit unit;
    unit.endpoints() = {"taskA", "taskB"};
    unit.weight() = 1.0;
    spec.workingUnits() = {unit};

    EXPECT_THROW(solver->addGoal(spec), std::invalid_argument);
  }
}

TEST(
    AssignmentFormatContainerAssignmentTest,
    UnsupportedCompactSpecsThrowBeforeProblemValidation) {
  auto solver = makeCompactAssignmentSolver(
      CompactAssignmentInput{
          {"host0", {{"taskA", 1}, {"taskB", 1}}},
          {"host1", {}},
      });

  FlowSpec spec;
  spec.scope() = "host";
  spec.dimension() = "cpu";
  ObjectPair pair;
  pair.object1() = "taskA";
  pair.object2() = "taskB";
  spec.pairs() = {pair};

  EXPECT_THAT(
      [&] { solver->addGoal(spec); },
      testing::ThrowsMessage<std::invalid_argument>(testing::HasSubstr(
          "FlowSpec is not supported with compact assignments")));
}
