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
#include "algopt/rebalancer/interface/Constants.h"
#include "algopt/rebalancer/interface/tests/utils.h"
#include "algopt/rebalancer/tests/SolverTestUtils.h"

#include <folly/container/irange.h>
#include <gtest/gtest.h>

using namespace std;
namespace facebook::rebalancer::interface::tests {

class EndToEndTest : public ::testing::TestWithParam<int> {
 protected:
  // Make a simple problem with 2 hosts and 3 shards, their cpu & network
  // h1 (cpu: 100, network: 1000): s1 (c: 50, n: 100), s2(c:10, n: 200),
  // s3(c:40, n:200) h2 (cpu: 100, network: 1000): goal: we want s1 to have
  // affinity to h2 and then balance on (importance: 200) cpu, then network
  // (importance: 5) further we ensure all constraints are easily satisfied in
  // all configurations for simplicty
  static std::unique_ptr<ProblemSolver> makeSimpleProblem(
      int executorThreadCount) {
    auto solver = initializeTestProblemSolver(
        {.executorThreadCount = executorThreadCount});

    // Set the object and container names
    solver->setObjectName("shard");
    solver->setContainerName("host");

    // Set the initial assignment
    const map<string, vector<string>> initial_assignment = {
        {"h1", {"s1", "s2", "s3"}},
        {"h2", {}},
    };
    solver->setAssignment(initial_assignment);

    // Define cpu dimension
    const map<string, double> shard_cpu = {
        {"s1", 50},
        {"s2", 10},
        {"s3", 40},
    };
    const map<string, double> host_cpu = {
        {"h1", 100},
        {"h2", 100},
    };
    solver->addObjectDimension("cpu", shard_cpu);
    solver->addContainerDimension("cpu", host_cpu);

    // Add capacity constraint on host cpu
    CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "cpu";
    capacitySpec.name() = "do not exceed cpu";
    solver->addConstraint(capacitySpec);

    // Create rack scope
    const map<string, string> host_to_rack = {{"h1", "r1"}, {"h2", "r2"}};
    solver->addScope("rack", host_to_rack);

    // Define network dimension
    const map<string, double> shard_network = {
        {"s1", 1000},
        {"s2", 2000},
        {"s3", 2000},
    };
    const map<string, double> rack_network = {
        {"r1", 10000},
        {"r2", 10000},
    };
    solver->addObjectDimension("network", shard_network);
    solver->addScopeDimension("network", "rack", rack_network);

    // Add capacity constraint on rack network
    CapacitySpec capacitySpec2;
    capacitySpec2.scope() = "rack";
    capacitySpec2.dimension() = "network";
    capacitySpec2.name() = "do not exceed network";

    solver->addConstraint(capacitySpec2);

    // Populate move reasons
    solver->enableMoveStats();

    return solver;
  }
};

constexpr double kEps = 1e-9;

INSTANTIATE_TEST_CASE_P(NumThreads, EndToEndTest, testThreadCounts());

static void visitHierarchicalProfileNodes(
    HierarchicalProfileNode& curNode,
    std::vector<HierarchicalProfileNode>& visited) {
  visited.push_back(curNode);
  for (auto& child : *curNode.children()) {
    visitHierarchicalProfileNodes(child, visited);
  }
}

TEST_P(EndToEndTest, ObjectiveTuple) {
  // Use an objective tuple to capture primary and secondary priorities
  // (separated by specifying a 'goalboundary')
  {
    auto solver = makeSimpleProblem(GetParam());
    // balancing cpu is very important
    BalanceSpec balanceSpec;
    balanceSpec.scope() = "host";
    balanceSpec.dimension() = "cpu";
    balanceSpec.name() = "balance cpu";
    solver->addGoal(balanceSpec, 100);

    // this is our most important goal
    solver->addGoalBoundary();

    // as a secondary goal, lets also balance network
    BalanceSpec balanceSpec2;
    balanceSpec2.scope() = "host";
    balanceSpec2.dimension() = "network";
    balanceSpec2.name() = "balance network ";
    solver->addGoal(balanceSpec2, 50);

    AssignmentAffinitiesSpec assignmentAffinitiesSpec;
    assignmentAffinitiesSpec.name() = "make solution stable";
    assignmentAffinitiesSpec.affinities() = {
        makeAssignmentAffinity("s2", "h2", 0.8)};
    solver->addGoal(assignmentAffinitiesSpec, 10);

    solver->addSolver(makeDefaultLocalSearchSolver());

    // Run the solver
    auto solution = solver->solve();

    // Expect assignment
    const map<string, string> expected_assignment = {
        {"s1", "h1"}, {"s2", "h2"}, {"s3", "h2"}};
    EXPECT_EQ(expected_assignment, toOrderedMap(*solution.assignment()));
  }

  // Use a single objective and ad-hoc weights
  {
    auto solver = makeSimpleProblem(GetParam());
    // balancing cpu is very important
    BalanceSpec balanceSpec3;
    balanceSpec3.scope() = "host";
    balanceSpec3.dimension() = "cpu";
    balanceSpec3.name() = "balance cpu";
    solver->addGoal(balanceSpec3, 100);

    // lets also balance network
    BalanceSpec balanceSpec4;
    balanceSpec4.scope() = "host";
    balanceSpec4.dimension() = "network";
    balanceSpec4.name() = "balance network ";
    solver->addGoal(balanceSpec4, 50);

    AssignmentAffinitiesSpec assignmentAffinitiesSpec;
    assignmentAffinitiesSpec.name() = "make solution stable";
    assignmentAffinitiesSpec.affinities() = {
        makeAssignmentAffinity("s2", "h2", 0.8)};
    solver->addGoal(assignmentAffinitiesSpec, 10);

    solver->addSolver(makeDefaultLocalSearchSolver());

    // Run the solver
    auto solution = solver->solve();

    // See that network goal now overrides cpu balance since weights & problem
    // state are not in sync, we ended up leaving some cpu balance on table to
    // make balance network better, if problem state is different we may need to
    // re-tweak weights
    // (instead using tuples makes the relative priority explicit)
    const map<string, string> expected_assignment = {
        {"s1", "h2"}, {"s2", "h2"}, {"s3", "h1"}};
    EXPECT_EQ(expected_assignment, toOrderedMap(*solution.assignment()));
  }
}

TEST_P(EndToEndTest, Example) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});

  // set DBG2 because the hierarchical profiler will not print all the expected
  // nodes if that's not the case (e.g., if info/dbg1, it only prints the nodes
  // that takes moe than 5% of the total time)
  solver->setLogLevel("DBG2");

  // Set the object and container names
  solver->setObjectName("shard");
  solver->setContainerName("host");

  // Set the initial assignment
  const map<string, vector<string>> initial_assignment = {
      {"h1", {"s1", "s2", "s3", "s4", "s5"}},
      {"h2", {}},
      {"h3", {}},
      {"h4", {}},
  };
  solver->setAssignment(initial_assignment);
  solver->publishEquivalenceSetInfo();

  // Define cpu dimension
  // Equivalence sets before: {s1, s2, s3, s4, s5}
  // splits equivalence sets as {s1, s2, s3} and {s4, s5}
  const map<string, double> shard_cpu = {
      {"s1", 40},
      {"s2", 40},
      {"s3", 40},
      {"s4", 20},
      {"s5", 20},
  };
  const map<string, double> host_cpu = {
      {"h1", 100},
      {"h2", 100},
      {"h3", 100},
      {"h4", 100},
  };
  solver->addObjectDimension("cpu", shard_cpu);
  solver->addContainerDimension("cpu", host_cpu);

  // Add capacity constraint and balance goal on host cpu
  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "cpu";
  capacitySpec.name() = "do not exceed cpu";
  solver->addConstraint(capacitySpec);

  BalanceSpec balanceSpec;
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  balanceSpec.name() = "balance cpu";
  solver->addGoal(balanceSpec);

  // Create rack scope
  const map<string, string> host_to_rack = {
      {"h1", "r1"}, {"h2", "r1"}, {"h3", "r2"}, {"h4", "r2"}};
  solver->addScope("rack", host_to_rack);

  // Define network dimension
  // Equivalence sets before: {s1, s2, s3}, {s4, s5}
  // splits equivalence sets as {s1, s2} {s3} and {s4, s5}
  const map<string, double> shard_network = {
      {"s1", 400},
      {"s2", 400},
      {"s3", 200},
      {"s4", 400},
      {"s5", 400},
  };
  const map<string, double> rack_network = {
      {"r1", 1000},
      {"r2", 1000},
  };
  solver->addObjectDimension("network", shard_network);
  solver->addScopeDimension("network", "rack", rack_network);

  // Add capacity constraint on rack network
  CapacitySpec capacitySpec2;
  capacitySpec2.scope() = "rack";
  capacitySpec2.dimension() = "network";
  capacitySpec2.name() = "do not exceed network";
  solver->addConstraint(capacitySpec2);

  // Make the solution stable
  AssignmentAffinitiesSpec assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.name() = "make solution stable";
  // Equivalence sets before: {s1, s2} {s3} and {s4, s5}
  // splits equivalence sets as {s1} {s2} {s3} and {s4, s5}
  assignmentAffinitiesSpec.affinities() = {
      makeAssignmentAffinity("s1", "h3", 0.4),
      makeAssignmentAffinity("s2", "h4", 0.2),
      makeAssignmentAffinity("s3", "h2", 0.1)};
  solver->addGoal(assignmentAffinitiesSpec, 1e-6);

  // Populate move reasons
  solver->enableMoveStats();

  solver->addSolver(makeDefaultLocalSearchSolver());

  // Run the solver
  auto solution = solver->solve();

  // Verify Equivalence sets info
  {
    auto& equivSetInfo = *solution.equivalenceSetInfo();
    const int numEquivSets = equivSetInfo.equivalenceSets()->size();
    folly::F14FastMap<std::string, std::set<std::string>> equivObjects;
    for (auto& equivSet : *equivSetInfo.equivalenceSets()) {
      EXPECT_TRUE(equivSet.name()->starts_with(kEquivSetNamePrefix.data()));
      equivObjects[*equivSet.name()].insert(
          equivSet.objectNames()->begin(), equivSet.objectNames()->end());
    }
    // s1,s2,s3 {s4,s5} are all distinct because of their cpu and network
    // dimensions as well as assignment affinity
    const std::set<std::string> singletons = {"s1", "s2", "s3"};
    const std::set<std::string> equivPair = {"s4", "s5"};
    EXPECT_EQ(4, numEquivSets);
    EXPECT_EQ(numEquivSets, equivObjects.size());
    for (auto& [_, objects] : equivObjects) {
      if (objects.size() == 1) {
        EXPECT_EQ(1, objects.size());
        EXPECT_TRUE(singletons.contains(*objects.begin()));
      } else {
        EXPECT_EQ(2, objects.size());
        EXPECT_EQ(equivPair, objects);
      }
    }
  }

  // Expect assignment
  const map<string, string> expected_assignment = {
      {"s1", "h3"}, {"s2", "h4"}, {"s3", "h2"}, {"s4", "h1"}, {"s5", "h1"}};
  EXPECT_EQ(expected_assignment, toOrderedMap(*solution.assignment()));

  // Expect initial objective
  EXPECT_NEAR(20140.3, *solution.initialObjective()->value(), kEps);
  ASSERT_EQ(4, solution.initialObjective()->objs()->size());

  const auto& initialObjectives = *solution.initialObjective()->objs();
  {
    const auto& summary = getObjectiveSummary(initialObjectives, "balance cpu");
    EXPECT_EQ(1, *summary.weight());
    EXPECT_NEAR(0.3, *summary.value(), kEps);
  }
  {
    const auto& summary =
        getObjectiveSummary(initialObjectives, "make solution stable");
    EXPECT_EQ(1e-6, *summary.weight());
    EXPECT_NEAR(0, *summary.value(), kEps);
  }
  {
    const auto& summary =
        getObjectiveSummary(initialObjectives, "do not exceed cpu");
    EXPECT_EQ(1, *summary.weight());
    EXPECT_NEAR(10060, *summary.value(), kEps);
  }
  {
    const auto& summary =
        getObjectiveSummary(initialObjectives, "do not exceed network");
    EXPECT_EQ(1, *summary.weight());
    EXPECT_NEAR(10080, *summary.value(), kEps);
  }

  // Expect initial constraint
  EXPECT_NEAR(1.4, *solution.initialConstraint()->brokenVal(), kEps);
  EXPECT_EQ(2, *solution.initialConstraint()->brokenCount());
  ASSERT_EQ(2, solution.initialConstraint()->constraints()->size());

  const auto& initialConstraints = *solution.initialConstraint()->constraints();

  {
    const auto& summary =
        getConstraintSummary(initialConstraints, "do not exceed cpu");
    EXPECT_NEAR(0.6, *summary.value(), kEps);
  }
  {
    const auto& summary =
        getConstraintSummary(initialConstraints, "do not exceed network");
    EXPECT_NEAR(0.8, *summary.value(), kEps);
  }

  // Expect final objective
  EXPECT_NEAR(-7e-7, *solution.finalObjective()->value(), kEps);
  ASSERT_EQ(4, solution.finalObjective()->objs()->size());

  const auto& finalObjectives = *solution.finalObjective()->objs();

  {
    const auto& summary = getObjectiveSummary(finalObjectives, "balance cpu");
    EXPECT_EQ(1, *summary.weight());
    EXPECT_NEAR(0, *summary.value(), kEps);
  }
  {
    const auto& summary =
        getObjectiveSummary(finalObjectives, "make solution stable");
    EXPECT_EQ(1e-6, *summary.weight());
    EXPECT_NEAR(-7e-7, *summary.value(), kEps);
  }
  {
    const auto& summary =
        getObjectiveSummary(finalObjectives, "do not exceed cpu");
    EXPECT_EQ(1, *summary.weight());
    EXPECT_NEAR(0, *summary.value(), kEps);
  }
  {
    const auto& summary =
        getObjectiveSummary(finalObjectives, "do not exceed network");
    EXPECT_EQ(1, *summary.weight());
    EXPECT_NEAR(0, *summary.value(), kEps);
  }

  // Expect final constraint
  EXPECT_NEAR(0, *solution.finalConstraint()->brokenVal(), kEps);
  EXPECT_EQ(0, *solution.finalConstraint()->brokenCount());
  ASSERT_EQ(2, solution.finalConstraint()->constraints()->size());

  const auto& finalConstraints = *solution.finalConstraint()->constraints();

  {
    const auto& summary =
        getConstraintSummary(finalConstraints, "do not exceed cpu");
    EXPECT_NEAR(0, *summary.value(), kEps);
  }
  {
    const auto& summary =
        getConstraintSummary(finalConstraints, "do not exceed network");
    EXPECT_NEAR(0, *summary.value(), kEps);
  }

  // Expect moves summary
  EXPECT_TRUE(solution.movesSummary().has_value());
  EXPECT_EQ(3, solution.movesSummary()->size());
  ASSERT_EQ(1, solution.movesSummary()->at(0).moves()->size());
  EXPECT_EQ("s1", *solution.movesSummary()->at(0).moves()->at(0).object());
  EXPECT_EQ(
      "h1", *solution.movesSummary()->at(0).moves()->at(0).srcContainer());
  EXPECT_EQ(
      "h3", *solution.movesSummary()->at(0).moves()->at(0).dstContainer());

  EXPECT_NEAR(
      0.1,
      *solution.movesSummary()->at(0).objectives()->at("balance cpu").change(),
      kEps);

  EXPECT_NEAR(
      4e-7,
      *solution.movesSummary()
           ->at(0)
           .objectives()
           ->at("make solution stable")
           .change(),
      kEps);
  EXPECT_NEAR(
      40,
      *solution.movesSummary()
           ->at(0)
           .objectives()
           ->at("do not exceed cpu")
           .change(),
      kEps);
  EXPECT_NEAR(
      40,
      *solution.movesSummary()
           ->at(0)
           .objectives()
           ->at("do not exceed network")
           .change(),
      kEps);
  ASSERT_EQ(1, solution.movesSummary()->at(1).moves()->size());
  EXPECT_EQ("s2", *solution.movesSummary()->at(1).moves()->at(0).object());
  EXPECT_EQ(
      "h1", *solution.movesSummary()->at(1).moves()->at(0).srcContainer());
  EXPECT_EQ(
      "h4", *solution.movesSummary()->at(1).moves()->at(0).dstContainer());

  EXPECT_NEAR(
      0.1,
      *solution.movesSummary()->at(1).objectives()->at("balance cpu").change(),
      kEps);
  EXPECT_NEAR(
      2e-7,
      *solution.movesSummary()
           ->at(1)
           .objectives()
           ->at("make solution stable")
           .change(),
      kEps);
  EXPECT_NEAR(
      10020,
      *solution.movesSummary()
           ->at(1)
           .objectives()
           ->at("do not exceed cpu")
           .change(),
      kEps);
  EXPECT_NEAR(
      10040,
      *solution.movesSummary()
           ->at(1)
           .objectives()
           ->at("do not exceed network")
           .change(),
      kEps);
  ASSERT_EQ(1, solution.movesSummary()->at(2).moves()->size());
  EXPECT_EQ("s3", *solution.movesSummary()->at(2).moves()->at(0).object());
  EXPECT_EQ(
      "h1", *solution.movesSummary()->at(2).moves()->at(0).srcContainer());
  EXPECT_EQ(
      "h2", *solution.movesSummary()->at(2).moves()->at(0).dstContainer());

  EXPECT_NEAR(
      0.1,
      *solution.movesSummary()->at(2).objectives()->at("balance cpu").change(),
      kEps);
  EXPECT_NEAR(
      1e-7,
      *solution.movesSummary()
           ->at(2)
           .objectives()
           ->at("make solution stable")
           .change(),
      kEps);

  // Expect solvers summary
  ASSERT_EQ(1, solution.solverSummaries()->size());
  EXPECT_EQ(EndReason::OPTIMAL, *solution.solverSummaries()->at(0).endReason());

  // Expect final evaluation summary: no moves are performed because local
  // search figures the objective is optimal already in this instance.
  EXPECT_TRUE(solution.finalEvaluationSummary().has_value());
  auto& globalStats = *solution.finalEvaluationSummary()->globalStats();
  EXPECT_EQ(0, *globalStats.totalCount());
  EXPECT_EQ(0, globalStats.nonAcceptingContainersCount().value());
  EXPECT_EQ(0, globalStats.fixedContainersCount().value());
  EXPECT_EQ(0, globalStats.fixedObjectsCount().value());
  ASSERT_EQ(
      0, solution.finalEvaluationSummary()->sourceContainerStats()->size());

  // Expect non-zero materialization and solve times.
  EXPECT_LT(0.0, *solution.problemProfile()->materializationSec());
  EXPECT_LT(0.0, *solution.problemProfile()->solveSec());

  // Verify local search profiles.
  {
    auto& profiles = *solution.problemProfile()->localSearchProfiles();
    EXPECT_EQ(1, profiles.size());

    auto& profile = profiles.at(0);

    auto& names = *profile.moveTypeNames();
    EXPECT_EQ(4, names.size());
    EXPECT_EQ("SINGLE", names.at(0));
    EXPECT_EQ("SWAP", names.at(1));
    EXPECT_EQ("TRIPLE_LOOP", names.at(2));
    EXPECT_EQ("KL_SEARCH", names.at(3));

    auto& events = *profile.moveTypeEvents();
    EXPECT_EQ(3, events.size());

    EXPECT_EQ(0, *events.at(0).moveTypeIndex());
    EXPECT_LT(0.0, *events.at(0).duration());
    EXPECT_NEAR(20140.3, *events.at(0).initialValue(), kEps);
    EXPECT_NEAR(20060.1999996, *events.at(0).finalValue(), kEps);

    EXPECT_EQ(0, *events.at(1).moveTypeIndex());
    EXPECT_LT(0.0, *events.at(1).duration());
    EXPECT_NEAR(20060.1999996, *events.at(1).initialValue(), kEps);
    EXPECT_NEAR(0.0999994, *events.at(1).finalValue(), kEps);

    EXPECT_EQ(0, *events.at(2).moveTypeIndex());
    EXPECT_LT(0.0, *events.at(2).duration());
    EXPECT_NEAR(0.0999994, *events.at(2).initialValue(), kEps);
    EXPECT_NEAR(-7e-7, *events.at(2).finalValue(), kEps);
  }
