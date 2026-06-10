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

#include "algopt/rebalancer/benchmarks/utils/BenchmarkUtils.h"
#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/ProblemSolverFactory.h"
#include "algopt/rebalancer/interface/tests/utils.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

#include <random>
#include <stdexcept>

using namespace folly;
using namespace std;
using namespace facebook::rebalancer::interface;

using SolverType = facebook::rebalancer::interface::benchmarks::SolverType;

static void runAssignmentAffinitiesWithObjectPotentialSorting(
    int regions,
    int hostsPerRegion,
    int shardsPerHost,
    int unhappyShards) {
  const int hosts = regions * hostsPerRegion;
  const int shards = hosts * shardsPerHost;

  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("shard");
  solver->setContainerName("host");

  vector<string> shardNames;
  map<string, vector<string>> hostToShards;
  for (const auto shardId : folly::irange(shards)) {
    int hostId = shardId / shardsPerHost;
    hostToShards[fmt::format("host-{}", hostId)].push_back(
        fmt::format("shard-{}", shardId));
    shardNames.push_back(fmt::format("shard-{}", shardId));
  }
  solver->setAssignment(hostToShards);

  map<string, string> hostToRegion;
  for (const auto hostId : folly::irange(hosts)) {
    int regionId = hostId / hostsPerRegion;
    hostToRegion[fmt::format("host-{}", hostId)] =
        fmt::format("region-{}", regionId);
  }
  solver->addScope("region", hostToRegion);

  std::shuffle(
      shardNames.begin(), shardNames.end(), std::default_random_engine(42));
  const set<string> unhappyShardNames(
      shardNames.begin(), shardNames.begin() + unhappyShards);

  AssignmentAffinitiesSpec affinities;
  affinities.scope() = "region";

  for (const auto shardId : folly::irange(shards)) {
    string shardName = fmt::format("shard-{}", shardId);
    const int hostId = shardId / shardsPerHost;
    int regionId = hostId / hostsPerRegion;
    int otherRegionId = (regionId + 1) % regions;

    AssignmentAffinity affinity;
    affinity.objectName() = shardName;
    affinity.scopeItemName() = fmt::format(
        "region-{}",
        !unhappyShardNames.contains(shardName) ? regionId : otherRegionId);
    affinity.affinity() = 1.0;

    affinities.affinities().value().push_back(affinity);
  }
  solver->addGoal(std::move(affinities));

  LocalSearchStageSolverSpec localSearchStageSolverSpec;

  LocalSearchStageSpec localSearchStageSpec;
  localSearchStageSpec.begin() = 0;
  localSearchStageSpec.end() = 1;
  localSearchStageSpec.solverSpec()->moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
  localSearchStageSpec.solverSpec()->stopAfterMoves() = unhappyShards;
  localSearchStageSpec.solverSpec()->enableObjectPotentialSorting() = true;
  localSearchStageSpec.solverSpec()
      ->exploreMovesFromContainersNotInObjective() = false;

  localSearchStageSolverSpec.stageSpecs() = {std::move(localSearchStageSpec)};

  solver->addSolver(localSearchStageSolverSpec);
  solver->solve();
}

static void runFixedObjects(int objectCount, int containerCount) {
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("shard");
  solver->setContainerName("server");

  map<string, vector<string>> serverToShards;
  for (const auto serverId : folly::irange(containerCount)) {
    serverToShards[fmt::format("server-{}", serverId)] = {};
  }
  for (const auto shardId : folly::irange(objectCount)) {
    int serverId = shardId % containerCount;
    serverToShards[fmt::format("server-{}", serverId)].push_back(
        fmt::format("shard-{}", shardId));
  }
  solver->setAssignment(serverToShards);

  {
    AvoidMovingSpec spec;
    for (const auto shardId : folly::irange(objectCount)) {
      spec.objects()->push_back(fmt::format("shard-{}", shardId));
    }
    solver->addConstraint(std::move(spec));
  }

  {
    AssignmentAffinitiesSpec spec;
    spec.scope() = "server";
    for (const auto shardId : folly::irange(objectCount)) {
      int serverId = (shardId + 1) % containerCount;
      AssignmentAffinity affinity;
      affinity.objectName() = fmt::format("shard-{}", shardId);
      affinity.scopeItemName() = fmt::format("server-{}", serverId);
      affinity.affinity() = 1.0;
      spec.affinities()->push_back(std::move(affinity));
    }
    solver->addGoal(std::move(spec));
  }

  solver->addSolver(makeDefaultLocalSearchSolver());
  solver->solve();
}

