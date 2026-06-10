#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Multi-Objective Optimization: Hybrid Approach (Tuples + Weights)

This example demonstrates how to combine tuples and weights for fine-grained
control. Use tuples to define strict priority levels, then use weights to
balance multiple objectives within each priority level.

Use this approach when:
- You want the best of both worlds
- You need flexible yet predictable behavior
- You want fine-grained control over complex objective hierarchies
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
    Priority,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)

# solution_start


def multi_objective_hybrid():
    """Use tuples for major priorities, weights within each tuple."""
    solver = ProblemSolver(
        service_name="storage-rebalancer", service_scope="production"
    )

    solver.setObjectName("storage_node")
    solver.setContainerName("host")

    # ... set up assignment and dimensions ...

    # TUPLE 0: Critical objectives (both equally important)
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=2.0,  # Slightly prefer CPU
        priority=Priority(tuple=0),
    )

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="host",
                dimension="memory_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=1.0,  # Within same tuple as CPU
        priority=Priority(tuple=0),
    )

    # TUPLE 1: Secondary objectives
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
            )
        ),
        weight=1.0,
        priority=Priority(tuple=1),
    )

    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution = solver.solve()

    return solution


# solution_end


if __name__ == "__main__":
    solution = multi_objective_hybrid()
    print(f"Solution found with objective value: {solution.objectiveValue}")