#ifndef REBALANCER_OSS_BUILD
  {
    // Verify Hierarchical profile
    // In OSS builds, hierarchicalProfileRoot is not populated (requires
    // ScubaLog::serializeEventHierarchyTree which is internal-only).
    auto& root = *solution.problemProfile()->hierarchicalProfileRoot();
    std::vector<HierarchicalProfileNode> profiledEvents;
    visitHierarchicalProfileNodes(root, profiledEvents);
    // We expect to at least profile following events, the test will need to
    // be updated if the names change
    const folly::F14FastSet<std::string> expectedEvents = {
        "CoreSolver::Solve", "Materialization", "LocalSearch::solve"};
    EXPECT_GE(profiledEvents.size(), expectedEvents.size());
    int numSeenEvents = 0;
    for (auto node : profiledEvents) {
      if (expectedEvents.contains(*node.eventName())) {
        numSeenEvents++;
        // just check that the value is > 0
        EXPECT_GT(node.duration(), 0);
      }
    }
    EXPECT_EQ(expectedEvents.size(), numSeenEvents);
  }
#endif
}

TEST_P(EndToEndTest, SimilarContainersError) {
  // ensure that an error is raised when similarContainers is not set and when
  // using SINGLE_RANDOM_STRATIFIED
  auto solver = makeSimpleProblem(GetParam());
  LocalSearchSolverSpec localSearchSolverSpec;
  localSearchSolverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec("SINGLE_RANDOM_STRATIFIED"));
  solver->addSolver(localSearchSolverSpec);

  // just adding some goal to make sure the localSearch tries to use the move
  // type
  BalanceSpec balanceSpec;
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  solver->addGoal(balanceSpec);

  REBALANCER_EXPECT_RUNTIME_ERROR(
      solver->solve(),
      "Expected SimilarContainers definition to be provided for sampling");
}

