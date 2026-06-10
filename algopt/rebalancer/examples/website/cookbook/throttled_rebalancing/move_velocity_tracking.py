#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Move Velocity Tracking Variation

Demonstrates how to track and limit move velocity over time using a sliding
window approach. This is useful for:
- Preventing bursts of movement
- Smoothing move rate over multiple rounds
- Enforcing aggregate limits over time windows
"""

import random
from collections import deque

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
def velocity_limited_rebalancing():
    """Limit move rate using sliding window."""
    WINDOW_SIZE_ROUNDS = 5
    MAX_MOVES_PER_WINDOW = 150  # Total across 5 rounds

    move_history = deque(maxlen=WINDOW_SIZE_ROUNDS)

    for round_num in range(100):
        # Calculate moves in window
        recent_moves = sum(move_history)

        # Adjust max moves for this round
        remaining_budget = MAX_MOVES_PER_WINDOW - recent_moves
        max_this_round = min(50, max(0, remaining_budget))

        if max_this_round == 0:
            print(f"Round {round_num + 1}: Skipping (velocity limit reached)")
            move_history.append(0)
            continue

        solution = solve_rebalancing_round(max_moves=max_this_round)
        moves_this_round = len(identify_moved_shards(...))
        move_history.append(moves_this_round)

        print(
            f"Round {round_num + 1}: {moves_this_round} moves "
            f"(window total: {sum(move_history)}/{MAX_MOVES_PER_WINDOW})"
        )


# variation_end


def solve_rebalancing_round(
    max_moves, current_assignment=None, moved_shards_history=None
):
    """Solve one round of rebalancing with move limit."""
    SHARD_COUNT = 500
    HOST_COUNT = 50

    solver = ProblemSolver(
        service_name="velocity-limited-rebalancer", service_scope="production"
    )

    solver.setObjectName("shard")
    solver.setContainerName("host")

    # Initialize if needed
    if current_assignment is None:
        current_assignment = initialize_imbalanced_cluster(SHARD_COUNT, HOST_COUNT)

    solver.setAssignment(current_assignment)

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
                allowance=max_moves * 50.0,
            )
        ),
        weight=1.0,
    )

    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    return solver.solve()


def identify_moved_shards(initial_assignment, final_assignment):
    """Identify which shards moved between assignments."""
    moved = set()
    for host, shards in final_assignment.items():
        initial_shards = set(initial_assignment.get(host, []))
        final_shards = set(shards)
        new_arrivals = final_shards - initial_shards
        moved.update(new_arrivals)
    return moved


def velocity_limited_rebalancing_complete():
    """Complete example with velocity-limited rebalancing."""
    WINDOW_SIZE_ROUNDS = 5
    MAX_MOVES_PER_WINDOW = 150  # Total across 5 rounds
    TOTAL_ROUNDS = 20
    SHARD_COUNT = 500
    HOST_COUNT = 50

    move_history = deque(maxlen=WINDOW_SIZE_ROUNDS)
    current_assignment = initialize_imbalanced_cluster(SHARD_COUNT, HOST_COUNT)

    print("Starting velocity-limited rebalancing")
    print(f"  Window size: {WINDOW_SIZE_ROUNDS} rounds")
    print(f"  Max moves per window: {MAX_MOVES_PER_WINDOW}")
    print("=" * 80)

    for round_num in range(TOTAL_ROUNDS):
        # Calculate moves in window
        recent_moves = sum(move_history)

        # Adjust max moves for this round
        remaining_budget = MAX_MOVES_PER_WINDOW - recent_moves
        max_this_round = min(50, max(0, remaining_budget))

        print(f"\nRound {round_num + 1}/{TOTAL_ROUNDS}")
        print(f"  Recent moves in window: {recent_moves}/{MAX_MOVES_PER_WINDOW}")
        print(f"  Remaining budget: {remaining_budget}")
        print(f"  Max moves this round: {max_this_round}")

        if max_this_round == 0:
            print(f"  Status: Skipping (velocity limit reached)")
            move_history.append(0)
            continue

        solution = solve_rebalancing_round(
            max_moves=max_this_round, current_assignment=current_assignment
        )

        # Identify moves
        moved_this_round = identify_moved_shards(
            current_assignment, solution.assignment
        )
        moves_count = len(moved_this_round)

        # Update tracking
        move_history.append(moves_count)
        current_assignment = solution.assignment

        print(f"  Moves this round: {moves_count}")
        print(f"  Window total: {sum(move_history)}/{MAX_MOVES_PER_WINDOW}")
        print(f"  Objective value: {solution.objectiveValue:.2f}")

        # Check convergence
        if moves_count == 0:
            print(f"\nConverged after {round_num + 1} rounds")
            break

    print("\nVelocity-limited rebalancing complete!")


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
    velocity_limited_rebalancing_complete()