static void runMoveObjectsOnce(int objectCount, bool moveObjectsOnce) {
  const int containerCount = objectCount + 1;

  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("shard");
  solver->setContainerName("server");

  if (moveObjectsOnce) {
    solver->enableRestrictMovingObjectOnlyOnce();
  }

  map<string, vector<string>> serverToShards;
  for (const auto i : folly::irange(objectCount)) {
    serverToShards[fmt::format("server-{}", i)] = {fmt::format("shard-{}", i)};
  }
  serverToShards[fmt::format("server-{}", containerCount - 1)] = {};
  solver->setAssignment(serverToShards);

  {
    CapacitySpec spec;
    spec.scope() = "server";
    spec.dimension() = "shard_count";
    spec.limit()->globalLimit() = 1;
    solver->addConstraint(std::move(spec));
  }

  {
    AssignmentAffinitiesSpec spec;
    spec.scope() = "server";
    for (const auto i : folly::irange(1, containerCount)) {
      for (const auto shardId : folly::irange(objectCount)) {
        int serverId = (shardId + i) % containerCount;
        AssignmentAffinity affinity;
        affinity.objectName() = fmt::format("shard-{}", shardId);
        affinity.scopeItemName() = fmt::format("server-{}", serverId);
        affinity.affinity() = spec.affinities()->size() + 1;
        spec.affinities()->push_back(std::move(affinity));
      }
    }
    solver->addGoal(std::move(spec));
  }

  {
    LocalSearchSolverSpec spec;
    spec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec()));
    solver->addSolver(spec);
  }

  solver->solve();
}

static void runInstanceWithFewMovesPerContainer(
    int objectCount,
    int containerCount) {
  // Create an example where all containers are tied in the hottest container
  // ordering and each container has just one useful move. Without dynamic seed,
  // each container will be explored twice, whereas with dynamic seed,
  // repetitions are less likely.
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("shard");
  solver->setContainerName("server");

  std::map<std::string, std::string> serverToScopeName;
  std::map<std::string, std::vector<std::string>> serverToShards;
  for (const auto serverId : folly::irange(containerCount)) {
    auto serverName = fmt::format("server{}", serverId);
    serverToShards[serverName] = {};
    serverToScopeName.emplace(serverName, "standardServer");
  }
  // create an extra server;
  serverToShards[fmt::format("extra-server")] = {};

  // to ensure that the objects are not treated as equivalent
  std::map<std::string, double> dummyDimValues;
  std::map<std::string, double> movableShards;
  const int shardsPerServer = objectCount / containerCount;
  for (const auto shardId : folly::irange(objectCount)) {
    // shards with id 0...(shardsPerServer-1) are in server 0, shard with id
    // shardsPerServer,...,2*shardsPerServer-1 are in server1 and so on
    int serverId = shardId / shardsPerServer;
    auto shardName = fmt::format("shard{}", shardId);
    serverToShards[fmt::format("server{}", serverId)].push_back(shardName);

    // the first shard in each server is considered movable; everything else is
    // immovable
    if (shardId % shardsPerServer == 0) {
      movableShards.emplace(shardName, 1);
    }

    dummyDimValues.emplace(shardName, 1.0 / (shardId + 1));
  }
  solver->setAssignment(serverToShards);

  solver->addScope("allServersExceptExtra", serverToScopeName);
  solver->addObjectDimension("movableShards", std::move(movableShards));
  solver->addObjectDimension("dummyDim", std::move(dummyDimValues));

  {
    CapacitySpec spec;
    spec.name() = "make all the unmovable shards not move";
    spec.scope() = "allServersExceptExtra";
    spec.dimension() = "shard_count";
    Limit limit;
    limit.globalLimit() = containerCount;
    spec.limit() = std::move(limit);

    spec.bound() = CapacitySpecBound::MIN;

    solver->addConstraint(std::move(spec));
  }

  {
    CapacitySpec spec;
    spec.name() =
        "remove as many movable shards as possible from all servers except the extra one";
    spec.scope() = "allServersExceptExtra";
    spec.dimension() = "movableShards";
    Limit limit;
    limit.globalLimit() = 0;
    spec.limit() = std::move(limit);

    solver->addGoal(std::move(spec));
  }

  {
    CapacitySpec spec;
    spec.name() = "dummy constraint to ensure objects are not equivalent";
    spec.scope() = "server";
    spec.dimension() = "dummyDim";
    Limit limit;
    limit.globalLimit() = objectCount;
    spec.limit() = std::move(limit);

    spec.bound() = CapacitySpecBound::MAX;

    solver->addConstraint(std::move(spec));
  }

  {
    LocalSearchSolverSpec spec;
    spec.moveTypeList()->push_back(
        ProblemSolver::makeMoveTypeSpec(SingleFastMoveTypeSpec()));
    solver->addSolver(spec);
  }

  solver->solve();
}

