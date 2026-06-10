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

#include <folly/init/Init.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace facebook::rebalancer::interface;

static void print(const ObjectiveSummary& summary) {
  cout << "  Value: " << *summary.value() << endl;
  int count = 0;
  for (auto& objective : *summary.objs()) {
    cout << "  Objective #" << count++ << ":" << endl;
    cout << "    Name: " << *objective.name() << endl;
    cout << "    Description: " << *objective.desc() << endl;
    cout << "    Weight: " << *objective.weight() << endl;
    cout << "    Value: " << *objective.value() << endl;
  }
}

static void print(const ConstraintSummary& summary) {
  cout << "  Broken value: " << *summary.brokenVal() << endl;
  cout << "  Broken count: " << *summary.brokenCount() << endl;
  int count = 0;
  for (auto& constraint : *summary.constraints()) {
    cout << "  Constraint #" << count++ << ":" << endl;
    cout << "    Name: " << *constraint.name() << endl;
    cout << "    Description: " << *constraint.desc() << endl;
    cout << "    Value: " << *constraint.value() << endl;
  }
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  auto solver = ProblemSolverFactory::makeProblemSolver("rebalancer", "tests");

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

  // Define cpu dimension
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
  solver->addConstraint(std::move(capacitySpec));

  BalanceSpec balanceSpec;
  balanceSpec.scope() = "host";
  balanceSpec.dimension() = "cpu";
  balanceSpec.formula() = BalanceSpecFormula::LEGACY;
  balanceSpec.fixAverageToInitial() = true;
  solver->addGoal(std::move(balanceSpec));

  // Create rack scope
  map<string, string> host_to_rack = {
      {"h1", "r1"}, {"h2", "r1"}, {"h3", "r2"}, {"h4", "r2"}};
  solver->addScope("rack", host_to_rack);

  // Define network dimension
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
  solver->addConstraint(std::move(capacitySpec2));

  // Populate move reasons
  solver->enableMoveStats();

  // Run the solver
  auto solution = solver->solve();

  // Print the solution
  cout << "Final assignment:" << endl;
  for (auto& it : *solution.assignment()) {
    cout << "  " << it.first << ": " << it.second << " "
         << host_to_rack.at(it.second) << endl;
  }
  cout << endl;

  // Print initial/final objective/constraint summaries
  cout << "Initial objective:" << endl;
  print(*solution.initialObjective());
  cout << endl;
  cout << "Initial constraint:" << endl;
  print(*solution.initialConstraint());
  cout << endl;
  cout << "Final objective:" << endl;
  print(*solution.finalObjective());
  cout << endl;
  cout << "Final constraint:" << endl;
  print(*solution.finalConstraint());
  cout << endl;

  // Print moves summary, if available
  if (solution.movesSummary()) {
    auto& movesSummary = *solution.movesSummary();
    int count = 0;
    for (auto& moveset : movesSummary) {
      cout << "Moveset #" << ++count << ":" << endl;
      cout << "  Moves:";
      for (auto& move : *moveset.moves()) {
        cout << " " << *move.object() << " " << *move.srcContainer() << " -> "
             << *move.dstContainer();
      }
      cout << endl;
      cout << "  Objective delta:" << endl;
      for (auto& objective : *moveset.objectives()) {
        cout << "    " << objective.first << ": " << *objective.second.change()
             << endl;
      }
    }
  }
  cout << endl;

  {
    int count = 0;
    for (auto& solverSummary : *solution.solverSummaries()) {
      cout << "Solver #" << ++count << ":" << endl;
      cout << "  End reason: ";
      switch (*solverSummary.endReason()) {
        case EndReason::OPTIMAL:
          cout << "found optimal" << endl;
          break;
        case EndReason::HIT_TIME_LIMIT:
          cout << "hit time limit" << endl;
          break;
        case EndReason::HIT_MOVE_LIMIT:
          cout << "hit move limit" << endl;
          break;
        case EndReason::HIT_STOP_CONSTRAINT:
          cout << "hit stop constraint" << endl;
          break;
        default:
          cout << "unknown" << endl;
      }
    }
  }

  return 0;
}
