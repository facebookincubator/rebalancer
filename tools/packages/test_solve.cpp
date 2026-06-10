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
//
// Standalone C++20 package smoke test.  Mirrors TestCapacitySolve from
// tools/wheels/test_bindings.py: 2 hosts, 4 tasks, capacity constraint,
// local-search solver.  Exits 0 on a correct 2-2 split; exits 1 with a
// diagnostic on failure.  No external test framework required.
//
// Compile (after deb install):
//   g++ -std=c++20 test_solve.cpp \
//       -I/usr/local/include -L/usr/local/lib -lrebalancer \
//       -Wl,-rpath,/usr/local/lib -o test_solve
// Compile (from RPM extraction dir $R):
//   g++ -std=c++20 test_solve.cpp \
//       -I$R/usr/local/include -L$R/usr/local/lib -lrebalancer \
//       -o test_solve
//   LD_LIBRARY_PATH=$R/usr/local/lib:$R/usr/local/lib/rebalancer ./test_solve

#include "algopt/rebalancer/interface/ProblemSolverFactory.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/ProblemSpecs_types.h"
#include "algopt/rebalancer/interface/thrift/gen-cpp2/Types_types.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace facebook::rebalancer::interface;

int main(int argc, char *argv[]) {
  try {
    auto solver =
        ProblemSolverFactory::makeProblemSolver("rebalancer", "pkg-test");

    solver->setObjectName("task");
    solver->setContainerName("host");

    // host0 starts overloaded (3 tasks), host1 underloaded (1 task).
    std::map<std::string, std::vector<std::string>> assignment = {
        {"host0", {"task0", "task1", "task2"}},
        {"host1", {"task3"}},
    };
    solver->setAssignment(assignment);

    // Each task consumes 10 GB of memory.
    std::map<std::string, double> taskMemory = {
        {"task0", 10.0},
        {"task1", 10.0},
        {"task2", 10.0},
        {"task3", 10.0},
    };
    solver->addObjectDimension("memory", taskMemory);

    // Each host has 20 GB capacity (default, no per-host override).
    solver->addContainerDimension("memory", std::map<std::string, double>{},
                                  /*defaultValue=*/20.0);

    // Capacity constraint: memory usage must not exceed host capacity.
    CapacitySpec cap;
    cap.name() = "memory_capacity";
    cap.scope() = "host";
    cap.dimension() = "memory";
    solver->addConstraint(cap);

    // Local-search solver with all standard move types.
    LocalSearchSolverSpec lsSpec;
    lsSpec.moveTypeList() = {
        ProblemSolver::makeMoveTypeSpec(SingleMoveTypeSpec{}),
        ProblemSolver::makeMoveTypeSpec(SwapMoveTypeSpec{}),
        ProblemSolver::makeMoveTypeSpec(TripleLoopMoveTypeSpec{}),
        ProblemSolver::makeMoveTypeSpec(KLSearchMoveTypeSpec{}),
    };
    solver->addSolver(lsSpec);

    const auto solution = solver->solve();
    // assignment() maps object -> container.
    const auto &result = *solution.assignment();

    int host0Count = 0;
    int host1Count = 0;
    for (const auto &[task, host] : result) {
      if (host == "host0")
        ++host0Count;
      else if (host == "host1")
        ++host1Count;
    }

    if (static_cast<int>(result.size()) != 4) {
      std::cerr << "FAIL: expected 4 tasks in solution, got " << result.size()
                << "\n";
      return 1;
    }

    if (host0Count == 2 && host1Count == 2) {
      std::cout << "PASS: 2-2 split achieved\n";
      return 0;
    }

    std::cerr << "FAIL: expected 2-2 split, got host0=" << host0Count
              << " host1=" << host1Count << "\n";
    return 1;

  } catch (const std::exception &e) {
    std::cerr << "EXCEPTION: " << e.what() << "\n";
    return 1;
  } catch (...) {
    std::cerr << "UNKNOWN EXCEPTION\n";
    return 1;
  }
}