static void runLargeChangeSet(int objectCount) {
  const int containerCount = objectCount;
  // build a simple problem that is easy to solve but creates a large set of
  // changes
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("shard");
  solver->setContainerName("host");
  map<string, vector<string>> assignment;
  for (const auto i : folly::irange(containerCount)) {
    assignment[fmt::format("h{}", i)] = {};
  }
  // initially all objects are in h0
  for (const auto i : folly::irange(objectCount)) {
    assignment["h0"].push_back(fmt::format("s{}", i));
  }
  solver->setAssignment(assignment);

  map<string, double> shard_cpu;
  for (const auto i : folly::irange(objectCount)) {
    shard_cpu[fmt::format("s{}", i)] = 1;
  }
  solver->addObjectDimension("cpu", std::move(shard_cpu));

  // capacity spec will force all but one object to move from h0
  CapacitySpec spec;
  spec.scope() = "host";
  spec.dimension() = "shard_count";
  spec.limit()->globalLimit() = 1;
  spec.bound() = CapacitySpecBound::MIN;
  solver->addGoal(std::move(spec));

  const OptimalSolverSpec optimal;
  solver->addSolver(optimal);
  solver->solve();
}

static void runGroupCapacity(int objectCount, int containerCount) {
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("tasks");
  solver->setContainerName("host");

  map<string, vector<string>> assignment;
  for (const auto i : folly::irange(containerCount)) {
    assignment[fmt::format("h{}", i)] = {};
  }

  // initially all objects are in container i % containerCount
  for (const auto i : folly::irange(objectCount)) {
    auto hostName = fmt::format("h{}", i % containerCount);
    assignment[hostName].push_back(fmt::format("t{}", i));
  }
  solver->setAssignment(assignment);

  // create 10000 groups
  constexpr int nGroups = 10000;
  std::map<std::string, std::string> taskToJob_;
  for (const auto i : folly::irange(objectCount)) {
    auto group = fmt::format("job{}", i % nGroups);
    auto task = fmt::format("t{}", i);
    taskToJob_.emplace(task, group);
  }
  solver->addPartition("job", std::move(taskToJob_));

  GroupCapacitySpec groupCapacitySpec;
  groupCapacitySpec.name() = "groupCapacityGoal";
  groupCapacitySpec.scope() = "host";
  groupCapacitySpec.partitionName() = "job";
  groupCapacitySpec.contributionPartition() = "job";
  groupCapacitySpec.definition() = GroupCapacitySpecDefinition::AFTER;

  Limit limit;
  limit.type() = LimitType::ABSOLUTE;
  limit.globalLimit() = 1;

  // contribution limit is one of 4 values in {1, 2, 3, 4}
  Limit contribution;
  contribution.type() = LimitType::ABSOLUTE;
  std::map<std::string, std::map<std::string, double>> scopeItemContributions;
  for (const auto i : folly::irange(containerCount)) {
    for (const auto j : folly::irange(nGroups)) {
      scopeItemContributions[fmt::format("h{}", i)][fmt::format("job{}", j)] =
          1 + (j % 4);
    }
  }
  contribution.scopeItemToGroupLimits() = std::move(scopeItemContributions);

  groupCapacitySpec.limit() = std::move(limit);
  groupCapacitySpec.contribution() = std::move(contribution);

  solver->addGoal(std::move(groupCapacitySpec));

  addSolver(*solver, SolverType::MATERIALIZE_ONLY);

  solver->solve();
}

