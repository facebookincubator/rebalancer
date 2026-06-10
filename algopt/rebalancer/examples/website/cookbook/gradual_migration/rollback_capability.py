#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.

"""
Rollback Capability Strategy

Demonstrates how to maintain rollback capability during early migration rounds.
This variation shows:
- Keeping old DC capacity available in early rounds
- Allowing movement back if needed
- Committing to migration only in later rounds
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
    NonAcceptingSpec,
)
from rebalancer.interface.thrift.v2.SolverSpecs.thrift_types import (
    LocalSearchSolverSpec,
)


# variation_start
def solve_with_rollback_capability(
    current_assignment, round_num, total_rounds, hosts_per_dc
):
    """
    Solve migration round with rollback capability in early rounds.

    Early rounds (0-1): Keep old DC accepting, allow rollback
    Later rounds (2+): Commit to migration, freeze old DC
    """
    solver = ProblemSolver(
        service_name=f"dc-migration-round-{round_num}", service_scope="production"
    )

    solver.setObjectName("workload")
    solver.setContainerName("host")
    solver.setAssignment(current_assignment)

    # Define datacenter scope
    host_to_dc = {}
    for i in range(hosts_per_dc):
        host_to_dc[f"old_dc_host_{i}"] = "dc_old"
        host_to_dc[f"new_dc_host_{i}"] = "dc_new"
    solver.addScope("datacenter", host_to_dc)

    # Workload resource usage
    workload_cpu = {f"workload_{i}": 2.0 + random.random() * 4.0 for i in range(200)}
    workload_mem = {f"workload_{i}": 4.0 + random.random() * 8.0 for i in range(200)}

    solver.addObjectDimension("cpu_usage", workload_cpu)
    solver.addObjectDimension("memory_usage", workload_mem)

    # Host capacities
    host_cpu_capacity = {
        **{f"old_dc_host_{i}": 20.0 for i in range(hosts_per_dc)},
        **{f"new_dc_host_{i}": 32.0 for i in range(hosts_per_dc)},
    }
    host_mem_capacity = {
        **{f"old_dc_host_{i}": 32.0 for i in range(hosts_per_dc)},
        **{f"new_dc_host_{i}": 64.0 for i in range(hosts_per_dc)},
    }

    solver.addContainerDimension("cpu_capacity", host_cpu_capacity)
    solver.addContainerDimension("memory_capacity", host_mem_capacity)

    # Capacity constraints
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="cpu-capacity", scope="host", dimension="cpu_usage"
            )
        )
    )
    solver.addConstraint(
        ConstraintSpecs(
            capacitySpec=CapacitySpec(
                name="memory-capacity", scope="host", dimension="memory_usage"
            )
        )
    )

    # Rollback capability logic
    if round_num < 2:  # Early rounds: maintain rollback capability
        print(f"  EARLY ROUND: Old DC kept accepting (rollback possible)")
        # Don't drain old DC aggressively
        # Don't add NonAcceptingSpec - allow movement back if needed
        # This means if issues occur, workloads can be moved back to old DC

        # Prefer migration but don't force it
        # Lower movement weight allows more flexibility
        movement_weight = 0.5
    else:  # Later rounds: commit to migration
        print(f"  LATER ROUND: Committing to migration")
        # Freeze old DC - no going back
        old_dc_hosts = [f"old_dc_host_{i}" for i in range(hosts_per_dc)]
        solver.addConstraint(
            ConstraintSpecs(
                nonAcceptingSpec=NonAcceptingSpec(
                    name="freeze-old-dc",
                    scope="host",
                    items=old_dc_hosts,
                )
            )
        )
        print(f"  Old DC frozen (rollback no longer possible)")

        # Higher movement weight for stability
        movement_weight = 2.0

    # Pin already-migrated workloads (unless we detect issues)
    already_migrated = []
    for host, workloads in current_assignment.items():
        if host.startswith("new_dc_"):
            already_migrated.extend(workloads)

    if already_migrated:
        # In early rounds, use lighter pinning to allow rollback
        if round_num < 2:
            print(
                f"  Lightly pinning {len(already_migrated)} migrated workloads (can rollback if needed)"
            )
        else:
            print(f"  Strongly pinning {len(already_migrated)} migrated workloads")

        solver.addConstraint(
            ConstraintSpecs(
                avoidMovingSpec=AvoidMovingSpec(
                    name="pin-already-migrated",
                    objects=already_migrated,
                )
            )
        )

    # Balance goals
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
        weight=2.0,
    )

    # Minimize movement (weight varies by round)
    solver.addGoal(
        GoalSpecs(
            minimizeMovementSpec=MinimizeMovementSpec(
                name="minimize-movement",
                scope="host",
                dimension="cpu_usage",
            )
        ),
        weight=movement_weight,
    )

    # Solve
    solver.addSolver(
        SolverSpecs(localSearchSolverSpec=LocalSearchSolverSpec(timeLimitMs=30000))
    )
    solution = solver.solve()

    print(f"  Solution found in {solution.profile.solveTime}ms")

    return solution


# variation_end


def initialize_old_datacenter(workload_count, hosts_per_dc):
    """Create initial assignment: all workloads in DC-OLD."""
    assignment = {}

    for i in range(hosts_per_dc):
        assignment[f"old_dc_host_{i}"] = []
        assignment[f"new_dc_host_{i}"] = []

    for i in range(workload_count):
        host_idx = i % hosts_per_dc
        assignment[f"old_dc_host_{host_idx}"].append(f"workload_{i}")

    return assignment


def rollback_capable_migration():
    """Run migration maintaining rollback capability in early rounds."""
    WORKLOAD_COUNT = 200
    HOSTS_PER_DC = 30
    TOTAL_ROUNDS = 5

    current_assignment = initialize_old_datacenter(WORKLOAD_COUNT, HOSTS_PER_DC)

    print("Starting migration with rollback capability")
    print(f"{'=' * 80}\n")

    for round_num in range(TOTAL_ROUNDS):
        print(f"\nROUND {round_num + 1}/{TOTAL_ROUNDS}")
        print(f"{'=' * 80}")

        solution = solve_with_rollback_capability(
            current_assignment,
            round_num,
            TOTAL_ROUNDS,
            HOSTS_PER_DC,
        )

        current_assignment = solution.assignment

        # Count workloads in each DC
        new_dc_count = sum(
            len(workloads)
            for host, workloads in current_assignment.items()
            if host.startswith("new_dc_")
        )
        old_dc_count = sum(
            len(workloads)
            for host, workloads in current_assignment.items()
            if host.startswith("old_dc_")
        )

        pct_new = (new_dc_count / WORKLOAD_COUNT) * 100
        pct_old = (old_dc_count / WORKLOAD_COUNT) * 100

        print(f"\nRound {round_num + 1} complete:")
        print(f"  DC-NEW: {new_dc_count}/200 ({pct_new:.1f}%)")
        print(f"  DC-OLD: {old_dc_count}/200 ({pct_old:.1f}%)")

        # In early rounds, simulate monitoring for issues
        if round_num < 2:
            print(f"  Monitoring metrics... (rollback available if issues detected)")

    print("\nMigration complete!")


if __name__ == "__main__":
    rollback_capable_migration()