TEST_P(EndToEndTest, DestinationsToExploreError) {
  // ensure that an error is raised when destinationsToExplore is not set and
  // using SINGLE_RANDOM_STRATIFIED
  auto solver = makeSimpleProblem(GetParam());
  LocalSearchSolverSpec localSearchSolverSpec;
  localSearchSolverSpec.moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleRandomStratifiedMoveTypeSpec()));

  REBALANCER_EXPECT_RUNTIME_ERROR(
      solver->addSolver(localSearchSolverSpec),
      "DestinationsToExploreOptions is empty; you need to specify one of the options");
}

TEST_P(EndToEndTest, LocalSearchProfilesInStageSolver) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});

  solver->setObjectName("shard");
  solver->setContainerName("host");

  const std::map<std::string, std::vector<std::string>> initial_assignment = {
      {"h1", {"s1", "s2", "s3"}},
      {"h2", {}},
  };
  solver->setAssignment(initial_assignment);

  // add to goal drain h1
  ToFreeSpec toFreeSpec;
  toFreeSpec.containers() = {"h1"};
  solver->addGoal(toFreeSpec);

  // create a localSearchStageSolver with 3 stages and each with move limit of
  // 1 and each using different set of move types
  LocalSearchStageSolverSpec stageSolverSpec;

  std::vector<LocalSearchStageSpec> stageSpecs;
  for (const auto i : folly::irange(3)) {
    LocalSearchStageSpec stageSpec;
    stageSpec.begin() = 0;
    stageSpec.end() = 1;
    stageSpec.solverSpec()->exploreMovesFromContainersNotInObjective() = false;

    LocalSearchSolverSpec solverSpec;
    solverSpec.stopAfterMoves() = 1;

    if (i == 0) {
      solverSpec.moveTypeList()->push_back(
          ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
    } else if (i == 1) {
      // deliberately choosing a bad destination container so that no move
      // will be performed using "SINGLE_FIXED_DEST"
      solverSpec.specialContainer() = "h1";
      solverSpec.moveTypeList()->push_back(
          ProblemSolver::makeMoveTypeSpec("SINGLE_FIXED_DEST"));
      solverSpec.moveTypeList()->push_back(
          ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
    } else {
      solverSpec.specialContainer() = "h2";
      SingleFixedSourceMoveTypeSpec singleFixedSourceMoveTypeSpec;
      singleFixedSourceMoveTypeSpec.specialContainer() = "h2";
      solverSpec.moveTypeList()->push_back(
          ProblemSolver::makeMoveTypeSpec(singleFixedSourceMoveTypeSpec));
      solverSpec.moveTypeList()->push_back(
          ProblemSolver::makeMoveTypeSpec("SINGLE_FIXED_DEST"));
      solverSpec.moveTypeList()->push_back(
          ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
    }

    stageSpec.solverSpec() = solverSpec;
    stageSpecs.push_back(stageSpec);
  }
  stageSolverSpec.stageSpecs() = stageSpecs;
  solver->addSolver(stageSolverSpec);

  auto solution = solver->solve();

  // sanity checks
  EXPECT_NEAR(3.0, *solution.initialObjective()->value(), kEps);
  EXPECT_NEAR(0.0, *solution.finalObjective()->value(), kEps);

  // verify localSearchProfiles
  {
    auto& profiles = *solution.problemProfile()->localSearchProfiles();
    EXPECT_EQ(1, profiles.size());

    auto& profile = profiles.at(0);

    auto& moveTypes = *profile.moveTypeNames();
    EXPECT_EQ(3, moveTypes.size());
    EXPECT_EQ(
        (std::set<std::string>{
            "SINGLE", "SINGLE_FIXED_DEST", "SINGLE_FIXED_SOURCE"}),
        std::set<std::string>(moveTypes.begin(), moveTypes.end()));

    std::unordered_map<std::string, int> moveTypeToIndex;
    for (const auto i : folly::irange(moveTypes.size())) {
      auto& moveType = moveTypes.at(i);
      moveTypeToIndex.emplace(moveType, i);
    }

    auto& events = *profile.moveTypeEvents();
    EXPECT_EQ(5, events.size());

    EXPECT_EQ(moveTypeToIndex["SINGLE"], *events.at(0).moveTypeIndex());
    EXPECT_NEAR(3.0, *events.at(0).initialValue(), kEps);
    EXPECT_NEAR(2.0, *events.at(0).finalValue(), kEps);

    EXPECT_EQ(
        moveTypeToIndex["SINGLE_FIXED_DEST"], *events.at(1).moveTypeIndex());
    EXPECT_NEAR(2.0, *events.at(1).initialValue(), kEps);
    EXPECT_NEAR(2.0, *events.at(1).finalValue(), kEps);

    EXPECT_EQ(moveTypeToIndex["SINGLE"], *events.at(2).moveTypeIndex());
    EXPECT_NEAR(2.0, *events.at(2).initialValue(), kEps);
    EXPECT_NEAR(1.0, *events.at(2).finalValue(), kEps);

    EXPECT_EQ(
        moveTypeToIndex["SINGLE_FIXED_SOURCE"], *events.at(3).moveTypeIndex());
    EXPECT_NEAR(1.0, *events.at(3).initialValue(), kEps);
    EXPECT_NEAR(1.0, *events.at(3).finalValue(), kEps);

    EXPECT_EQ(
        moveTypeToIndex["SINGLE_FIXED_DEST"], *events.at(4).moveTypeIndex());
    EXPECT_NEAR(1.0, *events.at(4).initialValue(), kEps);
    EXPECT_NEAR(0.0, *events.at(4).finalValue(), kEps);
  }
}

TEST_P(EndToEndTest, EnableObjectPotentialSorting) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});

  solver->setObjectName("shard");
  solver->setContainerName("host");

  const std::map<std::string, std::vector<std::string>> initial_assignment = {
      {"h1", {"s1"}},
      {"h2", {"s2"}},
      {"h3", {}},
  };
  solver->setAssignment(initial_assignment);

  // add to goal drain h1
  ToFreeSpec toFreeSpec;
  toFreeSpec.containers() = {"h1", "h2"};
  solver->addGoal(toFreeSpec);

  interface::LocalSearchStageSpec stageSpec;
  stageSpec.begin() = 0;
  stageSpec.end() = 1;
  stageSpec.solverSpec()->moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  stageSpec.solverSpec()->enableObjectPotentialSorting() = true;

  interface::LocalSearchStageSolverSpec stageSolverSpec;
  stageSolverSpec.recomputeContainerOrderingAfterEveryMove() = false;
  stageSolverSpec.stageSpecs() = {std::move(stageSpec)};

  solver->addSolver(stageSolverSpec);

  auto solution = solver->solve();

  EXPECT_EQ(
      interface::EndReason::OPTIMAL,
      *solution.solverSummaries()->at(0).endReason());

  const std::map<std::string, std::string> expected_assignment = {
      {"s1", "h3"}, {"s2", "h3"}};
  EXPECT_EQ(expected_assignment, toOrderedMap(*solution.assignment()));
}

