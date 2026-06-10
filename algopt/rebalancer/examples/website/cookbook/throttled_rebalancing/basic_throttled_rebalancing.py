#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Basic Throttled Rebalancing Example

Demonstrates how to gradually rebalance shards over multiple rounds with strict
move limits to avoid overwhelming production systems. This example shows:
- Multi-round rebalancing with movement throttling
- Pinning recently moved shards to prevent ping-pong behavior
- Balancing multiple dimensions (data size and query rate)
- Tracking progress across rounds
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

# solution_start


def throttled_rebalancing():
    """Gradually rebalance shards over multiple rounds."""
    MAX_MOVES_PER_ROUND = 50  # Limit moves to avoid overwhelming system
    TOTAL_ROUNDS = 10
    SHARD_COUNT = 500
    HOST_COUNT = 50

    # Track what's been moved across all rounds
    moved_shards_history = set()
    current_assignment = initialize_imbalanced_cluster(SHARD_COUNT, HOST_COUNT)

    print(
        f"Starting throttled rebalancing ({TOTAL_ROUNDS} rounds, max {MAX_MOVES_PER_ROUND} moves/round)"
    )
    print("=" * 80)

    for round_num in range(TOTAL_ROUNDS):
        print(f"\nRound {round_num + 1}/{TOTAL_ROUNDS}")
        print("-" * 40)

        solution = solve_rebalancing_round(
            current_assignment=current_assignment,
            moved_shards_history=moved_shards_history,
            max_moves=MAX_MOVES_PER_ROUND,
            round_num=round_num,
            shard_count=SHARD_COUNT,
            host_count=HOST_COUNT,
        )

        # Identify shards moved in this round
        moved_this_round = identify_moved_shards(
            current_assignment, solution.assignment
        )

        # Update tracking
        moved_shards_history.update(moved_this_round)
        current_assignment = solution.assignment

        # Analyze round results
        print(f"  Moves this round: {len(moved_this_round)}")
        print(f"  Total moves so far: {len(moved_shards_history)}")
        print(f"  Objective value: {solution.objectiveValue:.2f}")
        print(f"  Solve time: {solution.profile.solveTime}ms")

        # Check if done
        if len(moved_this_round) == 0:
            print(
                f"\n✅ Converged after {round_num + 1} rounds (no more beneficial moves)"
            )
            break

    print(f"\nThrottled rebalancing complete!")
    print(f"  Total rounds: {round_num + 1}")
    print(f"  Total shards moved: {len(moved_shards_history)}/{SHARD_COUNT}")
    print(f"  Move rate: {(len(moved_shards_history) / SHARD_COUNT) * 100:.1f}%")

    return current_assignment


def solve_rebalancing_round(
    current_assignment,
    moved_shards_history,
    max_moves,
    round_num,
    shard_count,
    host_count,
):
    """Solve one round of rebalancing with move limit."""
    solver = ProblemSolver(
        service_name="throttled-rebalancer", service_scope="production"
    )

    solver.setObjectName("shard")
    solver.setContainerName("host")
    solver.setAssignment(current_assignment)

    # Shard data sizes (varies 10-100 GB)
    shard_sizes = {
        f"shard{i}": 10.0 + random.random() * 90.0 for i in range(shard_count)
    }
    solver.addObjectDimension("data_size", shard_sizes)

    # Shard query rates (varies 100-1000 qps)
    shard_qps = {
        f"shard{i}": 100.0 + random.random() * 900.0 for i in range(shard_count)
    }
    solver.addObjectDimension("query_rate", shard_qps)

    # Host capacities (uniform)
    host_capacity = {f"host{i}": 1000.0 for i in range(host_count)}  # 1TB each
    solver.addContainerDimension("host_capacity", host_capacity)

    # CONSTRAINT 1: Host capacity
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="host-capacity",
                scope="host",
                dimension="data_size",
            )
        )
    )

    # CONSTRAINT 2: Pin recently moved shards (avoid ping-pong)
    if moved_shards_history:
        solver.addConstraint(
            ConstraintSpecs(
                avoidMovingSpec=AvoidMovingSpec(
                    name="pin-recently-moved",
                    objects=list(moved_shards_history),
                )
            )
        )

    # GOAL 1: Balance data size across hosts
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

    # GOAL 2: Balance query load across hosts
    solver.addGoal(
        GoalSpecs(
            balanceSpec=BalanceSpec(
                name="balance-queries",
                scope="host",
                dimension="query_rate",
                formula=BalanceSpecFormula.LEGACY,
                fixAverageToInitial=True,
            )
        ),
        weight=2.0,
    )

    # GOAL 3: Minimize movement (with allowance for max moves)
    # Allowance allows limited movement without penalty
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="data_size",
                allowance=max_moves
                * 50.0,  # Allow moving max_moves shards of avg size 50GB
            )
        ),
        weight=1.0,
    )

    # GOAL 4: Prefer moving small shards (less disruptive)
    # This is implicit in MinimizeMovementSpec using data_size dimension
    # Moving a 10GB shard costs less than moving 100GB shard

    # Use Local Search solver
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )

    # Solve
    solution = solver.solve()
    return solution


def initialize_imbalanced_cluster(shard_count, host_count):
    """Create initial imbalanced assignment."""
    assignment = {f"host{i}": [] for i in range(host_count)}

    # Create imbalanced distribution
    # Put most shards on first 20 hosts, leaving last 30 underutilized
    for i in range(shard_count):
        if i < 400:
            # 80% of shards on first 20 hosts (overloaded)
            host_idx = i % 20
        else:
            # 20% of shards on last 30 hosts (underutilized)
            host_idx = 20 + (i % 30)
        assignment[f"host{host_idx}"].append(f"shard{i}")

    return assignment


def identify_moved_shards(initial_assignment, final_assignment):
    """Identify which shards moved between assignments."""
    moved = set()

    for host, shards in final_assignment.items():
        initial_shards = set(initial_assignment.get(host, []))
        final_shards = set(shards)

        # Shards that arrived at this host
        new_arrivals = final_shards - initial_shards
        moved.update(new_arrivals)

    return moved


if __name__ == "__main__":
    throttled_rebalancing()
# solution_end