static void
runLargeProblem(int objectCount, int containerCount, int dimensionCount) {
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");

  solver->setObjectName("tasks");
  solver->setContainerName("host");

  map<string, vector<string>> assignment;
  for (const auto hostId : folly::irange(containerCount)) {
    assignment[fmt::format("host_{}", hostId)] = {};
  }
  for (const auto taskId : folly::irange(objectCount)) {
    int hostId = taskId % containerCount;
    assignment[fmt::format("host_{}", hostId)].push_back(
        fmt::format("task_{}", taskId));
  }
  solver->setAssignment(assignment);

  for (const auto dimensionId : folly::irange(dimensionCount)) {
    map<string, double> dimension;
    for (const auto taskId : folly::irange(objectCount)) {
      dimension[fmt::format("task_{}", taskId)] = taskId / double(objectCount);
    }
    solver->addObjectDimension(
        fmt::format("dimension_{}", dimensionId), dimension);
  }

  addSolver(*solver, SolverType::MATERIALIZE_ONLY);

  solver->solve();
}

static void runLocalSearchWithFixedContainers(
    int numObjects,
    int numContainers) {
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("task");
  solver->setContainerName("host");
  // All but last container are frozen and empty
  const int numFixedContainers = numContainers - 1;
  // all objects are in last container
  auto lastContainerName = fmt::format("host{}", numContainers - 1);
  std::map<std::string, std::vector<std::string>> initialAssignment;
  for (const auto hostId : folly::irange(numFixedContainers)) {
    initialAssignment[fmt::format("host{}", hostId)] = {};
  }
  for (const auto taskId : folly::irange(numObjects)) {
    initialAssignment[lastContainerName].push_back(
        fmt::format("task{}", taskId));
  }
  solver->setAssignment(initialAssignment);

  // add some cpu dimension to make sure all objects form unique equivalent set
  std::map<std::string, double> taskCpu;
  for (const auto taskId : folly::irange(numObjects)) {
    taskCpu[fmt::format("task{}", taskId)] = taskId;
  }
  solver->addObjectDimension("cpu", std::move(taskCpu));

  {
    // Add capacity goal on empty containers so that there is incentive to move
    // objects
    CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "cpu";
    capacitySpec.bound() = CapacitySpecBound::MIN;
    capacitySpec.limit()->globalLimit() = 1;
    solver->addGoal(std::move(capacitySpec));
  }

  {
    // However, this spec should prevent any moves from happening
    NonAcceptingSpec spec;
    spec.scope() = "host";
    for (const auto hostId : folly::irange(numFixedContainers)) {
      spec.items()->push_back(fmt::format("host{}", hostId));
    }
    solver->addConstraint(std::move(spec));
  }

  LocalSearchStageSpec stageSpec;
  stageSpec.begin() = 0;
  stageSpec.end() = 1;
  stageSpec.name() = "default";
  stageSpec.solverSpec()->exploreMovesFromContainersNotInObjective() = false;
  auto singleFixedSourceMoveTypeSpec = SingleFixedSourceMoveTypeSpec();
  singleFixedSourceMoveTypeSpec.specialContainer() =
      std::move(lastContainerName);
  stageSpec.solverSpec()->moveTypeList()->push_back(
      ProblemSolver::makeMoveTypeSpec(
          std::move(singleFixedSourceMoveTypeSpec)));

  LocalSearchStageSolverSpec spec;
  spec.stageSpecs()->push_back(stageSpec);
  solver->addSolver(spec);
  solver->solve();
}

