#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Bandwidth-Aware Throttling Variation

Demonstrates how to limit moves based on available network bandwidth rather than
a simple count of objects. This is useful for:
- Respecting network capacity constraints
- Calculating data volume that can be transferred in a time window
- Using bandwidth as the limiting factor rather than object count
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
def bandwidth_aware_rebalancing(max_bandwidth_gbps=10):
    """Limit moves based on available bandwidth."""
    # Calculate how much data can be moved in time window
    time_window_hours = 1.0
    max_data_gb = max_bandwidth_gbps * time_window_hours * 3600  # GB

    # Use as allowance in MinimizeMovementSpec
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="bandwidth-limit",
                scope="host",
                dimension="data_size",
                allowance=max_data_gb,  # Max GB to move
            )
        ),
        weight=10.0,  # High weight - enforce bandwidth limit
    )


# variation_end


def bandwidth_aware_rebalancing_complete():
    """Complete example with bandwidth-aware throttling."""
    MAX_BANDWIDTH_GBPS = 10
    TIME_WINDOW_HOURS = 1.0
    SHARD_COUNT = 500
    HOST_COUNT = 50

    # Calculate max data that can be moved
    max_data_gb = MAX_BANDWIDTH_GBPS * TIME_WINDOW_HOURS * 3600  # GB

    solver = ProblemSolver(
        service_name="bandwidth-aware-rebalancer", service_scope="production"
    )

    solver.setObjectName("shard")
    solver.setContainerName("host")

    # Initialize with imbalanced cluster
    current_assignment = initialize_imbalanced_cluster(SHARD_COUNT, HOST_COUNT)
    solver.setAssignment(current_assignment)

    # Add dimensions
    shard_sizes = {
        f"shard{i}": 10.0 + random.random() * 90.0 for i in range(SHARD_COUNT)
    }
    solver.addObjectDimension("data_size", shard_sizes)

    host_capacity = {f"host{i}": 1000.0 for i in range(HOST_COUNT)}
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

    # Add bandwidth-aware movement limit
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="bandwidth-limit",
                scope="host",
                dimension="data_size",
                allowance=max_data_gb,  # Max GB to move
            )
        ),
        weight=10.0,  # High weight - enforce bandwidth limit
    )

    # Solve
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution = solver.solve()

    print(f"Bandwidth-aware rebalancing complete")
    print(f"  Max bandwidth: {MAX_BANDWIDTH_GBPS} Gbps")
    print(f"  Time window: {TIME_WINDOW_HOURS} hours")
    print(f"  Max data to move: {max_data_gb:.2f} GB")
    print(f"  Objective value: {solution.objectiveValue:.2f}")

    return solution


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
    bandwidth_aware_rebalancing_complete()
