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

#include "algopt/rebalancer/interface/ProblemSolverFactory.h"
#include "algopt/rebalancer/interface/tests/utils.h"
#include "algopt/rebalancer/solver/utils/equivalence_sets/EquivalenceSetsMatching.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>

using namespace facebook::rebalancer::interface;
using namespace std;
static void
EquivalenceSetsHeavyMatching(int numShards, int numHosts, int numRacks) {
  auto solverLeft =
      ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");
  auto solverRight =
      ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");

  constexpr int seed = 12345; // Choose any integer as the seed
  std::mt19937 eng(seed); // Seed the generator

  // set DBG2 because the hierarchical profiler will not print all the expected
  // nodes if that's not the case (e.g., if info/dbg1, it only prints the nodes
  // that takes moe than 5% of the total time)
  solverLeft->setLogLevel("DBG2");
  solverRight->setLogLevel("DBG2");

  // Set the object and container names
  solverLeft->setObjectName("shard");
  solverLeft->setContainerName("host");
  solverRight->setObjectName("shard");
  solverRight->setContainerName("host");

  std::vector<double> cpu_capacities = {10, 20, 30, 40};
  std::vector<double> network_capacities = {100, 200, 400};

  // Shards
  std::vector<std::string> shards;
  for (const auto i : folly::irange(1, numShards + 1)) {
    shards.emplace_back("s" + std::to_string(i));
  }

  // Hosts
  std::vector<std::string> hosts;
  for (const auto i : folly::irange(1, numHosts + 1)) {
    hosts.emplace_back("h" + std::to_string(i));
  }

  // Racks
  std::vector<std::string> racks;
  for (const auto i : folly::irange(1, numRacks + 1)) {
    racks.emplace_back("r" + std::to_string(i));
  }

  // Set the initial assignment
  map<string, vector<string>> initial_assignment;
  for (const auto& host : hosts) {
    initial_assignment[host] = {};
  }
  initial_assignment["h1"] = shards;

  solverLeft->setAssignment(initial_assignment);
  solverLeft->publishEquivalenceSetInfo();
  solverRight->setAssignment(initial_assignment);
  solverRight->publishEquivalenceSetInfo();

  // Define cpu dimension
  std::map<std::string, double> shard_cpu;
  std::uniform_int_distribution<> distr(
      0, cpu_capacities.size() - 1); // Define the range
  // Assign a random CPU capacity to each shard
  for (const auto& shard : shards) {
    const int random_index = distr(eng);
    shard_cpu[shard] = cpu_capacities[random_index];
  }

  // host cpu
  map<string, double> host_cpu;
  for (const auto& host : hosts) {
    host_cpu[host] = 100;
  }

  solverLeft->addObjectDimension("cpu", shard_cpu);
  solverLeft->addContainerDimension("cpu", host_cpu);
  solverRight->addObjectDimension("cpu", shard_cpu);
  solverRight->addContainerDimension("cpu", host_cpu);

  // Add capacity constraint and balance goal on host cpu
  CapacitySpec capacitySpec;
  capacitySpec.scope() = "host";
  capacitySpec.dimension() = "cpu";
  capacitySpec.name() = "do not exceed cpu";
  solverLeft->addConstraint(capacitySpec);
  solverRight->addConstraint(capacitySpec);

  BalanceSpec balanceSpec;
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  balanceSpec.name() = "balance cpu";
  solverLeft->addGoal(balanceSpec);
  solverRight->addGoal(balanceSpec);

  // Create rack scope
  map<string, string> host_to_rack;
  std::uniform_int_distribution<> distr1(0, numRacks - 1); // Define the range
  for (const auto& host : hosts) {
    host_to_rack[host] = racks.at(distr1(eng));
  }
  solverLeft->addScope("rack", host_to_rack);
  solverRight->addScope("rack", host_to_rack);

  // Define network dimension
  map<string, double> shard_network;
  std::uniform_int_distribution<> distr2(
      0, network_capacities.size() - 1); // Define the range
  // Assign a random network capacity to each shard
  for (const auto& shard : shards) {
    const int random_index = distr2(eng);
    shard_network[shard] = network_capacities[random_index];
  }

  map<string, double> rack_network;
  for (const auto& rack : racks) {
    rack_network[rack] = 1000;
  }
  solverLeft->addObjectDimension("network", shard_network);
  solverLeft->addScopeDimension("network", "rack", rack_network);
  solverRight->addObjectDimension("network", shard_network);
  solverRight->addScopeDimension("network", "rack", rack_network);

  // Add capacity constraint on rack network
  CapacitySpec capacitySpec2;
  capacitySpec2.scope() = "rack";
  capacitySpec2.dimension() = "network";
  capacitySpec2.name() = "do not exceed network";
  solverLeft->addConstraint(capacitySpec2);
  //  NOTE: we don't add this constraint to the right solver to see the impact

  // Make the solution stable
  AssignmentAffinitiesSpec assignmentAffinitiesSpec;
  assignmentAffinitiesSpec.name() = "make solution stable";
  // Equivalence sets before: {s1, s2} {s3} and {s4, s5}
  // splits equivalence sets as {s1} {s2} {s3} and {s4, s5}
  assignmentAffinitiesSpec.affinities() = {
      makeAssignmentAffinity("s1", "h3", 0.4),
      makeAssignmentAffinity("s2", "h4", 0.2),
      makeAssignmentAffinity("s3", "h2", 0.1)};
  solverLeft->addGoal(assignmentAffinitiesSpec, 1e-6);
  solverRight->addGoal(assignmentAffinitiesSpec, 1e-6);

  // Populate move reasons
  solverLeft->enableMoveStats();
  solverRight->enableMoveStats();

  solverLeft->addSolver(makeDefaultLocalSearchSolver());
  solverRight->addSolver(makeDefaultLocalSearchSolver());

  // Run the solver
  auto solutionLeft = solverLeft->solve();
  auto solutionRight = solverRight->solve();
  auto& equivSetInfoLeft = *solutionLeft.equivalenceSetInfo();
  auto& equivSetInfoRight = *solutionRight.equivalenceSetInfo();

  folly::F14FastMap<std::string, std::set<std::string>> equivObjectsLeft;
  for (auto& equivSet : *equivSetInfoLeft.equivalenceSets()) {
    equivObjectsLeft[*equivSet.name()].insert(
        equivSet.objectNames()->begin(), equivSet.objectNames()->end());
  }

  folly::F14FastMap<std::string, std::set<std::string>> equivObjectsRight;
  for (auto& equivSet : *equivSetInfoRight.equivalenceSets()) {
    equivObjectsRight[*equivSet.name()].insert(
        equivSet.objectNames()->begin(), equivSet.objectNames()->end());
  }

  EquivalenceSetsMatching match =
      facebook::rebalancer::interface::EquivalenceSetsMatching(
          equivSetInfoLeft, equivSetInfoRight);
  const folly::F14FastMap<std::string, std::string> matching =
      match.heavyEdgeMatching();

  XLOG(INFO) << "Left Equivalence sets ";
  for (auto& [setName, objects] : equivObjectsLeft) {
    std::string equivSet = setName + ":";
    for (auto& object : objects) {
      equivSet += object + ", ";
    }
    XLOG(INFO) << equivSet;
  }
  XLOG(INFO) << "Right Equivalence sets ";
  for (auto& [setName, objects] : equivObjectsRight) {
    std::string equivSet = setName + ":";
    for (auto& object : objects) {
      equivSet += object + ", ";
    }
    XLOG(INFO) << equivSet;
  }
  XLOG(INFO) << "Matching";
  for (auto& [setName, matchingSet] : matching) {
    XLOG(INFO) << setName << " -> " << matchingSet;
  }
}

BENCHMARK(Equivalence_sets_matching_example1) {
  EquivalenceSetsHeavyMatching(100, 10, 2);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
