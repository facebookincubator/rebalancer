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

#include "algopt/rebalancer/interface/ProblemSolver.h"
#include "algopt/rebalancer/interface/tests/utils.h"

#include <folly/Benchmark.h>
#include <folly/container/irange.h>
#include <folly/init/Init.h>
#include <folly/Random.h>

namespace interface = facebook::rebalancer::interface;

static void run(int containerCount, int tasksPerHost) {
  /*
  In this example, there are 'containerCount' number of hosts, where all hosts
  expect two random ones have 'tasksPerHost' number of tasks in them. The two
  random hosts have ('tasksPerHost' + 1) number of tasks each.

  There are also two special tasks, "specialEvenHostTask" and
  "specialOddHostTask"; all other tasks are classified as "usual".

  The tasks are partitioned into two groups: "tasksInOddHosts" and
  "tasksInEvenHosts".

  + The group "tasksInOddHosts" consists of all "usual" tasks that have been
  assigned to an host with an odd index (henceforth referred as an 'odd host')
  and a "specialOddHostTask", which has been deliberately placed in a random
  host with even index (henceforth referred as an 'even host').

  + Similarly, the group "tasksInEvenHosts" consists of all "usual" tasks that
  have been assigned to an even host and the "specialEvenHostTask", which has
  been deliberately placed in a random odd host.

  The only constraint in the problem is one added by ExclusiveGroupsSpec, where
  we expect that the optimal solution is to place all tasks in group
  "tasksInOddHosts" on odd hosts and all tasks in group "tasksInEvenHosts" on
  even hosts.

  Note that to achieve this, the solver has to perform exactly two moves: move
  "specialEvenHostTask" from its initial host (which is odd) to an even host,
  and move "specialOddHostTask" from its initial host (which is even) to an
  odd host.
  */
  auto solver = interface::initializeTestProblemSolver();
  solver->setObjectName("task");
  solver->setContainerName("host");

  folly::F14FastSet<std::string> tasksInOddHosts;
  folly::F14FastSet<std::string> tasksInEvenHosts;
  {
    std::map<std::string, std::vector<std::string>> assignment;
    for (const auto i : folly::irange(containerCount)) {
      auto hostName = fmt::format("host{}", i);
      for (const auto j : folly::irange(tasksPerHost)) {
        auto taskName = fmt::format("task{}-{}", i, j);
        assignment[hostName].push_back(taskName);

        (i % 2 == 0) ? tasksInEvenHosts.emplace(taskName)
                     : tasksInOddHosts.emplace(taskName);
      }
    }

    // misplace specialEvenHostTask in odd host and specialOddHostTask in even
    // host
    const std::string specialEvenHostTask = "specialEvenHostTask";
    const std::string specialOddHostTask = "specialOddHostTask1";
    auto randomOddContainer = folly::Random::rand32(containerCount / 2) * 2 + 1;
    auto randomEvenContainer = folly::Random::rand32(containerCount / 2) * 2;

    assignment[fmt::format("host{}", randomOddContainer)].push_back(
        specialEvenHostTask);
    assignment[fmt::format("host{}", randomEvenContainer)].push_back(
        specialOddHostTask);

    solver->setAssignment(assignment);
  }

  {
    // there are two scopeItems "odd" and "even"
    std::map<std::string, std::string> containerToScopeItem;
    for (const auto i : folly::irange(containerCount)) {
      containerToScopeItem[fmt::format("host{}", i)] =
          (i % 2 == 0) ? "even" : "odd";
    }

    solver->addScope("host-parity", containerToScopeItem);
  }

  {
    // create an objectPartition, consisting of two groups: "tasksInOddHosts",
    // "tasksInEvenHosts"
    std::map<std::string, std::string> objectToGroup;
    for (const auto& task : tasksInOddHosts) {
      objectToGroup[task] = "tasksInOddHosts";
    }
    for (const auto& task : tasksInEvenHosts) {
      objectToGroup[task] = "tasksInEvenHosts";
    }

    objectToGroup["specialEvenHostTask"] = "tasksInEvenHosts";
    objectToGroup["specialOddHostTask1"] = "tasksInOddHosts";

    solver->addPartition("task-to-host-parity", std::move(objectToGroup));
  }

  {
    interface::ExclusiveGroupsSpec spec;
    spec.scope() = "host-parity";
    spec.dimension() = "task_count";
    spec.partitionName() = "task-to-host-parity";

    interface::LocalSearchSolverSpec solverSpec;
    solverSpec.moveTypeList()->push_back(
        interface::ProblemSolver::makeMoveTypeSpec(
            interface::SingleMoveTypeSpec()));
    solver->addSolver(solverSpec);

    solver->addConstraint(std::move(spec));
  }

  solver->solve();
}

BENCHMARK(FormulateUsingContainerUtils) {
  /*
   This benchmark shows why while formulate ExclusiveGroupsSpec, instead of
   directly computing the util(scopeItem), which would involve a single
   objectLookup, we split it into lookups across individual containers so that
   if, for example, only one of the containers in the scopeItem has a violation,
   then local search can accurately pin-point to that container, rather than
   have to look at all containers in the scopeItem.
   */
  run(20e3 /*containerCount*/, 10 /*tasksPerHost*/);
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  folly::runBenchmarks();

  return 0;
}
