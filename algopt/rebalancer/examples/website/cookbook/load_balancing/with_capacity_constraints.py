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
Load Balancing with Capacity Constraints

This variation adds capacity constraints to ensure hosts don't exceed their limits.
Useful when hosts have hard limits on CPU or memory they can support.
"""

# pyre-strict
from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    BalanceSpec,
    BalanceSpecFormula,
    CapacitySpec,
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

    # Define CPU usage for each task
    cpu_usage = {f"task{i}": 1.0 + (i % 5) * 0.5 for i in range(100)}
    solver.addObjectDimension("cpu_usage", cpu_usage)

    # Define memory usage for each task
    memory_usage = {f"task{i}": 2.0 + (i % 3) * 1.0 for i in range(100)}
    solver.addObjectDimension("memory_usage", memory_usage)

    # variation_start
    # Define host capacities
    host_cpu_capacity = {f"host{i}": 25.0 for i in range(10)}
    host_mem_capacity = {f"host{i}": 32.0 for i in range(10)}  # GB

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_mem_capacity)

    # Add capacity constraints
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity",
                scope="host",
                dimension="cpu_usage",
            )
        )
    )

    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity",
                scope="host",
                dimension="memory_usage",
            )
        )
    )
    # variation_end

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

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=10000))
    )

    # Solve
    solution = solver.solve()

    # Print results
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")

    return solution


if __name__ == "__main__":
    main()
