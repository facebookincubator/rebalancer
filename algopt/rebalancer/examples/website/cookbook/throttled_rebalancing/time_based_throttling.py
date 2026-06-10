#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Time-Based Throttling Variation

Demonstrates how to limit moves per time period instead of per round by adding
delays between rebalancing rounds. This is useful for:
- Spreading moves across real wall-clock time
- Allowing systems to stabilize between rounds
- Respecting operational windows
"""

import random
import time

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
def time_throttled_rebalancing():
    """Rebalance with time delays between rounds."""
    WAIT_BETWEEN_ROUNDS_SEC = 300  # 5 minutes
    MAX_MOVES_PER_ROUND = 20

    for _round_num in range(100):  # Run until converged
        solution = solve_rebalancing_round(
            max_moves=MAX_MOVES_PER_ROUND,
            # ... other params
        )

        moved_count = len(identify_moved_shards(...))

        if moved_count == 0:
            print("Converged!")
            break

        # Apply moves gradually
        apply_moves_gradually(solution, rate_limit_mbps=100)

        # Wait before next round
        print(f"Waiting {WAIT_BETWEEN_ROUNDS_SEC}s before next round...")
        time.sleep(WAIT_BETWEEN_ROUNDS_SEC)


# variation_end


def solve_rebalancing_round(max_moves, **kwargs):
    """Solve one round of rebalancing with move limit."""
    # Simplified for demonstration
    solver = ProblemSolver(
        service_name="time-throttled-rebalancer", service_scope="production"
    )

    solver.setObjectName("shard")
    solver.setContainerName("host")

    # Add basic setup (simplified)
    current_assignment = initialize_imbalanced_cluster(500, 50)
    solver.setAssignment(current_assignment)

    # Add dimensions
    shard_sizes = {f"shard{i}": 10.0 + random.random() * 90.0 for i in range(500)}
    solver.addObjectDimension("data_size", shard_sizes)

    host_capacity = {f"host{i}": 1000.0 for i in range(50)}
    solver.addContainerDimension("host_capacity", host_capacity)

    # Add capacity constraint
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="host-capacity",
                scope="host",
                dimension="data_size",
            )
        )
    )

    # Add balance goal
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

    # Add movement minimization
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


def identify_moved_shards(initial_assignment, final_assignment):
    """Identify which shards moved between assignments."""
    moved = set()
    for host, shards in final_assignment.items():
        initial_shards = set(initial_assignment.get(host, []))
        final_shards = set(shards)
        new_arrivals = final_shards - initial_shards
        moved.update(new_arrivals)
    return moved


def apply_moves_gradually(solution, rate_limit_mbps):
    """Apply moves with rate limiting (placeholder for demonstration)."""
    print(f"  Applying moves with rate limit: {rate_limit_mbps} Mbps")
    # In production, this would actually execute the moves with rate limiting


if __name__ == "__main__":
    time_throttled_rebalancing()