TEST_P(EndToEndTest, ConstraintPolicyHardWithInitialAssignmentFeasibleProblem) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});

  solver->setObjectName("shard");
  solver->setContainerName("host");
  solver->setRunId("feasible_problem_hard_constraint_policy");
  interface::ManifoldBackupParams backupParams;
  backupParams.uploadPolicy() = ManifoldUploadPolicy::NEVER;
  solver->setManifoldBackupParams(backupParams);

  solver->setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"h1", {"s1", "s2", "s3"}}, {"h2", {}}});

  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "shard_count";
  capacitySpec.definition() = CapacitySpecDefinition::AFTER;
  capacitySpec.limit()->scopeItemLimits() = {{"h1", 0}, {"h2", 3}};
  solver->addConstraint(capacitySpec, interface::ConstraintPolicy::HARD);

  CapacitySpec capacitySpec2;
  capacitySpec2.scope() = "host";
  capacitySpec2.dimension() = "shard_count";
  capacitySpec2.definition() = CapacitySpecDefinition::AFTER;
  capacitySpec2.bound() = CapacitySpecBound::MIN;
  capacitySpec2.limit()->scopeItemLimits() = {{"h1", 0}, {"h2", 0}};
  solver->addConstraint(capacitySpec2);

  const auto optimalSpec = facebook::algopt::makeAvailableOptimalSolverSpec();
  solver->addSolver(optimalSpec);

  auto solution = solver->solve();

  // The problem is feasible, so the solver should not throw an exception even
  // though the initial assignment is invalid. We expect all the shards to be
  // assigned to h2
  for (auto& [_, host] : *solution.assignment()) {
    EXPECT_EQ(host, "h2");
  }
}

