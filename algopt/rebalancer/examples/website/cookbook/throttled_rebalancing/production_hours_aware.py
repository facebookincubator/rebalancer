#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Production-Hours Aware Variation

Demonstrates how to only run rebalancing during low-traffic/off-peak hours.
This is useful for:
- Respecting operational windows
- Avoiding disruption during peak traffic
- Ensuring moves happen only when system can handle them
"""

import random
import time
from datetime import datetime

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
def production_hours_aware_rebalancing():
    """Only run rebalancing during off-peak hours."""
    while not is_fully_balanced():
        current_hour = datetime.now().hour

        # Only rebalance 2am-6am
        if 2 <= current_hour < 6:
            solution = solve_rebalancing_round(max_moves=50)
            apply_moves(solution)
            print(
                f"Rebalancing at {datetime.now()}: {solution.profile.moveCount} moves"
            )
        else:
            print(f"Waiting for off-peak hours (current: {current_hour}:00)")
            time.sleep(3600)  # Wait 1 hour


# variation_end


def is_fully_balanced():
    """Check if cluster is balanced (placeholder)."""
    # In real implementation, would check balance metrics
    return False


def solve_rebalancing_round(max_moves):
    """Solve one round of rebalancing."""
    SHARD_COUNT = 500
    HOST_COUNT = 50

    solver = ProblemSolver(
        service_name="production-hours-rebalancer", service_scope="production"
    )

    solver.setObjectName("shard")
    solver.setContainerName("host")

    # Initialize
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


def apply_moves(solution):
    """Apply moves from solution (placeholder)."""
    print(f"  Applied moves from solution")


def production_hours_aware_rebalancing_demo():
    """Demonstration of production-hours aware rebalancing (simulated)."""
    print("Production-Hours Aware Rebalancing")
    print("=" * 80)
    print("This example demonstrates time-based rebalancing")
    print("In production, this would wait for off-peak hours (2am-6am)")
    print("")

    # For demo purposes, simulate a few rounds
    for round_num in range(3):
        # Simulate checking time
        current_hour = datetime.now().hour
        print(f"\nRound {round_num + 1}")
        print(f"  Current time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"  Current hour: {current_hour}")

        # In real implementation, would check if in off-peak window
        # For demo, we'll just run it
        print(f"  Status: Running rebalancing (demo mode)")

        solution = solve_rebalancing_round(max_moves=50)
        print(f"  Objective value: {solution.objectiveValue:.2f}")
        print(f"  Solve time: {solution.profile.solveTime}ms")

        # In production, would sleep until next off-peak window
        print(f"  In production: would wait until next off-peak window")

    print("\nDemo complete!")


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
    production_hours_aware_rebalancing_demo()
