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
Load Balancing with Minimize Movement

This variation prefers solutions that move fewer tasks, reducing disruption
during rebalancing.
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
    MinimizeMovementSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


def main():
    # Create solver
    solver = ProblemSolver(service_name="load-balancer", service_scope="production")

    # Define objects and containers
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Current assignment
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

    # Define dimensions
    cpu_usage = {f"task{i}": 1.0 + (i % 5) * 0.5 for i in range(100)}
    solver.addObjectDimension("cpu_usage", cpu_usage)

    memory_usage = {f"task{i}": 2.0 + (i % 3) * 1.0 for i in range(100)}
    solver.addObjectDimension("memory_usage", memory_usage)

    # Goals
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

    # variation_start
    solver.addGoal(
        GoalSpecs(minimizeMovementSpec=MinimizeMovementSpec(name="minimize-moves")),
        weight=0.1,  # Lower priority than balancing
    )
    # variation_end

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=10000))
    )

    # Solve
    solution = solver.solve()

    # Print results
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Objects moved: {solution.profile.moveCount}")

    return solution


if __name__ == "__main__":
    main()
