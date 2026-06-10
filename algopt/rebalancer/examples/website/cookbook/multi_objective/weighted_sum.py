#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Multi-Objective Optimization: Weighted Sum Approach

This example demonstrates how to balance multiple conflicting objectives using
weighted sums. Different weights allow you to control the trade-off between
objectives - higher weights give more priority to specific goals.

Use this approach when:
- You want smooth trade-offs between objectives
- All goals are somewhat important
- You're willing to accept partial satisfaction of all goals
"""

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

# solution_start


def multi_objective_weighted():
    """Use weighted sum of objectives."""
    solver = ProblemSolver(
        service_name="storage-rebalancer", service_scope="production"
    )

    solver.setObjectName("storage_node")
    solver.setContainerName("host")

    # ... set up assignment and dimensions ...

    # PRIMARY: Balance CPU (highest weight)
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=10.0,  # Highest priority
    )

    # SECONDARY: Balance memory
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="host",
                dimension="memory_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=5.0,  # Medium priority
    )

    # TERTIARY: Minimize movement
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
            )
        ),
        weight=1.0,  # Lowest priority
    )

    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution = solver.solve()

    return solution


# solution_end


if __name__ == "__main__":
    solution = multi_objective_weighted()
    print(f"Solution found with objective value: {solution.objectiveValue}")
