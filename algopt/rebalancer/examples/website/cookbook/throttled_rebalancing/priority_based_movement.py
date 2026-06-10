#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Priority-Based Movement Variation

Demonstrates how to move critical objects first, then non-critical ones in
subsequent rounds. This is useful for:
- Ensuring high-priority workloads are balanced first
- Minimizing risk by moving critical items separately
- Staged rollout of changes
"""

import random

from algopt.rebalancer.interface.py_client.ProblemSolver import ProblemSolver
from rebalancer.interface.thrift.v2.ProblemSolver.thrift_types import (
    ConstraintSpecs,
    GoalSpecs,
    SolverSpecs,
)
from rebalancer.interface.thrift.v2.ProblemSpecs.thrift_types import (
    AvoidMovingSpec,
    BalanceSpec,
    BalanceSpecFormula,
    CapacitySpec,
    MinimizeMovementSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


# variation_start
def priority_based_rebalancing():
    """Rebalance critical shards first, then non-critical."""
    # Round 1: Only critical shards
    critical_shards = get_critical_shards()
    non_critical = get_non_critical_shards()

    # Pin non-critical shards in round 1
    solver.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-non-critical",
                objects=non_critical,
            )
        )
    )

    solution_r1 = solver.solve()
    apply_moves(solution_r1)

    # Round 2: Now rebalance non-critical
    # (critical already moved, will be pinned)
    solver2 = ProblemSolver(...)
    solver2.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-critical",
                objects=critical_shards,  # Don't re-move critical
            )
        )
    )

    solution_r2 = solver2.solve()


# variation_end


def get_critical_shards():
    """Identify critical shards (e.g., high QPS or important databases)."""
    # For demonstration, consider first 100 shards as critical
    return [f"shard{i}" for i in range(100)]


def get_non_critical_shards():
    """Identify non-critical shards."""
    # Remaining shards are non-critical
    return [f"shard{i}" for i in range(100, 500)]


def apply_moves(solution):
    """Apply moves from solution (placeholder)."""
    print(f"  Applied moves from solution")


def priority_based_rebalancing_complete():
    """Complete example with priority-based rebalancing."""
    SHARD_COUNT = 500
    HOST_COUNT = 50

    critical_shards = get_critical_shards()
    non_critical = get_non_critical_shards()

    # Initialize cluster
    current_assignment = initialize_imbalanced_cluster(SHARD_COUNT, HOST_COUNT)

    print("=" * 80)
    print("ROUND 1: Rebalancing critical shards only")
    print("=" * 80)

    # Round 1: Only critical shards can move
    solver1 = ProblemSolver(
        service_name="priority-rebalancer-r1", service_scope="production"
    )

    solver1.setObjectName("shard")
    solver1.setContainerName("host")
    solver1.setAssignment(current_assignment)

    # Add dimensions
    shard_sizes = {
        f"shard{i}": 10.0 + random.random() * 90.0 for i in range(SHARD_COUNT)
    }
    solver1.addObjectDimension("data_size", shard_sizes)

    host_capacity = {f"host{i}": 1000.0 for i in range(HOST_COUNT)}
    solver1.addContainerDimension("host_capacity", host_capacity)

    # Capacity constraint
    solver1.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="host-capacity",
                scope="host",
                dimension="data_size",
            )
        )
    )

    # Pin non-critical shards in round 1
    solver1.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-non-critical",
                objects=non_critical,
            )
        )
    )

    # Balance goal
    solver1.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-data",
                scope="host",
                dimension="data_size",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=2.0,
    )

    # Movement minimization
    solver1.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
                allowance=50 * 50.0,
            )
        ),
        weight=1.0,
    )

    solver1.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution_r1 = solver1.solve()

    print(f"  Objective value: {solution_r1.objectiveValue:.2f}")
    print(f"  Critical shards moved")

    # Update assignment
    current_assignment = solution_r1.assignment

    print("\n" + "=" * 80)
    print("ROUND 2: Rebalancing non-critical shards")
    print("=" * 80)

    # Round 2: Only non-critical shards can move
    solver2 = ProblemSolver(
        service_name="priority-rebalancer-r2", service_scope="production"
    )

    solver2.setObjectName("shard")
    solver2.setContainerName("host")
    solver2.setAssignment(current_assignment)

    # Re-add dimensions
    solver2.addObjectDimension("data_size", shard_sizes)
    solver2.addContainerDimension("host_capacity", host_capacity)

    # Capacity constraint
    solver2.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="host-capacity",
                scope="host",
                dimension="data_size",
            )
        )
    )

    # Pin critical shards (already moved)
    solver2.addConstraint(
        ConstraintSpecs(
            avoidMovingSpec=AvoidMovingSpec(
                name="pin-critical",
                objects=critical_shards,
            )
        )
    )

    # Balance goal
    solver2.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-data",
                scope="host",
                dimension="data_size",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=2.0,
    )

    # Movement minimization
    solver2.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
                allowance=50 * 50.0,
            )
        ),
        weight=1.0,
    )

    solver2.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution_r2 = solver2.solve()

    print(f"  Objective value: {solution_r2.objectiveValue:.2f}")
    print(f"  Non-critical shards moved")

    print("\nPriority-based rebalancing complete!")


def initialize_imbalanced_cluster(shard_count, host_count):
    """Create initial imbalanced assignment."""
    assignment = {f"host{i}": [] for i in range(host_count)}
    for i in range(shard_count):
        if i < 400:
            host_idx = i % 20
        else:
            host_idx = 20 + (i % 30)
        assignment[f"host{host_idx}"].append(f"shard{i}")
    return assignment


if __name__ == "__main__":
    priority_based_rebalancing_complete()