static void runAvoidRecomputingHottestOrderingSpecialCase(
    int objectCount,
    SolverType solverType) {
  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  solver->setObjectName("task");
  solver->setContainerName("host");

  const int nTasks = objectCount;
  const int nHosts = nTasks;
  std::vector<std::string> fakeHostTasks;
  for (const auto i : folly::irange(nTasks)) {
    auto taskName = fmt::format("task-{}", i);
    fakeHostTasks.push_back(taskName);
  }

  std::map<std::string, std::vector<std::string>> containerToObjects;
  for (const auto i : folly::irange(nHosts)) {
    auto hostName = fmt::format("host-{}", i);
    containerToObjects[hostName] = {};
  }
  containerToObjects["fakeHost"] = fakeHostTasks;
  solver->setAssignment(containerToObjects);

  { // capacity of every container except `fakeHost` should be at least 1
    CapacitySpec capacitySpec;
    capacitySpec.scope() = "host";
    capacitySpec.dimension() = "task_count";
    capacitySpec.limit()->globalLimit() = 1;
    capacitySpec.bound() = CapacitySpecBound::MIN;
    capacitySpec.filter()->itemsBlacklist() = {"fakeHost"};
    solver->addConstraint(capacitySpec);
  }

  switch (solverType) {
    case SolverType::LOCAL_SEARCH: {
      LocalSearchSolverSpec solverSpec;
      solverSpec.moveTypeList()->push_back(
          ProblemSolver::makeMoveTypeSpec(SingleRandomBatchesMoveTypeSpec()));
      solverSpec.recomputeContainerOrderingAfterEveryMove() = true;

      solver->addSolver(solverSpec);
      break;
    }
    case SolverType::LOCAL_SEARCH_STAGES: {
      LocalSearchStageSolverSpec stageSolverSpec;
      stageSolverSpec.recomputeContainerOrderingAfterEveryMove() = true;
      stageSolverSpec.exploreMovesFromContainersNotInObjective() = true;

      LocalSearchStageSpec stageSpec;
      stageSpec.begin() = 0;
      stageSpec.end() = 1;
      stageSpec.solverSpec()->moveTypeList()->push_back(
          ProblemSolver::makeMoveTypeSpec(SingleRandomBatchesMoveTypeSpec()));
      stageSolverSpec.stageSpecs()->push_back(stageSpec);

      solver->addSolver(stageSolverSpec);
      break;
    }
    default:
      throw std::runtime_error(
          "Expect only LOCAL_SEARCH or LOCAL_SEARCH_STAGES as solver type");
  }

  // Get a solution from Rebalancer.
  auto solution = solver->solve();
  auto assignment = *solution.assignment();

  // expect every task to move out of "fakeHost"
  for (auto& task : fakeHostTasks) {
    if (assignment[task] == "fakeHost") {
      throw std::runtime_error("Expected every task to move out of 'fakeHost'");
    }
  }
}

// materializer + local search solver

BENCHMARK(AssignAffinitiesObjectPotential) {
  runAssignmentAffinitiesWithObjectPotentialSorting(10, 1000, 10, 10);
}

BENCHMARK(FixedObjects) {
  runFixedObjects(100, 100);
}

BENCHMARK(DynamicSeedForWorstContainers) {
  // Benchmark that shows why it is useful to use dynamic seed when computing
  // hottest containers (i.e., seed used by
  // CoreLocalSearchSolve::getWorstContainers())
  runInstanceWithFewMovesPerContainer(1E6, 10);
}

BENCHMARK(MoveObjectsOnce_off) {
  runMoveObjectsOnce(100, false);
}

BENCHMARK(MoveObjectsOnce_on) {
  runMoveObjectsOnce(100, true);
}

// materializer

BENCHMARK(GroupCapacity_500000_200) {
  // This benchmark shows why, for each contributionGroup, it is beneficial to
  // group together scopeItems that have the same contribution multiplier in
  // GroupCapacitySpec
  runGroupCapacity(500000, 200);
}

BENCHMARK(LargeProblem_1000000_10_100) {
  runLargeProblem(1000000, 10, 100);
}

// case when we have a large change set to apply
BENCHMARK(Large_changeset_50000) {
  runLargeChangeSet(50000);
}

// local search benchmark with a large number of fixed containers
BENCHMARK(Fixed_Containers_local_search) {
  runLocalSearchWithFixedContainers(10000, 1000);
}

BENCHMARK(RecomputeHottestAtLeastTwoNotSkippedLS) {
  // Recomputing hottest ordering after every move can be expensive. For the
  // special case where all containers except one are skipped (because e.g.,
  // only one container is able to give out objects), there is no point in
  // recomputing hottest ordering after every move. This benchmark is just to
  // ensure that we have that conditional check.
  runAvoidRecomputingHottestOrderingSpecialCase(
      10e3 /*objectCount = containerCount*/, SolverType::LOCAL_SEARCH);
}

BENCHMARK(RecomputeHottestAtLeastTwoNotSkippedStages) {
  // see the comment w.r.t. the benchmark
  // AvoidRecomputingHottestOrderingSpecialCaseLS
  runAvoidRecomputingHottestOrderingSpecialCase(
      10e3 /*objectCount = containerCount*/, SolverType::LOCAL_SEARCH_STAGES);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  runBenchmarks();

  return 0;
}
