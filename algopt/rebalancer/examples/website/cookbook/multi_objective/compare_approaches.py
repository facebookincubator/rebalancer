#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Multi-Objective Optimization: Comparing Different Approaches

This example demonstrates how to compare different multi-objective optimization
approaches (weighted sum, lexicographic tuples, and hybrid) to understand their
trade-offs and select the best approach for your use case.
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

# variation_start


def multi_objective_weighted():
    """Use weighted sum of objectives."""
    solver = ProblemSolver(
        service_name="storage-rebalancer", service_scope="production"
    )

    solver.setObjectName("storage_node")
    solver.setContainerName("host")

    # ... set up assignment and dimensions ...

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=10.0,
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
        weight=5.0,
    )

    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
            )
        ),
        weight=1.0,
    )

    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    return solver.solve()


def multi_objective_tuples():
    """Use tuple optimization for strict priorities."""
    solver = ProblemSolver(
        service_name="storage-rebalancer", service_scope="production"
    )

    solver.setObjectName("storage_node")
    solver.setContainerName("host")

    # ... set up assignment and dimensions ...

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
        weight=1.0,
        priority=Priority(tuple=1),
    )

    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
            )
        ),
        weight=1.0,
        priority=Priority(tuple=2),
    )

    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    return solver.solve()


def multi_objective_hybrid():
    """Use tuples for major priorities, weights within each tuple."""
    solver = ProblemSolver(
        service_name="storage-rebalancer", service_scope="production"
    )

    solver.setObjectName("storage_node")
    solver.setContainerName("host")

    # ... set up assignment and dimensions ...

    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-cpu",
                scope="host",
                dimension="cpu_usage",
                formula=BalanceSpecFormula.LEGACY,
            )
        ),
        weight=2.0,
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
        weight=1.0,
        priority=Priority(tuple=0),
    )

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
    return solver.solve()


def compare_approaches():
    # Weighted approach
    solution1 = multi_objective_weighted()

    # Tuple approach
    solution2 = multi_objective_tuples()

    # Hybrid approach
    solution3 = multi_objective_hybrid()

    # Compare objectives
    print("Approach | CPU Balance | Memory Balance | Data Moved")
    print("-" * 60)
    # TODO: Extract and compare metrics


# variation_end


if __name__ == "__main__":
    compare_approaches()
