#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Progress Tracking Example

Demonstrates how to track and visualize rebalancing progress over multiple
rounds. This is useful for:
- Monitoring rebalancing campaigns
- Visualizing improvement over time
- Creating progress reports
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
def track_rebalancing_progress(rounds_data):
    """Visualize rebalancing progress over rounds.

    Args:
        rounds_data: List of dicts with round metrics
    """
    print("\nRebalancing Progress:")
    print(
        f"{'Round':<8} {'Moves':<8} {'Total Moved':<15} {'Objective':<12} {'Balance'}"
    )
    print("-" * 65)

    for i, data in enumerate(rounds_data):
        round_num = i + 1
        moves = data["moves_this_round"]
        total_moved = data["total_moved"]
        objective = data["objective_value"]
        balance_score = data["balance_score"]

        # Progress bar for balance (0-100%)
        progress = "█" * int(balance_score / 10) + "░" * (10 - int(balance_score / 10))

        print(
            f"{round_num:<8} {moves:<8} {total_moved:<15} {objective:<12.2f} {progress} {balance_score:.1f}%"
        )


# Example usage:
# rounds_data = [
#     {'moves_this_round': 50, 'total_moved': 50, 'objective_value': 1000, 'balance_score': 30},
#     {'moves_this_round': 45, 'total_moved': 95, 'objective_value': 600, 'balance_score': 60},
#     ...
# ]
# track_rebalancing_progress(rounds_data)
# variation_end


def calculate_balance_score(assignment, shard_sizes, host_count):
    """Calculate a balance score (0-100%) based on load variance."""
    # Calculate load per host
    host_loads = {}
    for host, shards in assignment.items():
        total_load = sum(shard_sizes.get(shard, 0) for shard in shards)
        host_loads[host] = total_load

    # Calculate variance
    loads = list(host_loads.values())
    avg_load = sum(loads) / len(loads)
    variance = sum((load - avg_load) ** 2 for load in loads) / len(loads)
    std_dev = variance**0.5

    # Convert to balance score (higher is better)
    # Perfect balance = 100%, high variance = lower score
    if avg_load == 0:
        return 100.0
    coefficient_of_variation = std_dev / avg_load
    balance_score = max(0, 100 * (1 - coefficient_of_variation))

    return min(100, balance_score)


def identify_moved_shards(initial_assignment, final_assignment):
    """Identify which shards moved between assignments."""
    moved = set()
    for host, shards in final_assignment.items():
        initial_shards = set(initial_assignment.get(host, []))
        final_shards = set(shards)
        new_arrivals = final_shards - initial_shards
        moved.update(new_arrivals)
    return moved


def progress_tracking_example():
    """Complete example with progress tracking."""
    MAX_MOVES_PER_ROUND = 50
    TOTAL_ROUNDS = 10
    SHARD_COUNT = 500
    HOST_COUNT = 50

    moved_shards_history = set()
    current_assignment = initialize_imbalanced_cluster(SHARD_COUNT, HOST_COUNT)

    # Track progress data
    rounds_data = []

    print("Throttled Rebalancing with Progress Tracking")
    print("=" * 80)

    for _round_num in range(TOTAL_ROUNDS):
        # Solve
        solution = solve_rebalancing_round(
            current_assignment=current_assignment,
            moved_shards_history=moved_shards_history,
            max_moves=MAX_MOVES_PER_ROUND,
            shard_count=SHARD_COUNT,
            host_count=HOST_COUNT,
        )

        # Identify moves
        moved_this_round = identify_moved_shards(
            current_assignment, solution.assignment
        )

        # Update tracking
        moved_shards_history.update(moved_this_round)
        current_assignment = solution.assignment

        # Calculate balance score
        shard_sizes = {
            f"shard{i}": 10.0 + random.random() * 90.0 for i in range(SHARD_COUNT)
        }
        balance_score = calculate_balance_score(
            current_assignment, shard_sizes, HOST_COUNT
        )

        # Record round data
        rounds_data.append(
            {
                "moves_this_round": len(moved_this_round),
                "total_moved": len(moved_shards_history),
                "objective_value": solution.objectiveValue,
                "balance_score": balance_score,
            }
        )

        # Check convergence
        if len(moved_this_round) == 0:
            break

    # Display progress
    track_rebalancing_progress(rounds_data)

    print(f"\nRebalancing complete!")
    print(f"  Total rounds: {len(rounds_data)}")
    print(f"  Total shards moved: {len(moved_shards_history)}/{SHARD_COUNT}")
    print(f"  Final balance score: {rounds_data[-1]['balance_score']:.1f}%")


def solve_rebalancing_round(
    current_assignment,
    moved_shards_history,
    max_moves,
    shard_count,
    host_count,
):
    """Solve one round of rebalancing with move limit."""
    solver = ProblemSolver(
        service_name="progress-tracking-rebalancer", service_scope="production"
    )

    solver.setObjectName("shard")
    solver.setContainerName("host")
    solver.setAssignment(current_assignment)

    # Shard data sizes
    shard_sizes = {
        f"shard{i}": 10.0 + random.random() * 90.0 for i in range(shard_count)
    }
    solver.addObjectDimension("data_size", shard_sizes)

    # Host capacities
    host_capacity = {f"host{i}": 1000.0 for i in range(host_count)}
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

    # Pin recently moved shards
    if moved_shards_history:
        solver.addConstraint(
            ConstraintSpecs(
                avoidMovingSpec=AvoidMovingSpec(
                    name="pin-recently-moved",
                    objects=list(moved_shards_history),
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

    # Minimize movement
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
    progress_tracking_example()
