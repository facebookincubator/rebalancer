#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Multi-Objective Optimization: Lexicographic Tuples Approach

This example demonstrates how to optimize objectives in strict priority order
using tuples. The solver optimizes each priority level completely before
considering the next level.

Use this approach when:
- You need guarantees on minimum quality for high-priority goals
- You're willing to completely ignore low-priority goals if needed
- You have true priority levels (e.g., correctness > performance > cost)
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


def multi_objective_tuples():
    """Use tuple optimization for strict priorities."""
    solver = ProblemSolver(
        service_name="storage-rebalancer", service_scope="production"
    )

    solver.setObjectName("storage_node")
    solver.setContainerName("host")

    # ... set up assignment and dimensions ...

    # PRIORITY 0 (highest): Balance CPU
    # Optimize this FIRST, then move to next priority
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=1.0,
        priority=Priority(tuple=0),  # Highest priority level
    )

    # PRIORITY 1: Balance memory
    # Only optimize AFTER CPU is optimized
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-memory",
                scope="host",
                dimension="memory_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=1.0,
        priority=Priority(tuple=1),  # Second priority level
    )

    # PRIORITY 2 (lowest): Minimize movement
    # Only matters after CPU and memory are optimized
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
            )
        ),
        weight=1.0,
        priority=Priority(tuple=2),  # Lowest priority level
    )

    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution = solver.solve()

    return solution


# solution_end


if __name__ == "__main__":
    solution = multi_objective_tuples()
    print(f"Solution found with objective value: {solution.objectiveValue}")
