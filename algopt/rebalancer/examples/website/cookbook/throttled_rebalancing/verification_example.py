#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Verification Example

Demonstrates how to verify that move limits are being respected during throttled
rebalancing. This is useful for:
- Validating throttling configuration
- Debugging unexpected behavior
- Ensuring production safety limits are enforced
"""

import random

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
    MinimizeMovementSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


# variation_start
def verify_throttling(
    initial_assignment,
    final_assignment,
    max_moves_expected,
    tolerance=1.1,  # Allow 10% overage
):
    """Verify move count within throttle limit."""
    moved_shards = identify_moved_shards(initial_assignment, final_assignment)
    actual_moves = len(moved_shards)

    max_allowed = int(max_moves_expected * tolerance)

    if actual_moves <= max_allowed:
        print(
            f"✅ Throttling respected: {actual_moves} moves "
            f"(limit: {max_moves_expected}, allowed: {max_allowed})"
        )
        return True
    else:
        print(
            f"❌ Throttling violated: {actual_moves} moves "
            f"(limit: {max_moves_expected})"
        )
        print(f"   Excess: {actual_moves - max_moves_expected} moves")
        return False


# variation_end


def identify_moved_shards(initial_assignment, final_assignment):
    """Identify which shards moved between assignments."""
    moved = set()
    for host, shards in final_assignment.items():
        initial_shards = set(initial_assignment.get(host, []))
        final_shards = set(shards)
        new_arrivals = final_shards - initial_shards
        moved.update(new_arrivals)
    return moved


def verification_example_complete():
    """Complete example demonstrating throttling verification."""
    SHARD_COUNT = 500
    HOST_COUNT = 50
    MAX_MOVES = 50

    print("Throttling Verification Example")
    print("=" * 80)

    # Initialize cluster
    initial_assignment = initialize_imbalanced_cluster(SHARD_COUNT, HOST_COUNT)

    print(f"\nRunning rebalancing with max_moves = {MAX_MOVES}")

    # Solve rebalancing
    solver = ProblemSolver(
        service_name="verification-rebalancer", service_scope="production"
    )

    solver.setObjectName("shard")
    solver.setContainerName("host")
    solver.setAssignment(initial_assignment)

    # Add dimensions
    shard_sizes = {
        f"shard{i}": 10.0 + random.random() * 90.0 for i in range(SHARD_COUNT)
    }
    solver.addObjectDimension("data_size", shard_sizes)

    host_capacity = {f"host{i}": 1000.0 for i in range(HOST_COUNT)}
    solver.addContainerDimension("host_capacity", host_capacity)

    # Capacity constraint
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="host-capacity",
                scope="host",
                dimension="data_size",
            )
        )
    )

    # Balance goal
    solver.addGoal(
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
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
                allowance=MAX_MOVES * 50.0,
            )
        ),
        weight=1.0,
    )

    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution = solver.solve()

    final_assignment = solution.assignment

    print(f"Rebalancing complete")
    print(f"  Objective value: {solution.objectiveValue:.2f}")
    print(f"  Solve time: {solution.profile.solveTime}ms")

    # Verify throttling
    print("\nVerifying throttling limits:")
    verify_throttling(
        initial_assignment,
        final_assignment,
        max_moves_expected=MAX_MOVES,
        tolerance=1.1,  # Allow 10% overage
    )

    # Additional verification with stricter tolerance
    print("\nVerifying with stricter tolerance (5%):")
    verify_throttling(
        initial_assignment,
        final_assignment,
        max_moves_expected=MAX_MOVES,
        tolerance=1.05,  # Allow 5% overage
    )


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
    verification_example_complete()