TEST_P(
    EndToEndTest,
    ConstraintPolicyHardWithInitialAssignmentInfeasibleProblem) {
  auto solver =
      initializeTestProblemSolver({.executorThreadCount = GetParam()});

  solver->setObjectName("shard");
  solver->setContainerName("host");
  solver->setConstraintPolicy(interface::ConstraintPolicy::HARD);
  solver->setRunId("infeasible_problem_hard_constraint_policy");
  interface::ManifoldBackupParams backupParams;
  backupParams.uploadPolicy() = ManifoldUploadPolicy::NEVER;
  solver->setManifoldBackupParams(backupParams);

  solver->setAssignment(
      std::map<std::string, std::vector<std::string>>{
          {"h1", {"s1", "s2", "s3"}}});

  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "shard_count";
  capacitySpec.definition() = CapacitySpecDefinition::AFTER;
  capacitySpec.limit()->scopeItemLimits() = {{"h1", 0}};
  solver->addConstraint(capacitySpec);

  const auto optimalSpec = facebook::algopt::makeAvailableOptimalSolverSpec();
  solver->addSolver(optimalSpec);

  // The problem is infeasible, and so since we use ConstraintPolicy::HARD, we
  // expect rebalancer to throw an exception in the end
  REBALANCER_EXPECT_RUNTIME_ERROR(
      solver->solve(),
      "Problem infeasible_problem_hard_constraint_policy infeasible");
}

