# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Rack-Aware Load Balancing

This variation balances at both host and rack levels for fault tolerance.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    BalanceSpecFormula,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def main():
    solver = ProblemSolver(service_name="load-balancer", service_scope="production")

    solver.setObjectName("task")
    solver.setContainerName("host")

    current_assignment = {
        "host0": [f"task{i}" for i in range(40)],
        "host1": [f"task{i}" for i in range(40, 70)],
        "host2": [f"task{i}" for i in range(70, 100)],
        "host3": [],
        "host4": [],
        "host5": [],
        "host6": [],
        "host7": [],
        "host8": [],
        "host9": [],
    }
    solver.setAssignment(current_assignment)

    cpu_usage = {f"task{i}": 1.0 + (i % 5) * 0.5 for i in range(100)}
    solver.addObjectDimension("cpu_usage", cpu_usage)

    memory_usage = {f"task{i}": 2.0 + (i % 3) * 1.0 for i in range(100)}
    solver.addObjectDimension("memory_usage", memory_usage)

    # variation_start
    # Define which rack each host is in
    host_to_rack = {
        "host0": "rack0",
        "host1": "rack0",
        "host2": "rack1",
        "host3": "rack1",
        "host4": "rack2",
        "host5": "rack2",
        "host6": "rack3",
        "host7": "rack3",
        "host8": "rack4",
        "host9": "rack4",
    }
    solver.addScope("rack", host_to_rack)

    # Balance across racks too
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu-across-racks",
                scope="rack",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=0.5,  # Less important than host-level balance
    )
    # variation_end

    # Host-level balancing
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="host",
                dimension="memory_usage",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=1.0,
    )

    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=10000))
    )
    solution = solver.solve()

    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")

    return solution


if __name__ == "__main__":
    main()
