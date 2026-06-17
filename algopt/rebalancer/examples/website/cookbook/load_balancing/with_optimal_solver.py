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
Load Balancing with Optimal Solver

For smaller problems (<50 objects), use the optimal solver to get
provably optimal solutions.
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
    OptimalSolverPackage,
    OptimalSolverSpec,
)


def main():
    solver = ProblemSolver(service_name="load-balancer", service_scope="production")

    solver.setObjectName("task")
    solver.setContainerName("host")

    # Smaller problem for optimal solver
    current_assignment = {
        "host0": [f"task{i}" for i in range(15)],
        "host1": [f"task{i}" for i in range(15, 25)],
        "host2": [f"task{i}" for i in range(25, 40)],
        "host3": [],
        "host4": [],
    }
    solver.setAssignment(current_assignment)

    cpu_usage = {f"task{i}": 1.0 + (i % 5) * 0.5 for i in range(40)}
    solver.addObjectDimension("cpu_usage", cpu_usage)

    memory_usage = {f"task{i}": 2.0 + (i % 3) * 1.0 for i in range(40)}
    solver.addObjectDimension("memory_usage", memory_usage)

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
    solver.addSolver(
        SolverSpecs(
            optimalSolverSpec=OptimalSolverSpec(
                solverPackage=OptimalSolverPackage.HIGHS,
                timeLimitMs=60000,  # 1 minute
            )
        )
    )
    # variation_end

    solution = solver.solve()

    print(f"Solution found in {solution.profile.solveTime}ms")
    print(f"Objective value: {solution.objectiveValue}")

    return solution


if __name__ == "__main__":
    main()