TEST_P(EndToEndTest, SolveTwice) {
  auto solver = makeSimpleProblem(GetParam());

  BalanceSpec balanceSpec;
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  balanceSpec.name() = "balance cpu";
  solver->addGoal(balanceSpec);

  solver->addSolver(makeDefaultLocalSearchSolver());

  auto solution1 = solver->solve();

  // Verify problem state after the first solve
  {
    const auto& bundle1 = solver->getBundle();
    const auto& idStore = *bundle1.problem()->universe()->idStore();
    EXPECT_EQ(1, idStore.goalNames()->size());
    EXPECT_EQ("balance cpu", idStore.goalNames()->at(0));
    EXPECT_EQ(2, idStore.constraintNames()->size());
    EXPECT_EQ(2, idStore.scopeNames()->size());
    EXPECT_EQ(0, idStore.partitionNames()->size());
    EXPECT_EQ(0, idStore.routingConfigNames()->size());
    EXPECT_EQ(3, idStore.dimensionNames()->size());
  }

  {
    const map<string, string> expectedAssignment = {
        {"s1", "h2"}, {"s2", "h1"}, {"s3", "h1"}};
    EXPECT_EQ(expectedAssignment, toOrderedMap(*solution1.assignment()));
  }

  EXPECT_EQ(0, *solution1.finalConstraint()->brokenCount());
  EXPECT_NEAR(0, *solution1.finalConstraint()->brokenVal(), kEps);

  {
    const auto& finalObjectives = *solution1.finalObjective()->objs();
    const auto& cpuBalance =
        getObjectiveSummary(finalObjectives, "balance cpu");
    EXPECT_NEAR(0, *cpuBalance.value(), kEps);
  }

  EXPECT_NEAR(0.25, *solution1.initialObjective()->value(), kEps);
  EXPECT_NEAR(0, *solution1.finalObjective()->value(), kEps);

  BalanceSpec balanceSpec2;
  balanceSpec2.scope() = "rack";
  balanceSpec2.dimension() = "network";
  balanceSpec2.name() = "balance network";
  solver->addGoal(balanceSpec2);

  const std::map<std::string, std::string> hostToZone = {
      {"h1", "z1"}, {"h2", "z1"}};
  solver->addScope("zone", hostToZone);

  const std::map<std::string, double> shardMemory = {
      {"s1", 100}, {"s2", 200}, {"s3", 150}};
  const std::map<std::string, double> zoneMemory = {{"z1", 1000}};
  solver->addObjectDimension("memory", shardMemory);
  solver->addScopeDimension("memory", "zone", zoneMemory);

  CapacitySpec capacitySpec;
  capacitySpec.scope() = "zone";
  capacitySpec.dimension() = "memory";
  capacitySpec.name() = "do not exceed memory";
  solver->addConstraint(capacitySpec);

  const std::map<std::string, std::string> shardToTenant = {
      {"s1", "t1"}, {"s2", "t1"}, {"s3", "t2"}};
  solver->addPartition("tenant", shardToTenant);

  solver->addRoutingConfig(
      "routingConfig1",
      "rack",
      "tenant",
      {},
      {{"r1", {{"r2", 10}}}, {"r2", {{"r1", 10}}}},
      std::nullopt);

  // Use the solution from the first solve as the new initial assignment
  {
    std::map<std::string, std::vector<std::string>> containerToObjects;
    for (const auto& [object, container] : *solution1.assignment()) {
      containerToObjects[container].push_back(object);
    }
    solver->setAssignment(containerToObjects);
  }

  auto solution2 = solver->solve();

  // Verify problem state after editing
  {
    const auto& bundle2 = solver->getBundle();
    const auto& idStore = *bundle2.problem()->universe()->idStore();
    EXPECT_EQ(2, idStore.goalNames()->size());
    EXPECT_EQ("balance cpu", idStore.goalNames()->at(0));
    EXPECT_EQ("balance network", idStore.goalNames()->at(1));
    EXPECT_EQ(3, idStore.constraintNames()->size());
    EXPECT_EQ(3, idStore.scopeNames()->size());
    EXPECT_EQ(4, idStore.dimensionNames()->size());
    EXPECT_EQ(1, idStore.partitionNames()->size());
    EXPECT_EQ("tenant", idStore.partitionNames()->at(0));
    EXPECT_EQ(1, idStore.routingConfigNames()->size());
    EXPECT_EQ("routingConfig1", idStore.routingConfigNames()->at(0));
  }

  // Verify that the solution from the first solve was correctly applied as
  // the initial assignment for the second solve
  EXPECT_EQ(
      toOrderedMap(*solution1.assignment()),
      toOrderedMap(*solution2.initialAssignment()));

  EXPECT_EQ(3, solution2.assignment()->size());

  EXPECT_EQ(0, *solution2.finalConstraint()->brokenCount());
  EXPECT_NEAR(0, *solution2.finalConstraint()->brokenVal(), kEps);

  {
    const auto& finalConstraints = *solution2.finalConstraint()->constraints();
    EXPECT_EQ(3, finalConstraints.size());
    const auto& cpuConstraint =
        getConstraintSummary(finalConstraints, "do not exceed cpu");
    EXPECT_NEAR(0, *cpuConstraint.value(), kEps);
    const auto& networkConstraint =
        getConstraintSummary(finalConstraints, "do not exceed network");
    EXPECT_NEAR(0, *networkConstraint.value(), kEps);
    const auto& memConstraint =
        getConstraintSummary(finalConstraints, "do not exceed memory");
    EXPECT_NEAR(0, *memConstraint.value(), kEps);
  }

  {
    const auto& finalObjectives = *solution2.finalObjective()->objs();
    const auto& cpuBalance =
        getObjectiveSummary(finalObjectives, "balance cpu");
    EXPECT_NEAR(0, *cpuBalance.value(), kEps);
    const auto& networkBalance =
        getObjectiveSummary(finalObjectives, "balance network");
    EXPECT_NEAR(0.075, *networkBalance.value(), kEps);
  }

  EXPECT_NEAR(0.075, *solution2.initialObjective()->value(), kEps);
  EXPECT_NEAR(0.075, *solution2.finalObjective()->value(), kEps);

  {
    const auto& assignment = *solution2.assignment();
    double totalMemInZone = 0;
    for (const auto& [shard, host] : assignment) {
      totalMemInZone += shardMemory.at(shard);
    }
    EXPECT_LE(totalMemInZone, zoneMemory.at("z1"));
  }

  {
    const std::map<std::string, double> shardCpu = {
        {"s1", 50}, {"s2", 10}, {"s3", 40}};
    const std::map<std::string, double> hostCpu = {{"h1", 100}, {"h2", 100}};
    std::map<std::string, double> hostLoad;
    for (const auto& [shard, host] : *solution2.assignment()) {
      hostLoad[host] += shardCpu.at(shard);
    }
    for (const auto& [host, load] : hostLoad) {
      EXPECT_LE(load, hostCpu.at(host));
    }
  }

  {
    const std::map<std::string, double> shardNetwork = {
        {"s1", 1000}, {"s2", 2000}, {"s3", 2000}};
    const std::map<std::string, std::string> hostToRack = {
        {"h1", "r1"}, {"h2", "r2"}};
    const std::map<std::string, double> rackNetwork = {
        {"r1", 10000}, {"r2", 10000}};
    std::map<std::string, double> rackLoad;
    for (const auto& [shard, host] : *solution2.assignment()) {
      rackLoad[hostToRack.at(host)] += shardNetwork.at(shard);
    }
    for (const auto& [rack, load] : rackLoad) {
      EXPECT_LE(load, rackNetwork.at(rack));
    }
  }
}

} // namespace facebook::rebalancer::interface::tests
