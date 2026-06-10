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
Basic Load Balancing Tasks Across Servers

This example demonstrates how to balance worker tasks across physical hosts
by distributing CPU and memory load evenly.

Problem: 100 worker processes need to run on 10 physical hosts. Each worker
has different CPU and memory requirements. The current placement is imbalanced.

Goal: Redistribute workers to balance load across all hosts.
"""

# solution_start
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
    # Create solver
    solver = ProblemSolver(service_name="load-balancer", service_scope="production")

    # Define objects and containers
    solver.setObjectName("task")
    solver.setContainerName("host")

    # Current assignment (example: all tasks on first 3 hosts)
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
    cpu_usage = {
        f"task{i}": 1.0 + (i % 5) * 0.5  # Varies from 1.0 to 3.0
        for i in range(100)
    }
    solver.addObjectDimension("cpu_usage", cpu_usage)

    # Define memory usage for each task
    memory_usage = {
        f"task{i}": 2.0 + (i % 3) * 1.0  # Varies from 2.0 to 4.0 GB
        for i in range(100)
    }
    solver.addObjectDimension("memory_usage", memory_usage)

    # Goal 1: Balance CPU across hosts
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

    # Goal 2: Balance memory across hosts
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
        SolverSpecs(
            localSearchSolverSpec=LocalSearchSolverSpec(
                timeLimitMs=10000,  # 10 second time limit
            )
        )
    )

    # Solve
    solution = solver.solve()

    # Analyze solution
    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")
    print(f"Objects moved: {solution.profile.moveCount}")

    # Print assignment summary
    for host in sorted(solution.assignment.keys()):
        tasks = solution.assignment[host]
        total_cpu = sum(cpu_usage.get(t, 0) for t in tasks)
        total_mem = sum(memory_usage.get(t, 0) for t in tasks)
        print(
            f"{host}: {len(tasks)} tasks, CPU={total_cpu:.1f}, Memory={total_mem:.1f}GB"
        )

    return solution


if __name__ == "__main__":
    main()
# solution_end
