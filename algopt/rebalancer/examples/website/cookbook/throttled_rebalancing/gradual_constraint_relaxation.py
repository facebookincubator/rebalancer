#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Gradual Constraint Relaxation Variation

Demonstrates how to start with strict movement limits and gradually relax them
over multiple rounds. This is useful for:
- Conservative initial moves followed by more aggressive rebalancing
- Exponentially increasing move allowances
- Testing system capacity incrementally
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
def gradual_relaxation_rebalancing():
    """Gradually increase move allowance each round."""
    base_allowance = 100.0  # GB

    for round_num in range(10):
        # Exponentially increase allowance
        allowance = base_allowance * (1.5**round_num)

        solver.addGoal(
            GoalSpecs(
                minimizeMovementSpec=MinimizeMovementSpec(
                    name="movement-limit",
                    scope="host",
                    dimension="data_size",
                    allowance=allowance,
                )
            ),
            weight=2.0,
        )

        solution = solver.solve()
        print(f"Round {round_num + 1}: Allowance = {allowance:.1f}GB")


# variation_end


def gradual_relaxation_rebalancing_complete():
    """Complete example with gradual constraint relaxation."""
    BASE_ALLOWANCE = 100.0  # GB
    TOTAL_ROUNDS = 10
    SHARD_COUNT = 500
    HOST_COUNT = 50

    current_assignment = initialize_imbalanced_cluster(SHARD_COUNT, HOST_COUNT)

    print("Starting gradual relaxation rebalancing")
    print("=" * 80)

    for round_num in range(TOTAL_ROUNDS):
        # Exponentially increase allowance
        allowance = BASE_ALLOWANCE * (1.5**round_num)

        print(f"\nRound {round_num + 1}/{TOTAL_ROUNDS}")
        print(f"  Allowance: {allowance:.1f} GB")

        solver = ProblemSolver(
            service_name="gradual-relaxation-rebalancer", service_scope="production"
        )

        solver.setObjectName("shard")
        solver.setContainerName("host")
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

        # Movement minimization with increasing allowance
        solver.addGoal(
            GoalSpecs(
                minimizeMovementSpec=MinimizeMovementSpec(
                    name="movement-limit",
                    scope="host",
                    dimension="data_size",
                    allowance=allowance,
                )
            ),
            weight=2.0,
        )

        # Solve
        solver.addSolver(
            SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
        )
        solution = solver.solve()

        # Update assignment
        current_assignment = solution.assignment

        print(f"  Objective value: {solution.objectiveValue:.2f}")
        print(f"  Solve time: {solution.profile.solveTime}ms")

    print("\nGradual relaxation rebalancing complete!")


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
    gradual_relaxation_rebalancing_complete()
